#pragma once

#include "concurrency/safe_dict.hpp"
#include "fs/data_manager.hpp"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

struct DeviceData {
    sockaddr_in addr;
    std::string name;
    std::chrono::steady_clock::time_point last_seen;
};

class UdpBroadcast {
  public:
    UdpBroadcast(int port);

    void process();
    bool is_own_ip_bigger(sockaddr_in addr);
    sockaddr_in get_broadcast_addr();

    ~UdpBroadcast();

  private:
    DataManager dm;
    // messages
    std::string BroadcastMsg;
    SafeDict<DeviceData> found_devices;
    // private methods
    bool check_recv(int timeout);
    bool check_msg(const std::string &msg, std::string &uuid,
                   std::string &name);
    bool recv_msg(const int &timeout, sockaddr_in &addr, std::string &msg);
    void send_msg(const std::string &msg);
    std::pair<bool, std::string>
    add_to_found_devices_if_not_own_uuid(char *buf, size_t size);
    void get_own_ip_legasy();
    bool get_own_and_brcast_addr(sockaddr_in &own_addr, sockaddr_in &baddr);
    bool is_ready_to_recv();
    bool is_suitable_interface_name(char *name);
    // private addrs
    sockaddr_in own_addr{};
    sockaddr_in local_addr{};
    sockaddr_in broadcast_addr{};
    // private connect data
    int broadcast_port;
    int broadcast_sock = socket(AF_INET, SOCK_DGRAM, 0);
};
void print_ip(sockaddr_in addr);
