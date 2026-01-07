#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <ios>
#include <fstream>
#include <vector>
#include <stdexcept>

class Client{
    public:
        const int server_port;
        const std::string server_ip;

        Client(int port,const std::string &ip) : server_port(port), server_ip(ip){
            client_sock = socket(AF_INET, SOCK_STREAM, 0);
            client_addr.sin_port = htons(server_port);
            client_addr.sin_family = AF_INET;
            inet_pton(AF_INET, server_ip.c_str(), &client_addr.sin_addr);
        }
        
        void connect_server(){
            if(::connect(client_sock, (sockaddr*)&client_addr, sizeof(client_addr)) != 0){throw std::runtime_error("Error with connect to server");}
        }

        void send(std::vector<char> &buf, size_t n){
            ::send(client_sock, buf.data(), n, 0);
        }

        ~Client(){
            ::close(client_sock);
        }

    private:
        int client_sock;
        sockaddr_in client_addr{};

};

int main(){
    Client ftr(12345, "192.168.0.101");
    ftr.connect_server();
    std::ifstream fin("test_out_text.txt", std::ios::binary);
    std::vector<char> buffer(8192);
    while(fin.read(buffer.data(), buffer.size()) || fin.gcount() > 0){
        ftr.send(buffer, fin.gcount());
    }
}
