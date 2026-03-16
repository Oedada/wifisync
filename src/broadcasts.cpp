#include <iostream>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


const char MagicMessage[] = "Wifisync Hello World";

class UdpBroadcast{
    public:
        int port;
        int broadcast_sock;
        sockaddr_in addr{};
        UdpBroadcast(int p) : port(p){
            broadcast_sock = socket(AF_INET, SOCK_DGRAM, 0);
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            std::string broadcast_addr = "255.255.255.255";
            inet_pton(AF_INET, broadcast_addr.c_str(), &addr.sin_addr);
            if(bind(broadcast_sock, (sockaddr*)&addr, sizeof(addr)) != 0){
                throw std::runtime_error("Can't bind broadcast sock to addr");
            }
            int enable = 1;
            setsockopt(broadcast_sock, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));
        }
        void send_msg(){
            sendto(broadcast_sock, MagicMessage, strlen(MagicMessage), 0,(sockaddr*)&addr, sizeof(addr));
        }

        void recieve(){
            char buf[1024];
            sockaddr_in sender{};
            socklen_t sender_len = sizeof(sender);

            ssize_t n = recvfrom(broadcast_sock, buf, sizeof(buf)-1, 0,(sockaddr*)&sender, &sender_len);
            if (n > 0) {
                buf[n] = '\0';
                if(std::strcmp(MagicMessage, buf) == 0){
                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &sender.sin_addr, ip, sizeof(ip));

                    std::cout << "Sender IP: " << ip << std::endl;
                    std::cout << "Sender Port: " << ntohs(sender.sin_port) << std::endl;
                }
                else{
                    std::cout << "Wrong" << "\n";
                }
            }
        }
        ~UdpBroadcast(){ 
            close(broadcast_sock);
        }
};


int main(int argc, char** argv) {
    UdpBroadcast b(12345);
    bool server = false;
    if(argc > 1){
        if(argv[1][0] == 's'){
            server = true;
        }
    }
    if(!server){
        b.send_msg();
    }
    else{
        b.recieve();
    }
}
