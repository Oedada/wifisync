#include <vector>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>

class Client{
    public:
        const int server_port;
        const std::string server_ip;
        Client(const int port,const std::string &ip);
        void connect_server();
        void send(const std::vector<char> &buf, const size_t size);
        ~Client();

    private:
        int client_sock;
        sockaddr_in client_addr;

};
