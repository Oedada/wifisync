#include <openssl/rand.h>
#include "headers/broadcast.hpp"
#include "headers/x25519.hpp"
#include "headers/ed25519.hpp"
#include <unistd.h>
#include <variant>
#include <thread>
#include <nlohmann/json.hpp>
#include "headers/request_server.hpp"
#include "headers/sync.hpp"


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

    ts.svr.Post("/incoming_connect", [&](const httplib::Request& req, httplib::Response& res) {
        json ret;
        ret["check"] = si.check_incoming_connections();
        res.set_content(ret.dump(), "application/json");
    });

    std::thread api_thread([&ts](){return ts.start_server(5000);});
    
    api_thread.join();
    broadcast_thread.join();
}
