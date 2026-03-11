#include <iostream>
#include <fstream>
#include <openssl/rand.h>
#include <vector>
#include "headers/hash.hpp"
#include "headers/files_opers.hpp"
#include "headers/client.hpp"
#include "headers/server.hpp"
#include "headers/x25519.hpp"
#include "headers/ed25519.hpp"
#include "headers/TCPSocket.hpp"
#include <thread>
#include <mutex>
#include "external_libs/cpp-httplib/httplib.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int counter = 0;
std::mutex m;

void server() {
    X25519 server;
    std::vector<char> pub_key_vector(
        std::begin(server.pub_key),
        std::end(server.pub_key)
    );
    std::cout << "Server pubkey: " << "\n";
    for(int i = 0; i < 32; i++){
        std::cout << server.pub_key[i];
    }
    std::cout << "\n";
    Server ftr(12345);
    TCPSocket s = ftr.accept_conn();
    std::cout << ftr.client_ip << ":" << ftr.client_port << std::endl;
    s.smart_send(pub_key_vector);
    std::ofstream fout("data/client_pub_key.pem", std::ios::binary);
    std::vector<unsigned char> client_pub_key = s.smart_recv();
    fout.write(reinterpret_cast<char*>(client_pub_key.data()), client_pub_key.size());
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "Server working" << "\n";
    }
}

void client(const int server_port,const std::string &server_ip){
    X25519 client;
    std::vector<char> pub_key_vector(
        std::begin(client.pub_key),
        std::end(client.pub_key)
    );
    std::cout << "Client pubkey: " << "\n";
    for(int i = 0; i < 32; i++){
        std::cout << pub_key_vector[i];
    }
    std::cout << "\n";
    Client ftr(server_port, server_ip);
    TCPSocket s = ftr.connect_server();
    s.smart_send(pub_key_vector);
    std::ofstream fout("data/server_pub_key.pem", std::ios::binary);
    std::vector<unsigned char> client_pub_key = s.smart_recv();
    fout.write(reinterpret_cast<char*>(client_pub_key.data()), client_pub_key.size());

}

void get_requests(int port){
    httplib::Server svr;

    // GET /hello
    svr.Get("/hello", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("Hello from C++ server!", "text/plain");
    });

    std::cout << "Server running on http://localhost:" << port << "\n";
    svr.listen("127.0.0.1", port); // блокирующий вызов
}

int main() {

    std::thread server_thread(server);
    std::thread client_thread(client, 12345, "192.168.0.104");
    std::thread api_thread(get_requests, 5000);

    api_thread.join();
    server_thread.join();
    client_thread.join();

    std::cout << counter << "\n";
}
