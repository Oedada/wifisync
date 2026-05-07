#include <exception>
#include <openssl/rand.h>
#include "TCPSocket.hpp"
#include "broadcast.hpp"
#include "x25519.hpp"
#include "ed25519.hpp"
#include <tuple>
#include <unistd.h>
#include <thread>
#include <nlohmann/json.hpp>
#include "http_server.hpp"


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
        std::cout << sock.ip();
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


int main() {
    std::thread broadcast_thread([&](){return si.broadcast();});

    HTTPServer hs;

    hs.add_handler(RequestType::get, "/devices", [&](const httplib::Request&, httplib::Response& res) {
        json ret;
        for(auto [key, p] : si.found_devices){
            ret[p.second] = key;
        }
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
        json ret;
        bool answer = j.at("answer").get<bool>();
        std::string uuid = j.at("uuid").get<std::string>();
        if(answer){
            auto [succes, sock, is_server] = handle_connection(GetConnectionType::Accept, uuid, ret, res);
        }
    });

    hs.add_handler(RequestType::post, "/connect", [&](const httplib::Request& req, httplib::Response& res) {
        json j = json::parse(req.body);
        json ret;
        std::string uuid = j.at("uuid").get<std::string>();
        auto [succes, sock, is_server] = handle_connection(GetConnectionType::Connect, uuid, ret, res);
    });

    std::thread api_thread([&hs](){return hs.start_server(5000);});
    
    api_thread.join();
    broadcast_thread.join();
}
