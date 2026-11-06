#include "core/change_applier.hpp"
#include "core/dif_walker.hpp"
#include "core/dif_work.hpp"
#include "core/transport.hpp"
#include "fs/data_manager.hpp"
#include "fs/environment.hpp"
#include "network/TCPSocket.hpp"
#include "network/server.hpp"
#include "network/session.hpp"
#include "util/constants.hpp"
#include "util/init.hpp"
#include "util/utils.hpp"
#include <functional>
#include <nlohmann/json.hpp>
#include <openssl/rand.h>
#include <string>
#include <sys/prctl.h>
#include <thread>
#include <unistd.h>

enum class GetConnectionType { Accept, Connect };

using json = nlohmann::json;
std::vector<fs::path> conflicts;

void full_sync(TCPSocket sock, Direction dir, std::string uuid) {
    DataManager dm;
    conflicts = {};
    logmsg("Calculating difference...");
    DifWork difwork(uuid);
    std::string other_uuid;
    std::string own_path;
    std::string other_path;
    if (dir == Direction::Incoming) {
        other_uuid = "35d04bbc-aa32-4972-8871-2f9082b5c940";
        own_path = "./dryrun.d/server/test/";
        other_path = "./dryrun.d/client/test/";
    } else {
        other_uuid = "b3efc590-c5d0-45bd-adf0-0cc94207cb54";
        other_path = "./dryrun.d/server/test/";
        own_path = "./dryrun.d/client/test/";
    }
    dm.devices.add_device(other_uuid, "oedada1");
    int res = dm.devices.set_path_pair_to_device(
        other_uuid, own_path,
        other_path);
    difwork.shift_snapshots();
    difwork.calculate_dif();
    json dif = FileAccess::read_json(constants::paths::data::difference);
    DifWalker dw(dif);
    logmsg("Starting synchronization...");
    Session session(std::move(sock), dir);
    std::thread session_thread(&Session::process, &session);
    if (dir == Direction::Incoming) {
        session.set_cmd(Cmd::Accept);
    }
    session.push_msg(count_subelements(dif));
    Transport tr(
        std::bind(&Session::push_msg, &session, std::placeholders::_1),
        std::bind(&Session::recv_bytes, &session, std::placeholders::_1));
    if (dir == Direction::Incoming) {
        dw.walk([&](SUnit u) { tr.send(u); });
        conflicts = tr.walk_received(ChangeApplier::apply_runit);
    } else {
        conflicts = tr.walk_received(ChangeApplier::apply_runit);
        dw.walk([&](SUnit u) { tr.send(u); });
    }
    difwork.shift_snapshots();
    print_vector(conflicts);
    logmsg("Successfully sync!");
}

void server() {
    Server server(12345);
    while (!server.is_ready_to_accept())
        ;
    full_sync(server.accept_conn(), Direction::Incoming,
              "35d04bbc-aa32-4972-8871-2f9082b5c940");
}

void client() {
    full_sync(client_connect(addr_from_ip_and_port("127.0.0.1", 12345)),
              Direction::Outgoing, "b3efc590-c5d0-45bd-adf0-0cc94207cb54");
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
