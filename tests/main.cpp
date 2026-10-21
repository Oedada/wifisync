#include "fs/data_manager.hpp"
#include "network/TCPSocket.hpp"
#include "network/server.hpp"
#include "network/session.hpp"
#include "util/init.hpp"
#include "util/utils.hpp"
#include <functional>
#include <thread>

void client() {
    Session session(client_connect(addr_from_ip_and_port("127.0.0.1", 7777)),
                    Direction::Outgoing);
    std::thread t(&Session::process, std::ref(session));
    t.join();
}

void server() {
    Server s(7777);
    while (!s.is_ready_to_accept()) {
    }
    Session session(s.accept_conn(), Direction::Incoming);
    logmsg("connected");
    std::thread t(&Session::process, std::ref(session));
    session.set_cmd(Cmd::Accept);
    t.join();
}

int main() {
    if (is_dryrun()) {
        logmsg("dry run");
    }
    init();
    DataManager dm;
    logmsg(dm.get_uuid());
    std::thread client_th(client);
    std::thread server_th(server);
    client_th.join();
    server_th.join();
    return 0;
}
