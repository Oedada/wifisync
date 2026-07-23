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


// int main() {
//     prctl(PR_SET_PDEATHSIG, SIGKILL);
//     setvbuf(stdout, NULL, _IONBF, 0);
//     std::thread broadcast_thread([&](){si.broadcast();});
//     std::thread sync_thread;
//     HTTPServer hs;
    

//     hs.add_handler(RequestType::get, "/devices", [&](const httplib::Request&, httplib::Response& res) {
//         json ret;
//         for(auto [key, p] : si.get_found_devices()){
//             ret[key] = p.name;
//         }
//         // ret["fake_device"] = "neuuid";
//         res.set_content(ret.dump(), "application/json");
//     });

//     hs.add_handler(RequestType::get, "/status", [&](const httplib::Request&, httplib::Response& res) {
//         res.set_content(std::to_string(st.get()), "plain/text");
//     });

//     hs.add_handler(RequestType::get, "/conflicts", [&](const httplib::Request&, httplib::Response& res) {
//         json ret = {};
//         for(size_t i = 0; i < conflicts.size(); i++){
//             ret[i] = conflicts[i];
//         }
//         res.set_content(ret.dump(), "application/json");
//     });

//     hs.add_handler(RequestType::get, "/missing_uuid", [&](const httplib::Request&, httplib::Response& res) {
//         res.set_content(std::to_string(static_cast<int>(missing_uuid)), "plain/text");
//     });

    
//     hs.add_handler(RequestType::get, "/incoming_connect", [&](const httplib::Request&, httplib::Response& res) {
//         json ret;
//         auto [status, uuid] = si.check_incoming_connections();
//         if(!status){
//             json_error(res, "There isn't any incoming connection", {});
//         }
//         else{
//             ret["uuid"] = uuid;
//             json_ok(res, ret);
//         }
//     });
    
    
//     hs.add_handler(RequestType::post, "/accept_connect", [&](const httplib::Request& req, httplib::Response& res) {
//         json j = json::parse(req.body);
//         std::string uuid = j.at("uuid").get<std::string>();
//         json ret;
//         auto cfg = read_json(env::get_data_path() / constants::DEVICES_FILE, false);
//         if(!cfg.contains(uuid) || cfg.at(uuid).at("paths").empty()){
//             ret["ok"] = false;
//             ret["error"] = -2;
//             res.set_content(ret.dump(), "application/json");
//             return;
//         }
//         bool answer = j.at("answer").get<bool>();
//         if(answer){
//             auto [success, sock, is_server] = handle_connection(GetConnectionType::Accept, uuid, ret, res);
//             if(success){
//                 ret["ok"] = true;
//                 st.set(1);
//                 logmsg("Connection accepted successfully!");
//                 if(sync_thread.joinable()){
//                     sync_thread.detach();
//                 }
//                 sync_thread = std::thread([sock = std::move(sock), is_server, uuid]()mutable {full_sync(std::move(sock), is_server, uuid);});
//             }
//             else{
//                 ret["ok"] = false;
//                 ret["error"] = -1; //connection refused
//             }
//         }
//         res.set_content(ret.dump(), "application/json");
//     });
    
//     hs.add_handler(RequestType::post, "/connect", [&](const httplib::Request& req, httplib::Response& res) {
//         json j = json::parse(req.body);
//         json ret;
//         std::string uuid = j.at("uuid").get<std::string>();
//         auto cfg = read_json(env::get_data_path() / constants::DEVICES_FILE, false);
//         if(!cfg.contains(uuid) || cfg.at(uuid).at("paths").empty()){
//             ret["ok"] = false;
//             ret["error"] = -2;
//             res.set_content(ret.dump(), "application/json");
//             logwarn("Uuid not find!");
//             return;
//         }
//         logmsg("HTTPServer: Request for connect to " + uuid);
//         auto [success, sock, is_server] = handle_connection(GetConnectionType::Connect, uuid, ret, res);
//         if(success){
//             st.set(1);
//             logmsg("Connected successfully!");
//             if(sync_thread.joinable()){
//                 sync_thread.detach();
//             }
//             sync_thread = std::thread([sock = std::move(sock), is_server, uuid]()mutable{return full_sync(std::move(sock), is_server, uuid);});
//         }
//     });
    
//     std::thread api_thread([&hs](){return hs.start_server(5000);});
    
//     logmsg("Core started!");
//     if(sync_thread.joinable()){
//         sync_thread.join();
//     }
//     api_thread.join();
//     broadcast_thread.join();
// }
