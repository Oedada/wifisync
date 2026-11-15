#include "network/session.hpp"
#include "network/TCPSocket.hpp"
#include "util/constants.hpp"
#include "util/exceptions.hpp"
#include "util/logger.hpp"
#include <cstdint>
#include <functional>
#include <mutex>
#include <nlohmann/detail/meta/type_traits.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>

Session::Session(TCPSocket s, Direction dir)
    : sock(std::move(s)), send_queue(process_msg_cv), recv_pipe(), dir(dir) {
    if (dir == Direction::Incoming) {
        state = State::Idle;
    } else {
        state = State::SendingRequest;
    }
    std::string uuid_and_name =
        DataManager::get_uuid() + ":" + DataManager::get_name();
    AcceptMessage = constants::StaticAcceptRequest + std::string(":") +
                    uuid_and_name + constants::MessageEndSymbol;
    DenyMessage = constants::StaticDenyRequest + std::string(":") +
                  uuid_and_name + constants::MessageEndSymbol;
    ConnectRequestMessage = constants::StaticConnectRequest + std::string(":") +
                            uuid_and_name + constants::MessageEndSymbol;
}

void Session::process() {
    process_cmd_thread = std::thread(&Session::process_cmd, this);
    process_msg_thread = std::thread(&Session::process_msg, this);
    receive_thread = std::thread(
        &TCPSocket::receive_callback, &sock,
        std::bind(&Session::sock_recv_callback, this, std::placeholders::_1));
    if (process_cmd_thread.joinable()) {
        process_cmd_thread.join();
        spdlog::get("session")->debug("Process cmd thread joined");
    }
    if (process_msg_thread.joinable()) {
        process_msg_thread.join();
        spdlog::get("session")->debug("Process msg thread joined");
    }
    if (receive_thread.joinable()) {
        receive_thread.join();
        spdlog::get("session")->debug("Receive thread joined");
    }
}

void Session::sock_recv_callback(uint64_t n) {
    if (n == 0) {
        std::lock_guard<std::mutex> lock(mtx);
        state = State::Closed;
        close();
    }
}

void Session::send_accept() { sock.send(AcceptMessage); }

void Session::send_deny() { sock.send(DenyMessage); }

void Session::send_request() { sock.send(ConnectRequestMessage); }

HelloMsg parse_msg(std::string s) {
    HelloMsg hello_msg{};
    std::stringstream ss(s);
    std::getline(ss, hello_msg.msg, ':');
    std::getline(ss, hello_msg.uuid, ':');
    std::getline(ss, hello_msg.name, ':');
    return hello_msg;
}

bool Session::accept_or_deny(std::string s) {
    auto parsed_msg = parse_msg(s);
    if (parsed_msg.msg == constants::StaticAcceptRequest) {
        return true;
    } else if (parsed_msg.msg == constants::StaticDenyRequest) {
        return false;
    } else {
        throw std::runtime_error("Unknow message on get response: " + s);
    }
}

bool Session::is_got_request(int timeout) {
    std::string buf;
    char c = 's';
    while (c != constants::MessageEndSymbol) {
        if (sock.receive(&c, 1, timeout)) {
            buf += c;
        } else {
            return false;
        }
    }
    return parse_msg(buf).msg == constants::StaticConnectRequest;
}

bool Session::get_access_or_deny(int timeout) {
    std::string buf;
    char c = 's';
    while (c != constants::MessageEndSymbol) {
        if (sock.receive(&c, 1, timeout)) {
            buf += c;
        } else {
            return false;
        }
    }
    return accept_or_deny(buf);
}

bool Session::set_cmd(Cmd cmd) {
    std::lock_guard<std::mutex> lock(mtx);
    spdlog::get("session")->trace("Set command {}, state: {}",
                                  static_cast<int>(cmd),
                                  static_cast<int>(state.load()));
    switch (cmd) {
    case Cmd::Accept:
    case Cmd::Deny:
        if (state.load() == State::Idle ||
            state.load() == State::ReceivedRequest) {
            command = cmd;
            process_cmd_cv.notify_one();
            return true;
        }
        return false;
    case Cmd::Disconnect:
        if (state.load() == State::Connected) {
            command = cmd;
            process_cmd_cv.notify_one();
            return true;
        }
        return false;
    case Cmd::Idle:
        return false;
    }
    return false;
}

