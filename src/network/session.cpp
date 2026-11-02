#include "network/session.hpp"
#include "network/TCPSocket.hpp"
#include "util/constants.hpp"
#include "util/exceptions.hpp"
#include "util/utils.hpp"
#include <cstdint>
#include <mutex>
#include <nlohmann/detail/meta/type_traits.hpp>
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
    std::string uuid_and_name = dm.get_uuid() + ":" + dm.get_name();
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
    receive_thread =
        std::thread(&TCPSocket::receive_callback, &sock, [this](uint64_t n) {});

    if (process_cmd_thread.joinable()) {
        process_cmd_thread.join();
    }
    if (process_msg_thread.joinable()) {
        process_msg_thread.join();
    }
    if (receive_thread.joinable()) {
        receive_thread.join();
    }
}

void Session::send_accept(int timeout) {
    logmsg("accept sended");
    sock.send(AcceptMessage);
}

void Session::send_deny(int timeout) { sock.send(DenyMessage); }

void Session::send_request(int timeout) { sock.send(ConnectRequestMessage); }

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
        sock.receive(&c, 1);
        buf += c;
    }
    return parse_msg(buf).msg == constants::StaticConnectRequest;
}

bool Session::get_access_or_deny(int timeout) {
    std::string buf;
    char c = 's';
    while (c != constants::MessageEndSymbol) {
        sock.receive(&c, 1);
        buf += c;
    }
    return accept_or_deny(buf);
}

bool Session::set_cmd(Cmd cmd) {
    std::lock_guard<std::mutex> lock(mtx);
    switch (cmd) {
    case Cmd::Accept:
    case Cmd::Deny:
        if (state == State::Idle || state == State::ReceivedRequest) {
            command = cmd;
            return true;
        }
        return false;
    case Cmd::Disconnect:
        if (state == State::Connected) {
            command = cmd;
            return false;
        }
        return false;
    case Cmd::Idle:
        return false;
    }
    process_cmd_cv.notify_one();
}

void Session::process_cmd() {
    std::unique_lock<std::mutex> lock(mtx);
    while (state.load() != State::Closed && state.load() != State::Error) {
        switch (state.load()) {
        case State::Idle:
            if (is_got_request(0)) {
                state.store(State::ReceivedRequest);
            } else {
                state.store(State::Denied);
            }
        case State::ReceivedRequest:
            if (command == Cmd::Accept) {
                send_accept(0);
                state.store(State::Connected);
            } else if (command == Cmd::Deny) {
                send_deny(0);
                state.store(State::Denied);
            } else {
                auto old_cmd = command;
                process_cmd_cv.wait(lock, [&] { return old_cmd != command; });
            }
            break;
        case State::SendingRequest:
            send_request(0);
            state.store(State::WaitAnswer);
            break;
        case State::WaitAnswer:
            if (get_access_or_deny(0)) {
                state.store(State::Connected);
                logmsg("Connected");
            } else {
                logmsg("Denied");
                state.store(State::Denied);
            }
            break;
        case State::Connected:
            process_msg_cv.notify_all();
            process_cmd_cv.wait(lock, [&] {
                return (state.load() == State::Error ||
                        state.load() == State::Closed) or
                       command == Cmd::Disconnect;
            });
            if (command == Cmd::Disconnect) {
                close();
                process_msg_cv.notify_one();
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

bool Session::push_msg(BigMsg msg) { return send_queue.push(msg); }

bool Session::push_msg(SmallMsg msg) { return send_queue.push(msg); }

void Session::process_msg() {
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
                        } catch (const NetworkError &err) {
                            saved_error = Error::NetworkError;
                            close();
                            // закрыть всё что нужно
                        } catch (const ConnectionClosedError &err) {
                            saved_error = Error::ConnectionClosed;
                            close();
                            // закрыть всё что нужно
                        }
                    } else {
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
                    logmsg("Breaking process msg loop");
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
            sock.receive(buf.data(), n);
            return buf;
        } else {
            std::unique_lock<std::mutex> lock(mtx);
            auto oldState = state.load();
            receive_cv.wait(lock, [&] { return oldState != state.load(); });
        }
    }
    throw std::runtime_error("lol");
}

void Session::close() {
    state.store(State::Closed);
    sock.close();
}
