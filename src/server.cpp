#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <stdexcept>

class Server{
    public:
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        const int server_port;
        std::string client_ip;
        int client_port;
        Server(const int port) : server_port(port){
            server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if(server_fd == -1){throw std::runtime_error("Error with create new socket for server\n");}
            server_addr.sin_port = htons(server_port);
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;
        }

        void bind_sock(){
            int opt = 1;
            setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            if(::bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) != 0){
                throw std::runtime_error("Port not binded \n");
            }
        }

        void listen_addr(){
            if(::listen(server_fd, SOMAXCONN) != 0){
                throw std::runtime_error("Error with prepare on accept connections\n");
            }
        }

        void accept_conn(){
            client_fd = ::accept(server_fd, (sockaddr*)&client_addr, &client_addr_len);
            if(client_fd == -1){
                throw std::runtime_error("Error with accept connection\n");
            }
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, ip,sizeof(ip));
            client_ip = ip;
            client_port = ntohs(client_addr.sin_port);
        }

        ssize_t recv(std::vector<char> &buf){
            return ::recv(client_fd, buf.data(), buf.size(), 0);
        }

        ~Server(){
            if(client_fd != -1)
                ::close(client_fd);
            ::close(server_fd);
        }

    private:
        int server_fd;
        sockaddr_in server_addr{};
        int client_fd = -1;
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
    
};

int main(){
    Server ftr(12345);
    ftr.bind_sock();
    ftr.listen_addr();
    ftr.accept_conn();
    std::cout << ftr.client_ip << ":" << ftr.client_port << std::endl;
    ssize_t n;
    std::vector<char> buffer(4096);
    std::ofstream fout("data/out", std::ios::binary);
    while((n = ftr.recv(buffer)) > 0){
        fout.write(buffer.data(), n);
    }
    return 0;
}
