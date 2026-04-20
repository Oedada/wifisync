#include <exception>
#include <openssl/rand.h>
#include "broadcast.hpp"
#include "x25519.hpp"
#include "ed25519.hpp"
#include <unistd.h>
#include <variant>
#include <thread>
#include <nlohmann/json.hpp>
#include "request_server.hpp"
#include "sync.hpp"


using json = nlohmann::json;
using Arg = std::variant<bool, int, std::string>;

// Глобальная очередь сообщений
std::vector<httplib::Response*> clients;
SessionInitializer si;
SafeCmdQueue cmd_q;

//-----------//
//Thread work//
//-----------//


int main() {
    std::thread broadcast_thread([&](){return si.broadcast();});

    TaskServer ts(cmd_q);

    ts.svr.Get("/devices", [&](const httplib::Request&, httplib::Response& res) {
        json ret;
        for(auto [key, p] : si.found_devices){
            ret[p.second] = key;
        }
        res.set_content(ret.dump(), "application/json");
    });

    ts.svr.Get("/incoming_connect", [&](const httplib::Request& req, httplib::Response& res) {
        json ret;
        auto [status, uuid] = si.check_incoming_connections();
        ret["status"] = status;
        ret["uuid"] = uuid;
        res.set_content(ret.dump(), "application/json");
    });

    ts.svr.Post("/accept_connect", [&](const httplib::Request& req, httplib::Response& res) {
        json j = json::parse(req.body);
        json ret;
        bool answer = j.at("answer").get<bool>();
        std::string uuid = j.at("uuid").get<std::string>();
        if(answer){
            try{
                auto [succes, sock, is_server] = si.accept_connection(uuid);
                std::cout << sock.ip();
                if(succes){
                    ret["ok"] = true;
                    res.set_content(ret.dump(), "application/json");
                }
                else{
                    res.status = 400;
                    ret["ok"] = false;
                    res.set_content(ret.dump(), "application/json");
                }
            } catch(std::exception &e){
                res.status = 400;
                ret["ok"] = false;
                ret["error"] = e.what();
                res.set_content(ret.dump(), "application/json");
            }
        }
    });

    ts.svr.Post("/connect", [&](const httplib::Request& req, httplib::Response& res) {
        json j = json::parse(req.body);
        json ret;
        std::string uuid = j.at("uuid").get<std::string>();
        try{
            auto [succes, sock, is_server] = si.connect_to(uuid);
            std::cout << sock.ip();
            if(succes){
                ret["ok"] = true;
                res.set_content(ret.dump(), "application/json");
            }
            else{
                res.status = 400;
                ret["ok"] = false;
                res.set_content(ret.dump(), "application/json");
            }
        } catch(std::exception &e){
            res.status = 400;
            ret["ok"] = false;
            ret["error"] = e.what();
            res.set_content(ret.dump(), "application/json");
        }
    });

    std::thread api_thread([&ts](){return ts.start_server(5000);});
    
    api_thread.join();
    broadcast_thread.join();
}
