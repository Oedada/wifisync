#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include "TCPSocket.hpp"


class Client{
    public:
        const int server_port;
        const std::string server_ip;
        Client(const int port,const std::string &ip);
        TCPSocket connect_server();

    private:
        int client_sock;
        sockaddr_in client_addr;

};
