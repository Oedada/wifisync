#include "network/session.hpp"
#include "network/TCPSocket.hpp"
#include "util/constants.hpp"
#include "util/utils.hpp"
#include <stdexcept>

Session::Session(TCPSocket s, Direction dir) : sock(std::move(s)), dir(dir) {
    if (dir == Direction::Incoming) {
        state = State::ReceivedRequest;
    } else {
        state = State::SendingRequest;
    }
    std::string uuid_and_name = dm.get_uuid() + ":" + dm.get_name();
    ConnectRequestHello = constants::StaticRequestConnect + uuid_and_name;
    ConnectResponseHello = constants::StaticResponseConnect + uuid_and_name;
}

void Session::read_received_data() {
    if (is_ready_to_recv()) {
        char buf[constants::BUFFER_SIZE];
        sockaddr_in tmp_addr;
        socklen_t sender_len = sizeof(tmp_addr);
        ssize_t n;
        while (is_ready_to_recv()) {
            n = recvfrom(broadcast_sock, buf, sizeof(buf) - 1, 0,
                         (sockaddr *)&tmp_addr, &sender_len);
        }
        if (n > 0) {
            recv_size = n;
            memcpy(recv_buf, buf, constants::BUFFER_SIZE);
            tmp_recv_addr = tmp_addr;
        }
    }
}

void Session::cycle() {
    while (true) {
        switch (state) {
        case State::ReceivedRequest:
            if (command == Cmd::Accept) {
                // send accept
            } else if (command == Cmd::Deny) {
                // send deny
            }
            break;
        case State::SendingRequest:
            //send request
            state = State::WaitAnswer;
            break;
        case State::WaitAnswer:
            // if(пришел)state = State::Connected если пол, если нет - state = State::Denied
            break;
        case State::Connected:
            //отправляем данные из очереди
            break;
        case State::Denied:
            //ждём команду disconnect или разъединение с той стороны и меняем на State::Closed
            break;
        case State::Error:
            // ждем команду Disconnect
            break;
        case State::Closed:
            // уничтожаем сокет
            break;
        }
    }
}
