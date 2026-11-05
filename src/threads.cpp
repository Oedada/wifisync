#include "core/change_applier.hpp"
#include "core/dif_walker.hpp"
#include "core/dif_work.hpp"
#include "core/transport.hpp"
#include "fs/environment.hpp"
#include "network/TCPSocket.hpp"
#include "network/broadcast.hpp"
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
    conflicts = {};
    logmsg("Calculating difference...");
    DifWork difwork(uuid);
    difwork.shift_snapshots();
    difwork.calculate_dif();
    json dif = FileAccess::read_json(constants::paths::data::difference);
    DifWalker dw(dif);
    logmsg("Starting synchronization...");
    Session session(std::move(sock), dir);
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

int main() {}
