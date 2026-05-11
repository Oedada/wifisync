#include "TCPSocket.hpp"
#include "broadcast.hpp"
#include "x25519.hpp"
#include "ed25519.hpp"
#include "http_server.hpp"
#include "transport.hpp"
#include "dif_work.hpp"
#include "environment.hpp"
#include "utils.hpp"
#include "change_applier.hpp"
#include "dif_walker.hpp"
#include "init.hpp"
#include <exception>
#include <openssl/rand.h>
#include <tuple>
#include <unistd.h>
#include <thread>
#include <nlohmann/json.hpp>


using json = nlohmann::json;
SessionInitializer si;

enum class GetConnectionType{
    Accept,
    Connect
};

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
    DifWork difwork(uuid);
    difwork.shift_snapshots();
    difwork.calculate_dif();
    json dif = read_json(env::get_data_path(constants::DATA_DIR) / constants::DIFFERENCE_FILENAME, false);
    DifWalker dw(dif);
    if(is_server){
        sock.send(count_subelements(dif));
        auto tr = Transport(std::move(sock));
        dw.walk([&](SUnit u){tr.send(u);});
        tr.walk_received(ChangeApplier::apply_runit);
    }
    else{
        sock.send(count_subelements(dif));
        Transport tr(std::move(sock));
        tr.walk_received(ChangeApplier::apply_runit);
        std::cout << count_subelements(dif) << "\n";
        dw.walk([&](SUnit u){tr.send(u);});
    }
}


int main() {
    init();
    std::thread broadcast_thread([&](){return si.broadcast();});
    std::thread sync_thread;
    HTTPServer hs;

    hs.add_handler(RequestType::get, "/devices", [&](const httplib::Request&, httplib::Response& res) {
        json ret;
        for(auto [key, p] : si.found_devices){
            ret[key] = p.second;
        }
        ret["fake_device"] = "neuuid";
        res.set_content(ret.dump(), "application/json");
    });

    hs.add_handler(RequestType::get, "/incoming_connect", [&](const httplib::Request&, httplib::Response& res) {
        json ret;
        auto [status, uuid] = si.check_incoming_connections();
        if(!status){
            json_error(res, "There isn't any incoming connection", {});
        }
        else{
            ret["uuid"] = uuid;
            json_ok(res, ret);
        }
    });


    hs.add_handler(RequestType::post, "/accept_connect", [&](const httplib::Request& req, httplib::Response& res) {
        json j = json::parse(req.body);
        std::string uuid = j.at("uuid").get<std::string>();
        json ret;
        bool answer = j.at("answer").get<bool>();
        if(answer){
            auto [success, sock, is_server] = handle_connection(GetConnectionType::Accept, uuid, ret, res);
            if(success){
                 sync_thread = std::thread([sock = std::move(sock), is_server, uuid]()mutable {full_sync(std::move(sock), is_server, uuid);});
            }
        }
    });
    
    hs.add_handler(RequestType::post, "/connect", [&](const httplib::Request& req, httplib::Response& res) {
        json j = json::parse(req.body);
        json ret;
        std::string uuid = j.at("uuid").get<std::string>();
        std::cout << "HTTPServer: Request for connect to " << uuid << "\n";
        auto [success, sock, is_server] = handle_connection(GetConnectionType::Connect, uuid, ret, res);
        if(success){
            sync_thread = std::thread([sock = std::move(sock), is_server, uuid]()mutable{return full_sync(std::move(sock), is_server, uuid);});
        }
    });

    std::thread api_thread([&hs](){return hs.start_server(5000);});
    
    if(sync_thread.joinable()){
        sync_thread.join();
    }
    api_thread.join();
    broadcast_thread.join();
}