void Session::process_cmd() {
    spdlog::get("session")->debug("ID: Process cmd thread");
    std::unique_lock<std::mutex> lock(mtx);
    while (state.load() != State::Closed && state.load() != State::Error) {
        switch (state.load()) {
        case State::Idle:
            if (is_got_request(constants::request_timeout_milliseconds)) {
                state.store(State::ReceivedRequest);
                spdlog::get("session")->info("Received request");
            } else {
                spdlog::get("session")->info("Denied");
                close();
                state.store(State::Denied);
            }
        case State::ReceivedRequest:
            if (command == Cmd::Accept) {
                send_accept();
                spdlog::get("session")->info("Send accept");
                state.store(State::Connected);
            } else if (command == Cmd::Deny) {
                send_deny();
                spdlog::get("session")->info("Send deny");
                state.store(State::Denied);
            } else {
                auto old_cmd = command;
                process_cmd_cv.wait(lock, [&] { return old_cmd != command; });
            }
            break;
        case State::SendingRequest:
            send_request();
            spdlog::get("session")->info("Request sended");
            state.store(State::WaitAnswer);
            break;
        case State::WaitAnswer:
            if (get_access_or_deny(constants::request_timeout_milliseconds)) {
                state.store(State::Connected);
                spdlog::get("session")->info("Connected");
            } else {
                spdlog::get("session")->info("Request Denied");
                close();
                state.store(State::Denied);
            }
            break;
        case State::Connected:
            process_msg_cv.notify_all();
            // spdlog::get
            process_cmd_cv.wait(lock, [&] {
                return (state.load() == State::Error ||
                        state.load() == State::Closed) or
                       command == Cmd::Disconnect;
            });
            spdlog::get("session")->info(
                "Process Cmd woke up from Connected state");
            if (command == Cmd::Disconnect) {
                spdlog::get("session")->info("Disconnecting");
                close();
            }
            break;
        case State::Denied:
        case State::Error:
        case State::Closed:
            break;
        }
        process_msg_cv.notify_one();
        receive_cv.notify_one();
    }
}

bool Session::push_msg(std::variant<SmallMsg, BigMsg> msg) {
    return send_queue.push(msg);
}

void Session::process_msg() {
    spdlog::get("session")->debug("ID: Process msg thread");
    while (state.load() != State::Closed && state.load() != State::Error) {
        if (state.load() == State::Connected) {
            if (!send_queue.empty()) {
                auto msg_or_null = send_queue.get();
                if (msg_or_null.has_value()) {
                    auto msg = msg_or_null.value();
                    if (std::holds_alternative<SmallMsg>(msg)) {
                        auto small_msg = std::get<SmallMsg>(msg);
                        try {
                            sock.send(small_msg.msg);
                            spdlog::get("session")->debug("Send SmallMsg: {}",
                                                          small_msg.msg);
                        } catch (const NetworkError &err) {
                            saved_error = Error::NetworkError;
                            spdlog::get("session")->error("network error");
                            close();
                        } catch (const ConnectionClosedError &err) {
                            saved_error = Error::ConnectionClosed;
                            close();
                            spdlog::get("session")->error("connection closed");
                        }
                    } else {
                        spdlog::get("session")->debug("Send BigMsg");
                        auto big_msg = std::get<BigMsg>(msg);
                        std::vector<std::byte> buf;
                        while ((buf = big_msg.read(constants::BUFFER_SIZE))
                                   .size() != 0) {
                            sock.send(
                                std::span<std::byte>(buf.data(), buf.size()));
                        }
                    }
                } else {
                    // или доотправить оставшиеся или кинуть bye сообщение
                    spdlog::get("session")->warn("Breaking process msg loop, "
                                                 "because send queue is empty");
                    break;
                }
            }
            std::unique_lock<std::mutex> lock(mtx);
            auto oldState = state.load();
            process_msg_cv.wait(lock, [&] {
                return oldState != state.load() || !send_queue.empty();
            });
        }
    }
}

std::vector<std::byte> Session::recv_bytes(uint64_t n) {
    while (state.load() != State::Closed && state.load() != State::Error) {
        if (state.load() == State::Connected) {
            std::vector<std::byte> buf(n);
            spdlog::get("session")->trace("Received: {}", buf);
            sock.receive(buf.data(), n);
            return buf;
        } else {
            std::unique_lock<std::mutex> lock(mtx);
            auto oldState = state.load();
            receive_cv.wait(lock, [&] { return oldState != state.load(); });
        }
    }
    return {};
}

void Session::close() {
    state.store(State::Closed);
    process_cmd_cv.notify_one();
    process_msg_cv.notify_one();
    receive_cv.notify_one();
    sock.close();
}
