#include <net/if.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

const int PORT = 12345;
const int SLEEP_TIME = 200000;

class UdpBroadcast{
    public:
        sockaddr_in other_addr{};
        UdpBroadcast(int p);
        void send_broadcast();

        bool is_own_ip_bigger();
        bool recieve();

        ~UdpBroadcast();
    private:
        void get_own_ip();
        bool get_own_and_brcast_addr(sockaddr_in &own_addr, sockaddr_in &baddr);
        bool is_ready_to_recv();
        void calculate_broadcast_addr(struct ifaddrs* ifa, sockaddr_in &baddr);
        bool is_suitable_interface_name(char *name);
        const char* Message;
        int broadcast_sock = socket(AF_INET, SOCK_DGRAM, 0);
        sockaddr_in own_addr{};
        sockaddr_in local_addr{};
        int broadcast_port;
        sockaddr_in broadcast_addr{};
};
void print_ip(sockaddr_in addr);
