#include <exception>
#include <iostream>
#include <map>
#include <openssl/rand.h>
#include "headers/server.hpp"
#include "headers/x25519.hpp"
#include "headers/ed25519.hpp"
#include <stdexcept>
#include <unistd.h>
#include <variant>
#include <mutex>
#include <queue>
#include "external_libs/cpp-httplib/httplib.h"
#include <nlohmann/json.hpp>
#include "headers/broadcast.hpp"
#include "headers/request_server.hpp"

using json = nlohmann::json;
using Arg = std::variant<bool, int, std::string>;


void SafeCmdQueue::add(tstypes::Command cmd){
    std::unique_lock<std::mutex> lock(m);
    queue.push(cmd);
    cv.notify_one();
}

tstypes::Command SafeCmdQueue::get(){
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [this](){return !queue.empty();});
    tstypes::Command front_el = std::move(queue.front());
    queue.pop();
    return front_el;
}

TaskServer::TaskServer(SafeCmdQueue &queue) : cmd_q(&queue){}
void TaskServer::start_server(int port){
    httplib::Server svr;

    // GET /hello
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("Wifisync http server's working", "text/plain");
    });

    svr.Post("/tasks", [this](const httplib::Request& req, httplib::Response& res){this->get_tasks(req, res);});

    std::cout << "Server running on http://0.0.0.0:" << port << "\n";
    svr.listen("0.0.0.0", port); // блокирующий вызов
}

int TaskServer::parse_task_json(json j, tstypes::Command& cmd){
    try{
        if(!j.at("args").is_array() || !j.at("cmd").is_string()){
            throw std::runtime_error("Type of json field not correct");
        }
        cmd.task = tstypes::str_to_tasks.at(j.at("cmd"));
        for(const nlohmann::basic_json<> & arg: j.at("args")){
            if(arg.is_boolean()){
                cmd.args.push_back(arg.get<bool>());
            }
            else if(arg.is_number_integer()){
                cmd.args.push_back(arg.get<int>());
            }
            else if(arg.is_string()){
                cmd.args.push_back(arg.get<std::string>());
            }
            else{
                std::cerr << std::string("Type of argument not correct: ") + std::string(arg);
                throw std::runtime_error(std::string("Type of argument not correct: "));
            }
        }
        return 200;
    }
    catch(json::out_of_range &e){
        return 400;
    }
}

void TaskServer::get_tasks(const httplib::Request& req, httplib::Response& res){
    try{
        json json_body = json::parse(req.body);
        res.set_content(json_body.dump(), "application/json");
        tstypes::Command cmd;
        int status = parse_task_json(json_body, cmd);
        if(status == 200){
            res.status = 200;
            res.set_content("Task received", "plain/text");
            for(auto arg: cmd.args){
                std::visit([](auto&& val){}, arg);
            }
            cmd_q->add(cmd);
        }
    }
    catch(const std::exception& e){
        res.status = 400;
        res.set_content(std::string("\"error\":\"") + e.what() + "\"", "application/json");
    }
}
