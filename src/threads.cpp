// #include <chrono>
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
#include "headers/TCPSocket.hpp"
#include <thread>
#include <mutex>
#include <queue>
#include "external_libs/cpp-httplib/httplib.h"
#include <nlohmann/json.hpp>
#include "headers/broadcast.hpp"
#include <fstream>


using json = nlohmann::json;
using Arg = std::variant<bool, int, std::string>;

enum class Tasks{
    Connect
};


struct Command{
    Tasks task;
    std::vector<Arg> args;
};

enum class Mode{
  Server,
  Client
};

struct ConnectData{
  Mode m;
  sockaddr_in addr;
};

int TCP_PORT = 12345;

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
    {"connect", Tasks::Connect}, 
};

TCPSocket create_connect(ConnectData data){
    if(data.m == Mode::Server){
        Server ftr(TCP_PORT);
        while(true){
        //TODO: добавить таймаут
            if(ftr.is_ready_to_accept()){
                TCPSocket s = ftr.accept_conn();
                // print_ip(ftr.client_addr);
                // print_ip(data.addr);
                if(ftr.client_addr.sin_addr.s_addr == data.addr.sin_addr.s_addr){
                    return s;
                }
                else{
                    std::cerr << "Ip doesn't correct";
                }
            }
        }
    }
    else if(data.m == Mode::Client){
        // print_ip(data.addr);
        TCPSocket s = client_connect(data.addr);
        return s;
    }
    else{
        throw std::runtime_error("Unkown mode");
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
                std::visit([](auto&& val){}, arg);
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

    std::cout << "Server running on http://0.0.0.0:" << port << "\n";
    svr.listen("0.0.0.0", port); // блокирующий вызов
}

//-----------//
//Thread work//
//-----------//

void thread_worker(){
    std::thread client_thread;
    UdpBroadcast br(PORT);
    while(true){
        Command cmd = cmd_q.get();
        if(cmd.task == Tasks::Connect){
            while(!br.recieve()){
                br.send_broadcast();
                usleep(SLEEP_TIME);
            }
            ConnectData d;
            if(br.is_own_ip_bigger()){
                d.m = Mode::Server;
            }
            else{
                d.m = Mode::Client;
            }
            d.addr = br.other_addr;
            TCPSocket sock = create_connect(d);
            if(d.m == Mode::Client){
                std::string msg = "Hello world";
                std::vector<char> vmsg(msg.begin(), msg.end());
                sock.smart_send_msg(vmsg);
                std::ofstream fout("data/test_send_file.txt");
                sock.smart_recv_file(fout);
            }
            else{
                std::vector<unsigned char> vmsg = sock.smart_recv_msg();
                std::string msg(vmsg.begin(), vmsg.end());
                std::cout << "Message: " << msg << "\n";
                std::ifstream fin("data/test_out_text.txt", std::ios::binary | std::ios::ate);
                uint64_t fsize = fin.tellg();
                fin.seekg(0);
                sock.smart_send_file(fin, fsize);
                
            }
        }
    }
    if (client_thread.joinable()) {
       client_thread.join();
    }
}

int main() {
    std::thread api_thread(http_server, 5000);
    std::thread thread_work_thread(thread_worker);
    
    api_thread.join();
    thread_work_thread.join();
}
