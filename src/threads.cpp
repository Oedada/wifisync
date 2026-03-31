// #include <chrono>
#include <iostream>
#include <openssl/rand.h>
#include "headers/server.hpp"
#include "headers/x25519.hpp"
#include "headers/ed25519.hpp"
#include <optional>
#include <stdexcept>
#include <unistd.h>
#include <variant>
#include "headers/TCPSocket.hpp"
#include <thread>
#include <nlohmann/json.hpp>
#include "headers/broadcast.hpp"
#include <fstream>
#include "headers/request_server.hpp"
#include "headers/constants.hpp"


using json = nlohmann::json;
using Arg = std::variant<bool, int, std::string>;

enum class Mode{
    Server,
    Client
};

struct ConnectData{
    Mode m;
    sockaddr_in addr;
};
SafeCmdQueue cmd_q;


TCPSocket create_connect(ConnectData data){
    if(data.m == Mode::Server){
        Server ftr(constants::TCP_PORT);
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
        TCPSocket s = client_connect(data.addr);
        return s;
    }
    else{
        throw std::runtime_error("Unkown mode");
    }
}

//-----------//
//Thread work//
//-----------//

TCPSocket broadcast_and_tcp_connect(ConnectData &d){
    UdpBroadcast br(PORT);
    while(!br.recieve()){
        br.send_broadcast();
        usleep(SLEEP_TIME);
    }
    if(br.is_own_ip_bigger()){
        d.m = Mode::Server;
    }
    else{
        d.m = Mode::Client;
    }
    d.addr = br.other_addr;
    TCPSocket sock = create_connect(d);
    return sock;
}


void thread_worker(){
    std::optional<TCPSocket> sock;
    while(true){
        tstypes::Command cmd = cmd_q.get();

        if(cmd.task == tstypes::Tasks::Connect){
            ConnectData d;
            sock = broadcast_and_tcp_connect(d);
            if(d.m == Mode::Client){
                std::cout << ">>>Role: Client\n";
                std::string msg = "Hello world";
                std::vector<char> vmsg(msg.begin(), msg.end());
                sock->smart_send_msg(vmsg);
                std::ofstream fout("data/test_send_file.txt");
                sock->recv_file(fout);
            }
            else{
                std::cout << ">>>Role: Server\n";
                std::vector<unsigned char> vmsg = sock->smart_recv_msg();
                std::string msg(vmsg.begin(), vmsg.end());
                std::cout << "Message: " << msg << "\n";
                std::ifstream fin("data/test_out_text.txt", std::ios::binary | std::ios::ate);
                uint64_t fsize = fin.tellg();
                fin.seekg(0);
                sock->send_file(fin, fsize);
            }
        }
    }
}

// int main() {
//     TaskServer ts(cmd_q);
//     std::thread api_thread([&ts](){return ts.start_server(5000);});
//     std::thread thread_work_thread(thread_worker);
    
//     api_thread.join();
//     thread_work_thread.join();
// }
