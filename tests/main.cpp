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
#include <algorithm>
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
    conflicts = {};
    logmsg("Calculating difference...");
    DifWork difwork(uuid);
    std::string own_path;
    std::string other_path;
    if (dir == Direction::Incoming) {
        own_path = "./dryrun.d/server/test/";
        other_path = "./dryrun.d/client/test/";
    } else {
        other_path = "./dryrun.d/server/test/";
        own_path = "./dryrun.d/client/test/";
    }
    auto devices_uuid = DataManager::devices.uuid_list();
    if (std::find(devices_uuid.begin(), devices_uuid.end(), uuid) ==
        devices_uuid.end()) {
        DataManager::devices.add_device(uuid, "oedada1");
    }
    auto watched_paths = DataManager::list_watch();
    if(std::find(watched_paths.begin(), watched_paths.end(), own_path) == watched_paths.end()){
        DataManager::add_watch(own_path);
    }
    DataManager::add_watch(own_path);
    if (!DataManager::devices.get_device(uuid).paths_pairs.contains(own_path)) {
        DataManager::devices.set_path_pair_to_device(uuid, own_path,
                                                     other_path);
    }
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
              "e5420172-9517-4b5c-93a8-d6cf576a4d41");
}

void client() {
    full_sync(client_connect(addr_from_ip_and_port("127.0.0.1", 12345)),
              Direction::Outgoing, "c3d78a40-b38d-464f-835d-8c4e7df390eb");
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
