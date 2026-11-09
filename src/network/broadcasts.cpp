#include "fs/data_manager.hpp"
#include "network/broadcast.hpp"
#include "util/constants.hpp"
#include "util/utils.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstddef>
#include <ifaddrs.h>
#include <iostream>
#include <net/if.h>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

using steady = std::chrono::steady_clock;

UdpBroadcast::UdpBroadcast(int port) : broadcast_port(port) {
    if (broadcast_sock < 0) {
        throw std::runtime_error("Can't create socket");
    }
    std::string uuid_and_name = DataManager::get_uuid() + ":" + DataManager::get_name();
    BroadcastMsg = constants::StaticBroadcastMessage + uuid_and_name;
    // get own addr and broadcast addr
    if (!get_own_and_brcast_addr(own_addr, broadcast_addr)) {
        get_own_ip_legasy();
        inet_pton(AF_INET, constants::GLOBAL_BROADCAST_IP,
                  &broadcast_addr.sin_addr);
    }
    // broadcast addr
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(broadcast_port);
    // local addr
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(broadcast_port);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    // for binding two sockets on one addr
    int enable = 1;
    if (setsockopt(broadcast_sock, SOL_SOCKET, SO_REUSEADDR, &enable,
                   sizeof(enable)) < 0) {
        throw std::runtime_error("Can't set SO_REUSEADDR");
    }
    // bind
    if (bind(broadcast_sock, (sockaddr *)&local_addr, sizeof(local_addr)) !=
        0) {
        throw std::runtime_error("Can't bind broadcast sock to addr");
    }
    // enable broadcast
    enable = 1;
    setsockopt(broadcast_sock, SOL_SOCKET, SO_BROADCAST, &enable,
               sizeof(enable));
}

std::map<std::string, DeviceData> UdpBroadcast::get_devices() {
    return found_devices.get_all();
}

bool UdpBroadcast::check_recv(int timeout) {
    std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();
    pollfd pfd{.fd = broadcast_sock, .events = POLLIN, .revents = 0};
    int ret = poll(&pfd, 1, timeout);
    if (ret > 0) {
        if (pfd.revents & (POLLERR | POLLNVAL | POLLHUP)) {
            throw std::runtime_error("Udp socket error");
        } else if (pfd.revents & POLLIN) {
            return true;
        } else {
            return false;
        }
    } else if (ret == 0) {
        return false;
    } else {
        int error = errno;
        if (error == EINTR) {
            return check_recv(
                timeout - std::chrono::duration_cast<std::chrono::milliseconds>(
                              steady::now() - start)
                              .count());
        } else {
            throw std::runtime_error("Poll Udp socket error");
        }
    }
}

bool UdpBroadcast::recv_msg(const int &timeout, sockaddr_in &addr,
                            std::string &msg) {
    std::vector<std::byte> buf;
    sockaddr_in sender{};
    socklen_t sender_size = sizeof(sender);
    buf.resize(constants::BUFFER_SIZE);
    bool ret = check_recv(timeout);
    if (!ret) {
        return false;
    }
    ssize_t recieved =
        recvfrom(broadcast_sock, buf.data(), buf.size(), 0,
                 reinterpret_cast<sockaddr *>(&sender), &sender_size);
    if (recieved == -1) {
        // обработать ошибку
        throw std::runtime_error(
            "lOl unprocessed error on UdpBroadcast::process, haha");
    } else if (recieved == 0) {
        addr = sender;
        msg = "";
    } else {
        addr = sender;
        // TODO: не самая надёжная схема читать строку из сырых байтов, может и
        // не сработать, но нужно ли это фиксить - вопрос
        msg = std::string(buf.begin(), buf.begin() + recieved);
    }
    return true;
}

void UdpBroadcast::process() {
    std::chrono::steady_clock::time_point last_send =
        std::chrono::steady_clock::now();
    steady::time_point nearest_expseen =
        steady::now() +
        std::chrono::milliseconds(constants::EXPIRE_DEVICE_MILLISECOND_TIME);
    while (true) {
        auto keys = found_devices.keys();
        for (auto key : keys) {
            auto device = found_devices.get(key).value_or(DeviceData{
                .addr = sockaddr_in{},
                .name = "this isn't normal",
                .last_seen =
                    (steady::now() -
                     std::chrono::seconds(
                         constants::EXPIRE_DEVICE_MILLISECOND_TIME + 1))});
            if (std::chrono::steady_clock::now() - device.last_seen >
                std::chrono::seconds(constants::TIMEOUT_TIME)) {
                found_devices.erase(key);
            }
            nearest_expseen =
                std::min(nearest_expseen, device.last_seen);
        }
        if (steady::now() >=
            last_send + std::chrono::milliseconds(
                            constants::SEND_MILLISECOND_INTERVAL)) {
            send_msg(BroadcastMsg);
            last_send = steady::now();
        }
        nearest_expseen += std::chrono::milliseconds(
            constants::EXPIRE_DEVICE_MILLISECOND_TIME);
        steady::time_point next_event =
            std::min(nearest_expseen,
                     last_send + std::chrono::milliseconds(
                                     constants::SEND_MILLISECOND_INTERVAL));
        sockaddr_in addr{};
        std::string msg, uuid, name;
        bool msg_received =
            recv_msg(std::chrono::duration_cast<std::chrono::milliseconds>(
                         next_event - steady::now())
                         .count(),
                     addr, msg);
        if (msg_received) {
            bool is_valid_msg = check_msg(msg, uuid, name);
            if (is_valid_msg) {
                if (!found_devices.contains(uuid)) {
                    if (uuid != DataManager::get_uuid()) {
                        found_devices.set(
                            uuid,
                            DeviceData{.addr = addr,
                                       .name = name,
                                       .last_seen =
                                           std::chrono::steady_clock::now()});
                    }
                } else {
                    found_devices.set(uuid,
                                      DeviceData{.addr = addr,
                                                 .name = name,
                                                 .last_seen = steady::now()});
                }
            }
        }
    }
}

