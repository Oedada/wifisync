#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>

int main(){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_port = htons(12345);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if(bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == 0) std::cout << "Bind succesfully \n";
    else std::cout << "Not binded \n";
    listen(server_fd, 1);

    std::cout << "Начиниаю принимать соединения... \n";
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_addr_len);
    std::cout << "Соединение установлено\n";

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip,sizeof(ip));

    std::cout << "Client addres: " << ip << ":" << ntohs(client_addr.sin_port);
    std::cout << std::endl;


    ssize_t n;
    std::vector<char> buf(8192);
    std::ofstream fout("out", std::ios::binary);
    while((n = recv(client_fd, buf.data(), buf.size(), 0)) > 0){
        fout.write(buf.data(), n);
    }
    close(client_fd);
    close(server_fd);
    return 0;
}
