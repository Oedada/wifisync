#include <iostream>
#include <fstream>
#include <filesystem>
#include "headers/hash.hpp"
#include "headers/files_opers.hpp"
#include "headers/client.hpp"
#include "headers/server.hpp"
#include "headers/x25519.hpp"
#include "headers/ed25519.hpp"

using json = nlohmann::json;

void test_hash(){
    Hash h;
    std::string hello = "hello";
    std::vector<unsigned char> msg(hello.begin(), hello.end());
    h.update(msg);
    h.calculate();
    std::cout << h.to_hex();
}

void test_files(){
    Units plugins("data/files.json");
    plugins.set_unit("data/train", true);
    plugins.rm_unit("data/train/HiddenArmor");
    std::cout << plugins.is_registred("data/train/lol33");
    std::cout << plugins.is_registred("data/train/spark");
    json configs;
    plugins.get_unit("data/train", configs);
    std::string json_string = configs.dump();
    std::ofstream fout("data/test.json");
    fout.write(json_string.c_str(), json_string.size());
}

void test_client(){
    Client ftr(12345, "192.168.0.101");
    ftr.connect_server();
    std::ifstream fin("data/test_out_text.txt", std::ios::binary);
    std::vector<char> buffer(8192);
    while(fin.read(buffer.data(), buffer.size()) || fin.gcount() > 0){
        ftr.send(buffer, fin.gcount());
    }
}

void test_server(){
    Server ftr(12345);
    ftr.bind_sock();
    ftr.listen_addr();
    ftr.accept_conn();
    std::cout << ftr.client_ip << ":" << ftr.client_port << std::endl;
    ssize_t n;
    std::vector<char> buffer(4096);
    std::ofstream fout("data/out", std::ios::binary);
    while((n = ftr.receive(buffer)) > 0){
        fout.write(buffer.data(), n);
    }
}

void test_x25519(){
    X25519 client;
    X25519 server;
    client.calculate_secret(server.pub_key);
    server.calculate_secret(client.pub_key);
    std::cout << equal_secrets(server.secret, client.secret, server.secret_len) << "\n";
}

void test_ed25519(){
    Ed25519 test("data/keys");
    unsigned char lol[] = "lol";
    test.sign(lol, 3);
    std::cout << check_sig("data/keys/ed25519_pub.pem", lol, 3, test.sig, 64);
}

int main(){
    std::cout << "Working" << "\n";
    X25519 client;
    X25519 server;
    client.calculate_secret(server.pub_key);
    server.calculate_secret(client.pub_key);
    std::cout << equal_secrets(server.secret, client.secret, server.secret_len) << "\n";
    return 0;
}
