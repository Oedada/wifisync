#include <openssl/rand.h>
#include "x25519.hpp"
#include "ed25519.hpp"
#include <unistd.h>
#include "httplib.h"
#include <nlohmann/json.hpp>
#include "http_server.hpp"
#include "utils.hpp"
#include "constants.hpp"

void HTTPServer::start_server(int port){

    svr.set_mount_point("/", "gui/static");

    logmsg(std::string("Server running on http://") + constants::LOCAL_IP_ADDR + ":"  + std::to_string(port));
    svr.listen(constants::LOCAL_IP_ADDR, port);
}

void HTTPServer::add_handler(RequestType rt, std::string pattern, std::function<void (const httplib::Request &, httplib::Response &)> func){
    if(rt == RequestType::get){
        svr.Get(pattern, func);    
    } else if(rt == RequestType::post){
        svr.Post(pattern, func);    
    }
}

void json_ok(httplib::Response& res, nlohmann::json body){
    res.status = 200;
    body["ok"] = true;
    res.set_content(body.dump(), "application/json");
}

void json_error(httplib::Response& res, std::string err, nlohmann::json body){
    res.status = 400;
    body["ok"] = false;
    body["error"] = err;
    res.set_content(body.dump(), "application/json");
}


