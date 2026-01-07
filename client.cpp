#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

int main(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_port = htons(12345);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.0.101", &addr.sin_addr);

    if(connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0) std::cout << "Connected succesfully \n";
    else std::cout << "Not connected";

    std::string msg = "Hi from client";
    send(sock, msg.c_str(), msg.size(), 0);

    close(sock);
}
