#include "network/session.hpp"
#include "network/TCPSocket.hpp"
#include "util/constants.hpp"
#include "util/utils.hpp"
#include <mutex>
#include <nlohmann/detail/meta/type_traits.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>

Session::Session(TCPSocket s, Direction dir) : sock(std::move(s)), dir(dir) {
    if (dir == Direction::Incoming) {
        state = State::Idle;
    } else {
        state = State::SendingRequest;
    }
    std::string uuid_and_name = dm.get_uuid() + ":" + dm.get_name();
    AcceptMessage = constants::StaticAcceptRequest + std::string(":") + uuid_and_name +
                    constants::MessageEndSymbol;
    DenyMessage = constants::StaticDenyRequest + std::string(":") + uuid_and_name +
                  constants::MessageEndSymbol;
    ConnectRequestMessage = constants::StaticConnectRequest + std::string(":") + uuid_and_name +
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

void Session::send_accept(int timeout) {logmsg("accept sended"); sock.send(AcceptMessage); }

void Session::send_deny(int timeout) { sock.send(DenyMessage); }

void Session::send_request(int timeout) { sock.send(ConnectRequestMessage); }

HelloMsg parse_msg(std::string s){
    HelloMsg hello_msg{};
    std::stringstream ss(s);
    std::getline(ss, hello_msg.msg, ':');
    std::getline(ss, hello_msg.uuid, ':');
    std::getline(ss, hello_msg.name, ':');
    return hello_msg;
}

bool Session::accept_or_deny(std::string s){
    auto parsed_msg = parse_msg(s);
    if(parsed_msg.msg == constants::StaticAcceptRequest){
        return true;
    } else if(parsed_msg.msg == constants::StaticDenyRequest){
        return false;
    } else {
        throw std::runtime_error("Unknow message on get response: " + s);
    }

}

bool Session::is_got_request(int timeout){
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
        if (step == Step::Initialization && (state == State::Idle || state == State::ReceivedRequest)) {
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
        case State::Idle:
           if(is_got_request(0)){
                state = State::ReceivedRequest;
           }
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

void Session::process_msg() {
    while (step != Step::Finalization) {
        if(step == Step::DataTransfering){
        if(!send_queue.empty()){
        auto msg_or_null = send_queue.get();
        if (msg_or_null.has_value()) {
            auto msg = msg_or_null.value();
            if (std::holds_alternative<SmallMsg>(msg)) {
                auto small_msg = std::get<SmallMsg>(msg);
                logmsg(std::string("For send:") + small_msg.msg);
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
        }}
        if(1){
            logmsg(std::to_string(sock.empty()));
            char buf[constants::BUFFER_SIZE];
            int n;
            n = sock.receive(buf, constants::BUFFER_SIZE, true);
            logmsg(std::string("Received:") + std::string(buf));
            recv_pipe.write(std::span<char>(buf, n));
        }
        auto oldStep = step;
        std::unique_lock<std::mutex> lock(mtx);
        state_and_step_cv.wait(lock, [&] {
            return oldStep != step || !send_queue.empty() || 1;
        });}
    }
}

std::vector<char> Session::recv_bytes(uint64_t n){
    return recv_pipe.read(n);
}
