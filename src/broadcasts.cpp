#include <cstddef>
#include <cstdint>
#include <net/if.h>
#include <ifaddrs.h>
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


const char MagicMessage[] = "Wifisync Hello Wifi";
const char MagicResponse[] = "Wifisync Response Wifi";
const int PORT = 12345;
const int SLEEP_TIME = 200000;

class UdpBroadcast{
    public:
        int broadcast_port;
        sockaddr_in broadcast_addr{};
        sockaddr_in other_addr{};
        sockaddr_in own_addr{};
        sockaddr_in local_addr{};
        int broadcast_sock = socket(AF_INET, SOCK_DGRAM, 0);
        UdpBroadcast(int p) : broadcast_port(p){
            // get own addr and broadcast addr
            get_own_and_brcast_addr(own_addr, broadcast_addr);
            // broadcast addr
            broadcast_addr.sin_family = AF_INET;
            broadcast_addr.sin_port = htons(broadcast_port);
            //local addr
            local_addr.sin_family = AF_INET;
            local_addr.sin_port = htons(broadcast_port);
            local_addr.sin_addr.s_addr = INADDR_ANY;
            //bind
            if(bind(broadcast_sock, (sockaddr*)&local_addr, sizeof(local_addr)) != 0){
                throw std::runtime_error("Can't bind broadcast sock to addr");
            }
            //enable broadcast
            int enable = 1;
            setsockopt(broadcast_sock, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));
        }
        void send_msg(const char* msg, size_t s){
            sendto(broadcast_sock, msg, s, 0,(sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
        }

        bool is_suitable_interface_name(char *name){
            std::string n = name;
            return !(n.find("lo") != std::string::npos ||
                    n.find("tun") != std::string::npos ||
                    n.find("docker") != std::string::npos ||
                    n.find("veth") != std::string::npos ||
                    n.find("zt") != std::string::npos ||
                    n.find("br-") != std::string::npos);
        }

        void print_ip(sockaddr_in addr){
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
            std::cout << " IP: " << ip << std::endl;
        }

        void calculate_broadcast_addr(struct ifaddrs* ifa, sockaddr_in &baddr){
            auto* addr = (struct sockaddr_in*)ifa->ifa_addr;
            auto* mask = (struct sockaddr_in*)ifa->ifa_netmask;
            uint32_t ip_addr = addr->sin_addr.s_addr;
            uint32_t net_mask = mask->sin_addr.s_addr;
            uint32_t broadcast = ip_addr | ~net_mask;
            baddr.sin_addr.s_addr = broadcast;
        }

        bool get_own_and_brcast_addr(sockaddr_in &own_addr, sockaddr_in &baddr){
            struct ifaddrs* ifaddr;

            if(getifaddrs(&ifaddr) == -1){
                throw std::runtime_error("Can't get ifaddr");
            }
            for(struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next){
                if (!ifa->ifa_addr) continue;
                if(!is_suitable_interface_name(ifa->ifa_name)){
                    continue;
                }
                if (ifa->ifa_addr->sa_family != AF_INET) continue;
                if(!(ifa->ifa_flags & IFF_UP)) continue;
                calculate_broadcast_addr(ifa, baddr);
                own_addr = *(sockaddr_in*)ifa->ifa_addr;
                freeifaddrs(ifaddr);
                return true;
            }
            freeifaddrs(ifaddr);
            return false;
        }

        void get_own_ip(){
            int tmp_sock = socket(AF_INET, SOCK_DGRAM, 0);
            sockaddr_in tmp_addr{};
            tmp_addr.sin_family = AF_INET;
            tmp_addr.sin_port = htons(53);
            inet_pton(AF_INET, "8.8.8.8", &tmp_addr.sin_addr);

            if(connect(tmp_sock, (sockaddr*)&tmp_addr, sizeof(tmp_addr)) != 0){
                inet_pton(AF_INET, "192.0.2.1", &tmp_addr.sin_addr);
                if(connect(tmp_sock, (sockaddr*)&tmp_addr, sizeof(tmp_addr)) != 0){
                    throw std::runtime_error("Can't build route for know own ip");
                }
            }
            socklen_t own_addr_size = sizeof(own_addr); 
            getsockname(tmp_sock, (sockaddr*)&own_addr, &own_addr_size);
            close(tmp_sock);
        }

        bool is_ready_to_recv(){
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(broadcast_sock, &readfds);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 200000;
            
            int ret = select(broadcast_sock + 1, &readfds, nullptr, nullptr, &timeout);

            if(ret > 0 && FD_ISSET(broadcast_sock, &readfds)){
                return true;
            }
            return false;
        }

        bool recieve(){
            if(is_ready_to_recv()){
                char buf[1024];
                sockaddr_in tmp_addr;
                socklen_t sender_len = sizeof(tmp_addr);
                ssize_t n = recvfrom(broadcast_sock, buf, sizeof(buf)-1, 0,(sockaddr*)&tmp_addr, &sender_len);
                if (n > 0) {
                    if(tmp_addr.sin_addr.s_addr != own_addr.sin_addr.s_addr){
                        if(n == strlen(MagicMessage) && memcmp(buf, MagicMessage, strlen(MagicMessage)) == 0){
                                other_addr = tmp_addr;
                                sendto(broadcast_sock, MagicResponse, strlen(MagicResponse), 0,(sockaddr*)&other_addr, sizeof(other_addr));
                                return true;
                        }
                        else if(n == strlen(MagicResponse) && memcmp(buf, MagicResponse, strlen(MagicResponse)) == 0){
                                other_addr = tmp_addr;
                                return true;
                        }
                    }
                }
            }
            return false;
        }

        ~UdpBroadcast(){ 
            close(broadcast_sock);
        }
};


int main() {
    UdpBroadcast b(PORT);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &b.own_addr.sin_addr, ip, sizeof(ip));
    std::cout << ip << "\n";
    while(!b.recieve()){
        b.send_msg(MagicMessage, strlen(MagicMessage));
        usleep(SLEEP_TIME);
    }
    inet_ntop(AF_INET, &b.other_addr.sin_addr, ip, sizeof(ip));
    std::cout << "Other Ip: " << ip << "\n";
    std::cout << "Other Port: " << b.other_addr.sin_port << "\n";
}
