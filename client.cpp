#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <ios>
#include <fstream>
#include <vector>

class Client{
    
}
int main(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_port = htons(12345);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.0.101", &addr.sin_addr);

    if(connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0) std::cout << "Connected succesfully \n";
    else std::cout << "Not connected";

    std::vector<char> buf(8192);
    std::ifstream fin("client_file.txt", std::ios::binary);

    while(fin.read(buf.data(), buf.size()) || fin.gcount() > 0){
        send(sock, buf.data(), fin.gcount(), 0);
    }

    close(sock);
}
