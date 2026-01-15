#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <stdexcept>
#include "headers/client.hpp"

    Client::Client(const int port,const std::string &ip) : server_port(port), server_ip(ip){
            client_sock = socket(AF_INET, SOCK_STREAM, 0);
            client_addr.sin_port = htons(server_port);
            client_addr.sin_family = AF_INET;
            inet_pton(AF_INET, server_ip.c_str(), &client_addr.sin_addr);
        }
        
    void Client::connect_server(){
        if(::connect(client_sock, (sockaddr*)&client_addr, sizeof(client_addr)) != 0){
            throw std::runtime_error("Error with connect to server");
        }
    }

    void Client::send(const std::vector<char> &buf, const size_t size){
        ::send(client_sock, buf.data(), size, 0);
    }

    Client::~Client(){
        ::close(client_sock);
    }
