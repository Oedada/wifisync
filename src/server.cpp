#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include "headers/server.hpp"


        Server::Server(const int port) : server_port(port){
            server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if(server_fd == -1){throw std::runtime_error("Error with create new socket for server\n");}
            server_addr.sin_port = htons(server_port);
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;
        }

        void Server::bind_sock(){
            int opt = 1;
            setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            if(::bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) != 0){
                throw std::runtime_error("Port not binded \n");
            }
        }

        void Server::listen_addr(){
            if(::listen(server_fd, SOMAXCONN) != 0){
                throw std::runtime_error("Error with prepare on accept connections\n");
            }
        }

        bool Server::is_ready_to_accept(){
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server_fd, &readfds);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;
            
            int ret = select(server_fd + 1, &readfds, nullptr, nullptr, &timeout);

            if(ret > 0 && FD_ISSET(server_fd, &readfds)){
                return true;
            }
            return false;
        }

        TCPSocket Server::accept_conn(){
            bind_sock();
            listen_addr();
            client_fd = ::accept(server_fd, (sockaddr*)&client_addr, &client_addr_len);
            if(client_fd == -1){
                throw std::runtime_error("Error with accept connection\n");
            }
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, ip,sizeof(ip));
            client_ip = ip;
            client_port = ntohs(client_addr.sin_port);
            return TCPSocket(client_fd);
        }

        Server::~Server(){
            if (server_fd >= 0)
                ::close(server_fd);
        }

