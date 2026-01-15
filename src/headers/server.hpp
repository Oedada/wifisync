#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <string>

class Server{
    public:
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        const int server_port;
        std::string client_ip;
        int client_port;

        Server(const int port);
        void bind_sock();
        void listen_addr();
        void accept_conn();
        ssize_t receive(std::vector<char> &buf);
        ~Server();

    private:
        int server_fd;
        sockaddr_in server_addr{};
        int client_fd = -1;
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
    
};
