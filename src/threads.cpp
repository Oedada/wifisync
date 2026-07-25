#include "core/change_applier.hpp"
#include "core/dif_walker.hpp"
#include "core/dif_work.hpp"
#include "core/transport.hpp"
#include "crypto/ed25519.hpp"
#include "crypto/x25519.hpp"
#include "fs/environment.hpp"
#include "network/broadcast.hpp"
#include "network/http_server.hpp"
#include "network/TCPSocket.hpp"
#include "util/constants.hpp"
#include "util/init.hpp"
#include "util/utils.hpp"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <nlohmann/json.hpp>
#include <openssl/rand.h>
#include <signal.h>
#include <string>
#include <sys/prctl.h>
#include <thread>
#include <tuple>
#include <unistd.h>

enum class GetConnectionType{
    Accept,
    Connect
};

struct Status{
    int get() const {
        return val;
    }
    int set(int v){
        return val = v;
    }
    private:
        int val = 0;

};
using json = nlohmann::json;
SessionInitializer si;
Status st;
bool missing_uuid = false;
std::vector<fs::path> conflicts;


std::tuple<bool, TCPSocket, bool> handle_connection(GetConnectionType gct, std::string uuid, json body, httplib::Response& res){
    bool success = false, is_server = false;
    TCPSocket sock;
    try{
        if(gct == GetConnectionType::Accept){
            std::tie(success, sock, is_server) = si.accept_connection(uuid);
        }
        else{
            std::tie(success, sock, is_server) = si.connect_to(uuid);
        }
        if(success){
            json_ok(res, body);
        }
        else{
            json_error(res, "Can't accept connection", body);
        }
    } catch(std::exception &e){
        json_error(res, e.what(), body);
        success = false;
    }
    return {success, std::move(sock), is_server};
}

void full_sync(TCPSocket sock, bool is_server, std::string uuid){
    conflicts = {};
    st.set(2);
    logmsg("Calculating difference...");
    DifWork difwork(uuid);
    difwork.shift_snapshots();
    difwork.calculate_dif();
    json dif = read_json(env::get_data_path(constants::DATA_DIR) / constants::DIFFERENCE_FILENAME, false);
    DifWalker dw(dif);
    st.set(3);
    logmsg("Starting synchronization...");
    sock.send(0);
    uint64_t ok = sock.recv_uint64();
    if(ok != 0){
        return;
    }
    sock.send(count_subelements(dif));
    Transport tr(std::move(sock));
    if(is_server){
        st.set(4);
        dw.walk([&](SUnit u){tr.send(u);});
        st.set(5);
        conflicts = tr.walk_received(ChangeApplier::apply_runit);
    }
    else{
        st.set(4);
        conflicts = tr.walk_received(ChangeApplier::apply_runit);
        st.set(5);
        dw.walk([&](SUnit u){tr.send(u);});
    }
    difwork.shift_snapshots();
    print_vector(conflicts);
    logmsg("Successfully sync!");
    si.set_state(State::Discovering);
    st.set(6);
}

int main(){
    
}
