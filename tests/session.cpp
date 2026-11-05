#include "network/TCPSocket.hpp"
#include "network/server.hpp"
#include "network/session.hpp"
#include "util/init.hpp"
#include "util/utils.hpp"
#include <stdexcept>
#include <string>
#include <thread>

void client() {
    Session s(client_connect(addr_from_ip_and_port("127.0.0.1", 7777)), Direction::Outgoing);
    std::thread t(&Session::process, &s);
    auto buf = s.recv_bytes(5);
    std::string ss(buf.begin(), buf.end());
    logmsg(ss);
    t.join();
}

void server() {
    Server server(7777);
    while (!server.is_ready_to_accept()) {
    }
    Session s(server.accept_conn(), Direction::Incoming);
    std::thread t(&Session::process, &s);
    s.set_cmd(Cmd::Accept);
    s.push_msg(SmallMsg("Hello"));
    t.join();
}

std::vector<std::string> check_args(int argc, char *argv[]) {
    std::vector<std::string> vs;
    for (int i = 0; i < argc; i++) {
        vs.push_back(std::string(argv[i]));
    }
    return vs;
}

int main(int argc, char *argv[]) {
    init();
    if (is_dryrun()) {
        logmsg("dry run");
    }
    auto args = check_args(argc, argv);
    if (args.empty()) {
        throw std::runtime_error("Null args");
    }
    if (args[1] == "s") {
        std::thread server_th(server);
        server_th.join();
    } else if (args[1] == "c") {
        std::thread client_th(client);
        client_th.join();
    }
    return 0;
}
