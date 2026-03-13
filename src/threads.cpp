#include <chrono>
#include <exception>
#include <iostream>
#include <map>
#include <openssl/rand.h>
#include "headers/client.hpp"
#include "headers/server.hpp"
#include "headers/x25519.hpp"
#include "headers/ed25519.hpp"
#include <stdexcept>
#include <variant>
#include "headers/TCPSocket.hpp"
#include <thread>
#include <mutex>
#include <queue>
#include "external_libs/cpp-httplib/httplib.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using Arg = std::variant<bool, int, std::string>;

enum class Tasks{
    StartServer,
    Connect,
    SendTest
};


struct Command{
    Tasks task;
    std::vector<Arg> args;
};

class SafeCmdQueue{
    private:
        std::queue<Command> queue;
        std::mutex m;
        std::condition_variable cv;
    public:
        void add(Command cmd){
            std::unique_lock<std::mutex> lock(m);
            queue.push(cmd);
            cv.notify_one();
        }

        Command get(){
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock, [this](){return !queue.empty();});
            Command front_el = std::move(queue.front());
            queue.pop();
            return front_el;
        }
};

SafeCmdQueue cmd_q;
std::map<std::string, Tasks> str_to_tasks{
    {"start_server", Tasks::StartServer},
    {"connect", Tasks::Connect}, 
    {"send", Tasks::SendTest}
};


void server() {
    Server ftr(12345);
    while(true){
        if(ftr.is_ready_to_accept()){
            TCPSocket s = ftr.accept_conn();
            std::cout << "Client info - " << ftr.client_ip << ":" << ftr.client_port << std::endl;
        }
    }
}

void client(const int server_port,const std::string &server_ip){
    Client ftr(server_port, server_ip);
    TCPSocket s = ftr.connect_server();
    while(true){
        std::cout << "Client don't off" << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int parse_task_json(json j, Command& cmd){
    try{
        if(!j.at("args").is_array() || !j.at("cmd").is_string()){
            throw std::runtime_error("Type of json field not correct");
        }
        cmd.task = str_to_tasks.at(j.at("cmd"));
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

void get_tasks(const httplib::Request& req, httplib::Response& res){
    try{
        json json_body = json::parse(req.body);
        res.set_content(json_body.dump(), "application/json");
        Command cmd;
        int status = parse_task_json(json_body, cmd);
        if(status == 200){
            res.status = 200;
            res.set_content("Task received", "plain/text");
            for(auto arg: cmd.args){
                std::visit([](auto&& val){std::cout << val << "\n";}, arg);
            }
            cmd_q.add(cmd);
        }
    }
    catch(const std::exception& e){
        res.status = 400;
        res.set_content(std::string("\"error\":\"") + e.what() + "\"", "application/json");
    }
}

void http_server(int port){
    httplib::Server svr;

    // GET /hello
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("Wifisync http server's working", "text/plain");
    });

    svr.Post("/tasks", get_tasks);

    std::cout << "Server running on http://127.0.0.1:" << port << "\n";
    svr.listen("127.0.0.1", port); // блокирующий вызов
}

//-----------//
//Thread work//
//-----------//

void thread_worker(){
    std::thread client_thread;
    while(true){
        Command cmd = cmd_q.get();
        if(cmd.task == Tasks::Connect){

            if(cmd.args.size() != 2){
                throw std::runtime_error("Should be two args in connect function");
            }
            if(auto ip_ptr = std::get_if<std::string>(&cmd.args[0])){
                std::string ip = *ip_ptr;
                if(auto port_prt = std::get_if<int>(&cmd.args[1])){
                   int port = *port_prt;
                   client_thread = std::thread(client, port, ip);
                }
                else{
                    throw std::runtime_error("Port don't int");
                }
            }
            else{
                throw std::runtime_error("Ip don't string");
            }
        }
    }
    if (client_thread.joinable()) {
       client_thread.join();
    }
}

// int main() {
//     std::thread server_thread(server);
//     std::thread api_thread(http_server, 5000);
//     std::thread thread_work_thread(thread_worker);
    
//     api_thread.join();
//     thread_work_thread.join();
//     server_thread.join();
// }
