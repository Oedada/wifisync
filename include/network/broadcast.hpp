#pragma once

#include "network/TCPSocket.hpp"
#include "util/constants.hpp"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <map>
#include <net/if.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

enum class Message{
    Broadcast,
    ConnectRequest,
    ConnectResponse
};

enum class State {
    Discovering,
    Syncing
};

struct DeviceData{
    sockaddr_in addr;
    std::string name;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<long, std::ratio<1, 1000000000>>> last_seen;
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
        std::map<std::string, DeviceData> get_found_devices();
        
        ~UdpBroadcast();
        private:
            //messages
            std::string ConnectRequest;
            std::string ConnectResponse;
            std::string BroadcastMsg;
            std::map<std::string, DeviceData> found_devices;
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

class SessionInitializer{
    private:
        UdpBroadcast br;
        State state = State::Discovering;
        bool is_server;
        TCPSocket sock;
        int connect(std::string uuid);
        int create_tcp_connection(std::string uuid);
        std::map<std::string, DeviceData> found_devices;
    public:
        std::map<std::string, DeviceData> get_found_devices();
        SessionInitializer();
        void broadcast();
        void set_state(State st);
        std::pair<bool, std::string> check_incoming_connections();
        std::tuple<bool, TCPSocket, bool> accept_connection(std::string uuid);
        std::tuple<bool, TCPSocket, bool> connect_to(std::string uuid);
};
