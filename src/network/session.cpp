#include "network/session.hpp"
#include "network/TCPSocket.hpp"
#include "network/broadcast.hpp"
#include "util/constants.hpp"
#include "util/utils.hpp"
#include <stdexcept>

Session::Session() {
    std::string uuid_and_name = dm.get_uuid() + ":" + dm.get_name();
    ConnectRequestHello = constants::StaticRequestConnect + uuid_and_name;
    ConnectResponseHello = constants::StaticResponseConnect + uuid_and_name;
}

bool Session::set_response_cmd(ResponseCmd cmd) {
    switch (cmd.type) {
    case ResponseCmdType::Accept:
    case ResponseCmdType::Deny:
        if (resp_conn.state == ResponseState::Received &&
            req_conn.state != RequestState::Connected &&
            req_conn.state != RequestState::Connecting &&
            req_conn.state != RequestState::WaitAns) {
            resp_conn.cmd = cmd;
            return true;
        }
        return false;
    case ResponseCmdType::Cancel:
        if (resp_conn.state == ResponseState::Connecting) {
            resp_conn.cmd = cmd;
            return true;
        }
        return false;
    case ResponseCmdType::Disconnect:
        if (resp_conn.state == ResponseState::Connected) {
            resp_conn.cmd = cmd;
            return true;
        }
        return false;
    case ResponseCmdType::Pass:
        if (resp_conn.state == ResponseState::Error ||
            resp_conn.state == ResponseState::Denied) {
            resp_conn.cmd = cmd;
            return true;
        }
        return false;
    }
}

bool Session::set_request_cmd(RequestCmd cmd) {
    switch (cmd.type) {
    case RequestCmdType::Connect:
        if (req_conn.state == RequestState::Pass &&
            resp_conn.state != ResponseState::Connected &&
            resp_conn.state != ResponseState::Connecting &&
            resp_conn.state != ResponseState::Received) {
            req_conn.cmd = cmd;
            return true;
        }
        return false;
    case RequestCmdType::Disconnect:
        if (req_conn.state == RequestState::Connected) {
            req_conn.cmd = cmd;
            return true;
        }
        return false;
    case RequestCmdType::Cancel:
        if (req_conn.state == RequestState::Connected) {
            req_conn.cmd = cmd;
            return true;
        }
        return false;
    case RequestCmdType::Pass:
        if (req_conn.state == RequestState::Denied ||
            req_conn.state == RequestState::Error) {
            req_conn.cmd = cmd;
            return true;
        }
        return false;
    }
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
    switch (req_conn.cmd.type) {
    case RequestCmdType::Connect:
        if (req_conn.state == RequestState::Pass) {
            req_conn.state = RequestState::Connecting;
            auto [ip, port] =
                std::get<std::pair<std::string, int>>(req_conn.cmd.data);
            try {
                auto sock = client_connect(addr_from_ip_and_port(ip, port));
                sock.send(ConnectRequestHello.data());
                req_conn.state = RequestState::WaitAns;
            } catch (std::runtime_error &e) {
                logerr(e.what());
                req_conn.state = RequestState::Error;
            }
        } else if(req_conn.state == RequestState::WaitAns){
            
        }
    }
}
