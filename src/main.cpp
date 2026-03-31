#include <iostream>
#include <fstream>
#include <filesystem>
#include <netinet/in.h>
#include <openssl/rand.h>
#include <sys/socket.h>
#include <vector>
#include "headers/hash.hpp"
#include "headers/files_opers.hpp"
#include "headers/server.hpp"
#include "headers/x25519.hpp"
#include "headers/ed25519.hpp"
#include "headers/TCPSocket.hpp"
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
    sockaddr_in caddr;
    caddr.sin_family = AF_INET;
    caddr.sin_port = 12345;
    inet_pton(AF_INET, "192.168.0.101", &caddr.sin_addr);
    TCPSocket s = client_connect(caddr);
    std::ifstream fin("data/test_out_text.txt", std::ios::binary);
    std::vector<char> buffer(8192);
}

void test_server(){
    Server ftr(12345);
    TCPSocket s = ftr.accept_conn();
    std::cout << ftr.client_ip << ":" << ftr.client_port << std::endl;
    ssize_t n = 1;
    std::ofstream fout("data/out", std::ios::binary);
    std::vector<unsigned char>buffer = s.smart_recv_msg();
    while(true){
        fout.write(reinterpret_cast<char*>(buffer.data()), n);
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

std::vector<unsigned char> gen_random_bytes(size_t len) {
    std::vector<unsigned char> buf(len);
    if(RAND_bytes(buf.data(), static_cast<int>(len)) != 1) {
        throw std::runtime_error("Failed to generate random bytes");
    }
    return buf;
}

void test_x_ed_25519(){
    X25519 client;
    X25519 server;
    client.calculate_secret(server.pub_key);
    server.calculate_secret(client.pub_key);
    std::cout << "Xsecrets is equal: " <<  equal_secrets(server.secret, client.secret, server.secret_len) << "\n";
    Ed25519 test("data/keys");
    std::vector<unsigned char> msg;
    msg.insert(msg.end(), std::begin(server.pub_key), std::end(server.pub_key));
    msg.insert(msg.end(), std::begin(client.pub_key), std::end(client.pub_key));
    std::vector<unsigned char> randc = gen_random_bytes(32);
    std::vector<unsigned char> rands = gen_random_bytes(32);
    msg.insert(msg.end(), randc.begin(), randc.end());
    msg.insert(msg.end(), rands.begin(), rands.end());
    test.sign(msg.data(), msg.size());
    std::cout << check_sig("data/keys/ed25519_pub.pem", msg.data(), msg.size(), test.sig, 64);
}

void test_x_ed_25519_file_trans(int argc, char** argv){
    bool server = false;
    if(argc > 1){
        if(argv[1][0] == 's'){
            server = true;
        }
    }
    if(server){
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
        s.smart_send_msg(pub_key_vector);
        std::ofstream fout("data/client_pub_key.pem", std::ios::binary);
        std::vector<unsigned char> client_pub_key = s.smart_recv_msg();
        fout.write(reinterpret_cast<char*>(client_pub_key.data()), client_pub_key.size());
    } else{
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
        sockaddr_in caddr;
        caddr.sin_family = AF_INET;
        caddr.sin_port = 12345;
        inet_pton(AF_INET, "ip", &caddr.sin_addr);
        TCPSocket s = client_connect(caddr);
        s.smart_send_msg(pub_key_vector);
        std::ofstream fout("data/server_pub_key.pem", std::ios::binary);
        std::vector<unsigned char> client_pub_key = s.smart_recv_msg();
        fout.write(reinterpret_cast<char*>(client_pub_key.data()), client_pub_key.size());
    }
}




// int main(int argc, char** argv){
//     std::cout << "Working" << "\n";
//     Units dir("data/test.json");
//     dir.set_unit("data/train", true);
//     json file_tree = dir.json_tree;
    
//     return 0;
// }