void UdpBroadcast::send_msg(const std::string &msg) {
    auto sent = sendto(broadcast_sock, msg.data(), msg.size(), 0,
                       reinterpret_cast<sockaddr *>(&broadcast_addr),
                       sizeof(broadcast_addr));
    if (sent < 0) {
        int error = errno;
        switch (error) {
        case EBADF:
        case EFAULT:
        case EINVAL:
            throw std::runtime_error("Lol kek some error in send udp socket");
        }
    }
}

bool UdpBroadcast::check_msg(const std::string &msg, std::string &uuid,
                             std::string &name) {
    std::stringstream ss(msg);
    std::string hello;
    std::string luuid;
    std::string lname;
    std::getline(ss, hello, ':');
    std::getline(ss, luuid, ':');
    std::getline(ss, lname, ':');
    if (hello + ':' == constants::StaticBroadcastMessage) {
        uuid = luuid;
        name = lname;
        return true;
    }
    return false;
}

sockaddr_in UdpBroadcast::get_broadcast_addr() { return broadcast_addr; }

bool UdpBroadcast::is_own_ip_bigger(sockaddr_in addr) {
    return own_addr.sin_addr.s_addr > addr.sin_addr.s_addr;
}

UdpBroadcast::~UdpBroadcast() { close(broadcast_sock); }

//----------//
// Utilites //
//----------//
bool UdpBroadcast::is_ready_to_recv() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(broadcast_sock, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = constants::SOCKET_CHECK_TIMEOUT;

    int ret = select(broadcast_sock + 1, &readfds, nullptr, nullptr, &timeout);

    if (ret < 0) {
        perror("Error: select");
        return false;
    }
    if (ret > 0 && FD_ISSET(broadcast_sock, &readfds)) {
        return true;
    }
    return false;
}

//----------------//
// Ip calculating //
//----------------//
bool UdpBroadcast::is_suitable_interface_name(char *name) {
    std::string n = name;
    return !(n.find("lo") != std::string::npos ||
             n.find("tun") != std::string::npos ||
             n.find("docker") != std::string::npos ||
             n.find("veth") != std::string::npos ||
             n.find("zt") != std::string::npos ||
             n.find("br-") != std::string::npos);
}

bool UdpBroadcast::get_own_and_brcast_addr(sockaddr_in &own_addr,
                                           sockaddr_in &baddr) {
    struct ifaddrs *ifaddr;

    if (getifaddrs(&ifaddr) == -1) {
        throw std::runtime_error("Can't get ifaddr");
    }
    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || !is_suitable_interface_name(ifa->ifa_name) ||
            !ifa->ifa_netmask || ifa->ifa_addr->sa_family != AF_INET ||
            !(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_flags & IFF_BROADCAST))
            continue;
        baddr = *reinterpret_cast<sockaddr_in *>(ifa->ifa_broadaddr);
        own_addr = *reinterpret_cast<sockaddr_in *>(ifa->ifa_addr);
        freeifaddrs(ifaddr);
        return true;
    }
    freeifaddrs(ifaddr);
    return false;
}

void UdpBroadcast::get_own_ip_legasy() {
    int tmp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in tmp_addr{};
    tmp_addr.sin_family = AF_INET;
    tmp_addr.sin_port = htons(53);
    inet_pton(AF_INET, constants::PING_CONNECT_TEST_IP, &tmp_addr.sin_addr);

    if (connect(tmp_sock, (sockaddr *)&tmp_addr, sizeof(tmp_addr)) != 0) {
        inet_pton(AF_INET, constants::TEST_NET_IP, &tmp_addr.sin_addr);
        if (connect(tmp_sock, (sockaddr *)&tmp_addr, sizeof(tmp_addr)) != 0) {
            throw std::runtime_error("Can't build route for know own ip");
        }
    }
    socklen_t own_addr_size = sizeof(own_addr);
    getsockname(tmp_sock, (sockaddr *)&own_addr, &own_addr_size);
    close(tmp_sock);
}

void print_ip(sockaddr_in addr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    std::cout << " IP: " << ip << std::endl;
}
