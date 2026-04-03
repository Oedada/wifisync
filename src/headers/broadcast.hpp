#pragma once
#include "constants.hpp"
#include <net/if.h>
#include <string>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <utility>

enum class Message{
    Broadcast,
    ConnectRequest,
    ConnectResponse
};


class UdpBroadcast{
    public:
        UdpBroadcast(int port);

        //new api
        void send(const Message &msg, sockaddr_in addr);
        std::pair<bool, std::string> recv(const enum Message &msg);
        bool is_own_ip_bigger(sockaddr_in addr);
        void read_received_data();
        sockaddr_in get_broadcast_addr();
        std::map<std::string, std::pair<sockaddr_in, std::string>> get_found_devices();
        
        ~UdpBroadcast();
        private:
            //messages
            std::string ConnectRequest;
            std::string ConnectResponse;
            std::string BroadcastMsg;
            std::map<std::string, std::pair<sockaddr_in, std::string>> found_devices;
            // private methods
            std::pair<bool, std::string> add_to_found_devices_if_not_own_uuid(char* buf, size_t size);
            std::pair<std::string, std::string> parse_msg(const char *buf, const int &size);
            void get_own_ip_legasy();
            bool get_own_and_brcast_addr(sockaddr_in &own_addr, sockaddr_in &baddr);
            bool is_ready_to_recv();
            void calculate_broadcast_addr(struct ifaddrs* ifa, sockaddr_in &baddr);
            bool is_suitable_interface_name(char *name);
            //received
            char recv_buf[constants::BUFFER_SIZE];
            sockaddr_in tmp_recv_addr{};
            size_t recv_size;
            //private addrs
            sockaddr_in own_addr{};
            sockaddr_in local_addr{};
            sockaddr_in broadcast_addr{};
            //private connect data
            int broadcast_port;
            int broadcast_sock = socket(AF_INET, SOCK_DGRAM, 0);
};
void print_ip(sockaddr_in addr);
