#include "network/session.hpp"
#include "network/TCPSocket.hpp"
#include "util/constants.hpp"
#include "util/utils.hpp"
#include <mutex>
#include <nlohmann/detail/meta/type_traits.hpp>
#include <stdexcept>
#include <thread>
#include <variant>

Session::Session(TCPSocket s, Direction dir) : sock(std::move(s)), dir(dir) {
    if (dir == Direction::Incoming) {
        state = State::ReceivedRequest;
    } else {
        state = State::SendingRequest;
    }
    std::string uuid_and_name = dm.get_uuid() + ":" + dm.get_name();
    AcceptMessage = constants::StaticAcceptRequest + uuid_and_name +
                    constants::MessageEndSymbol;
    DenyMessage = constants::StaticDenyRequest + uuid_and_name +
                  constants::MessageEndSymbol;
    ConnectRequestMessage = constants::StaticConnectRequest + uuid_and_name +
                            constants::MessageEndSymbol;
}

void Session::process() {
    process_cmd_thread = std::thread(&Session::process_cmd, this);
    process_msg_thread = std::thread(&Session::process_msg, this);
    if (process_cmd_thread.joinable()) {
        process_cmd_thread.join();
    }
    if (process_msg_thread.joinable()) {
        process_msg_thread.join();
    }
}

void Session::close_sock() { sock.close(); }

void Session::send_accept(int timeout) { sock.send(AcceptMessage); }

void Session::send_deny(int timeout) { sock.send(DenyMessage); }

void Session::send_request(int timeout) { sock.send(ConnectRequestMessage); }

bool Session::get_access_or_deny(int timeout) {
    std::string buf;
    char c = 's';
    while (c != constants::MessageEndSymbol) {
        sock.receive(&c, 1);
        buf += c;
    }
    if (buf == AcceptMessage) {
        return true;
    } else if (buf == DenyMessage) {
        return false;
    } else {
        throw std::runtime_error("Unknow message on get response");
    }
}

bool Session::set_cmd(Cmd cmd) {
    std::lock_guard<std::mutex> lock(mtx);
    switch (cmd) {
    case Cmd::Accept:
    case Cmd::Deny:
        if (step == Step::Initialization && state == State::ReceivedRequest) {
            command = cmd;
            return true;
        }
        return false;
    case Cmd::Disconnect:
        if (step == Step::DataTransfering && state == State::Connected) {
            command = cmd;
            return false;
        }
        return false;
    case Cmd::Idle:
        return false;
    }
    cmd_cv.notify_all();
}

bool Session::is_stopped() {
    std::lock_guard<std::mutex> lock(mtx);
    return step == Step::Stopped;
}

void Session::process_cmd() {
    std::unique_lock<std::mutex> lock(mtx);
    while (step != Step::Stopped) {
        switch (state) {
        case State::ReceivedRequest:
            if (command == Cmd::Accept) {
                send_accept(0);
                state = State::Connected;
            } else if (command == Cmd::Deny) {
                send_deny(0);
                state = State::Denied;
            } else {
                auto old_cmd = command;
                cmd_cv.wait(lock, [&] { return old_cmd != command; });
            }
            break;
        case State::SendingRequest:
            send_request(0);
            state = State::WaitAnswer;
            break;
        case State::WaitAnswer:
            if (get_access_or_deny(0)) {
                state = State::Connected;
                logmsg("Connected");
            } else {
                logmsg("Denied");
                state = State::Denied;
            }
            break;
        case State::Connected:
            step = Step::DataTransfering;
            state_and_step_cv.notify_all();
            state_and_step_cv.wait(lock, [&] {
                return state == State::Error || state == State::Closed;
            });
            break;
        case State::Denied:
        case State::Error:
        case State::Closed:
            step = Step::Finalization;
            close_sock();
            step = Step::Stopped;
            break;
        }
        state_and_step_cv.notify_all();
    }
}

bool Session::push_msg(BigMsg msg) { return send_queue.push(msg); }

bool Session::push_msg(SmallMsg msg) { return send_queue.push(msg); }

// TODO: дописать вот эту фигню, чтобы цикл не крутился вхолостую, там надо
// state_and_step_cv на send_queue поставить и на step
void Session::process_msg() {
    while (step != Step::Stopped) {
        auto msg_or_null = send_queue.get();
        if (msg_or_null.has_value()) {
            auto msg = msg_or_null.value();
            if (std::holds_alternative<SmallMsg>(msg)) {
                auto small_msg = std::get<SmallMsg>(msg);
                sock.send(small_msg.msg);
            } else {
                auto big_msg = std::get<BigMsg>(msg);
                std::vector<char> buf;
                while ((buf = big_msg.read(constants::BUFFER_SIZE)).size() !=
                       0) {
                    sock.send(buf, buf.size());
                }
            }
        } else {
            // или доотправить оставшиеся или кинуть bye сообщение
            break;
        }
        auto oldStep = step;
        std::unique_lock<std::mutex> lock(mtx);
        state_and_step_cv.wait(lock, [&] {
            return oldStep != step || !send_queue.empty() || !recv_pipe.empty();
        });
    }
}
