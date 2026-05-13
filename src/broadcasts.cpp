#include <cstdint>
#include <net/if.h>
#include <ifaddrs.h>
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <chrono>
#include "TCPSocket.hpp"
#include "broadcast.hpp"
#include "constants.hpp"
#include "environment.hpp"
#include "server.hpp"
#include "utils.hpp"

UdpBroadcast::UdpBroadcast(int p) : broadcast_port(p){
    if(broadcast_sock < 0){
        throw std::runtime_error("Can't create socket");
    }
    std::string uuid_and_name = env::get_uuid() + ":" + env::get_name();
    BroadcastMsg = constants::StaticBroadcastMessage + uuid_and_name;
    ConnectRequest = constants::StaticRequestConnect + uuid_and_name;
    ConnectResponse = constants::StaticResponseConnect + uuid_and_name;
    // get own addr and broadcast addr
    if(!get_own_and_brcast_addr(own_addr, broadcast_addr)){
        get_own_ip_legasy();
        inet_pton(AF_INET, constants::GLOBAL_BROADCAST_IP, &broadcast_addr.sin_addr); 
    }
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

void UdpBroadcast::read_received_data(){
    if(is_ready_to_recv()){
        char buf[constants::BUFFER_SIZE];
        sockaddr_in tmp_addr;
        socklen_t sender_len = sizeof(tmp_addr);
        ssize_t n;
        while(is_ready_to_recv()){
            n = recvfrom(broadcast_sock, buf, sizeof(buf)-1, 0,(sockaddr*)&tmp_addr, &sender_len);
        }
        if(n > 0){
            recv_size = n;
            memcpy(recv_buf, buf, constants::BUFFER_SIZE);
            tmp_recv_addr = tmp_addr;
        }
    }
}

void UdpBroadcast::send(const enum Message &msg, sockaddr_in addr){
    switch (msg) {
        case(Message::Broadcast):
            sendto(broadcast_sock, BroadcastMsg.data(), BroadcastMsg.size(), 0,(sockaddr*)&addr, sizeof(addr));
            break;
        case(Message::ConnectRequest):
            sendto(broadcast_sock, ConnectRequest.data(), ConnectRequest.size(), 0,(sockaddr*)&addr, sizeof(addr));
            break;
        case(Message::ConnectResponse):
            sendto(broadcast_sock, ConnectResponse.data(), ConnectResponse.size(), 0,(sockaddr*)&addr, sizeof(addr));
            break;
    }
}

std::pair<bool, std::string> UdpBroadcast::recv(const enum Message &msg){
    if(tmp_recv_addr.sin_addr.s_addr != own_addr.sin_addr.s_addr){
        switch (msg) {
        case(Message::Broadcast):
            if(recv_size > strlen(constants::StaticBroadcastMessage) && memcmp(recv_buf, constants::StaticBroadcastMessage, strlen(constants::StaticBroadcastMessage)) == 0){
                return add_to_found_devices_if_not_own_uuid(recv_buf, recv_size);
            }
            break;
        case(Message::ConnectRequest):
            if(recv_size > strlen(constants::StaticRequestConnect) && memcmp(recv_buf, constants::StaticRequestConnect, strlen(constants::StaticRequestConnect)) == 0){
                return add_to_found_devices_if_not_own_uuid(recv_buf, recv_size);
            }
            break;
        case(Message::ConnectResponse):
            if(recv_size > strlen(constants::StaticResponseConnect) && memcmp(recv_buf, constants::StaticResponseConnect, strlen(constants::StaticResponseConnect)) == 0){
                return add_to_found_devices_if_not_own_uuid(recv_buf, recv_size);
            }
            break;
    }
    }
    return std::make_pair(false, "");
}

sockaddr_in UdpBroadcast::get_broadcast_addr(){
    return broadcast_addr;
}

std::map<std::string, DeviceData> UdpBroadcast::get_found_devices(){
    return found_devices;
}

bool UdpBroadcast::is_own_ip_bigger(sockaddr_in addr){
    return own_addr.sin_addr.s_addr > addr.sin_addr.s_addr;
}


UdpBroadcast::~UdpBroadcast(){ 
    close(broadcast_sock);
}

//----------//
// Utilites //
//----------//
bool UdpBroadcast::is_ready_to_recv(){
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
    if(ret > 0 && FD_ISSET(broadcast_sock, &readfds)){
        return true;
    }
    return false;
}

std::pair<std::string, std::string> UdpBroadcast::parse_msg(const char *buf, const int &size){
    int colon_counter = 0;
    std::string uuid;
    std::string name;
    for(int i = 0; i < size; i++){
        if(colon_counter == 1 && buf[i] != ':'){
            uuid += buf[i];
        }
        else if(colon_counter == 2 && buf[i] != ':'){
            name += buf[i];
        }
        if(buf[i] == ':'){
            colon_counter++;
        }
    }
    return std::make_pair(uuid, name);
}

std::pair<bool, std::string> UdpBroadcast::add_to_found_devices_if_not_own_uuid(char* buf, size_t size){
    auto [uuid, name] = parse_msg(buf, size);
    if(uuid != env::get_uuid()){
        if(!found_devices.contains(uuid)){
            DeviceData dd;
            dd.addr = tmp_recv_addr;
            dd.name = name;
            dd.last_seen = std::chrono::steady_clock::now();
            found_devices[uuid] = dd;   
        }
        else{
            found_devices[uuid].last_seen = std::chrono::steady_clock::now();
        }
        return std::make_pair(true, uuid);
    }
    return std::make_pair(false, "");
}

//----------------//
// Ip calculating //
//----------------//
bool UdpBroadcast::is_suitable_interface_name(char *name){
    std::string n = name;
    return !(n.find("lo") != std::string::npos ||
            n.find("tun") != std::string::npos ||
            n.find("docker") != std::string::npos ||
            n.find("veth") != std::string::npos ||
            n.find("zt") != std::string::npos ||
            n.find("br-") != std::string::npos);
}


void UdpBroadcast::calculate_broadcast_addr(struct ifaddrs* ifa, sockaddr_in &baddr){
    auto* addr = (struct sockaddr_in*)ifa->ifa_addr;
    auto* mask = (struct sockaddr_in*)ifa->ifa_netmask;
    uint32_t ip_addr = addr->sin_addr.s_addr;
    uint32_t net_mask = mask->sin_addr.s_addr;
    uint32_t broadcast = ip_addr | ~net_mask;
    baddr.sin_addr.s_addr = broadcast;
}

bool UdpBroadcast::get_own_and_brcast_addr(sockaddr_in &own_addr, sockaddr_in &baddr){
    struct ifaddrs* ifaddr;
    
    if(getifaddrs(&ifaddr) == -1){
        throw std::runtime_error("Can't get ifaddr");
    }
    for(struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next){
        if (!ifa->ifa_addr) continue;
        if(!is_suitable_interface_name(ifa->ifa_name)){
            continue;
        }
        if (!ifa->ifa_netmask) continue;
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

void UdpBroadcast::get_own_ip_legasy(){
    int tmp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in tmp_addr{};
    tmp_addr.sin_family = AF_INET;
    tmp_addr.sin_port = htons(53);
    inet_pton(AF_INET, constants::PING_CONNECT_TEST_IP, &tmp_addr.sin_addr);
    
    if(connect(tmp_sock, (sockaddr*)&tmp_addr, sizeof(tmp_addr)) != 0){
        inet_pton(AF_INET, constants::TEST_NET_IP, &tmp_addr.sin_addr);
        if(connect(tmp_sock, (sockaddr*)&tmp_addr, sizeof(tmp_addr)) != 0){
            throw std::runtime_error("Can't build route for know own ip");
        }
    }
    socklen_t own_addr_size = sizeof(own_addr);
    getsockname(tmp_sock, (sockaddr*)&own_addr, &own_addr_size);
    close(tmp_sock);
}

void print_ip(sockaddr_in addr){
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    std::cout << " IP: " << ip << std::endl;
}




    SessionInitializer::SessionInitializer() : br(constants::BROADCAST_PORT){}

    void SessionInitializer::set_state(State st){
        state = st;
    }

    void SessionInitializer::broadcast(){
        while(true){
            if(state == State::Discovering){
                br.send(Message::Broadcast, br.get_broadcast_addr());
                br.read_received_data();
                br.recv(Message::Broadcast);
                std::this_thread::sleep_for(std::chrono::microseconds(constants::SLEEP_TIME));
                found_devices = br.get_found_devices();
            }
            else{
                std::this_thread::sleep_for(std::chrono::microseconds(constants::SLEEP_TIME));
            }
        }
    }

    std::map<std::string, DeviceData> SessionInitializer::get_found_devices(){
        std::vector<std::string> for_delete;
        for(auto [key, val] : found_devices){
            if(std::chrono::steady_clock::now() - val.last_seen >= std::chrono::seconds(5)){
                for_delete.push_back(key);
            }
        }
        for(auto key : for_delete){
            found_devices.erase(key);
        }
        return found_devices;
    }
    
    std::pair<bool, std::string> SessionInitializer::check_incoming_connections(){
        if(state == State::Discovering){
            br.read_received_data();
            auto [succes, uuid] = br.recv(Message::ConnectRequest);
            if(succes){
                return std::make_pair(succes, uuid);
            }
            return std::make_pair(succes, uuid);
        }
        else{
            return std::make_pair(false, "uuid");
        }
    }

    std::tuple<bool, TCPSocket, bool> SessionInitializer::accept_connection(std::string uuid){
        state = State::Syncing;
        br.send(Message::ConnectResponse, br.get_found_devices()[uuid].addr);
        if(create_tcp_connection(uuid) == 0){
            return {true, std::move(sock), is_server};
        }
        else{
            state = State::Discovering;
            return {false, std::move(sock), is_server};
        }
    }

    std::tuple<bool, TCPSocket, bool> SessionInitializer::connect_to(std::string uuid){
        state = State::Syncing;
        if(connect(uuid) == 0){
            return {true, std::move(sock), is_server};
        }
        else{
            state = State::Discovering;
            return {false, std::move(sock), is_server};
        }
    }

    // 0 -> ok
    //-1 -> other errors
    //-2 -> timeout for broadcast
    //-3 -> timeout for TCP
    int SessionInitializer::connect(std::string uuid){
        try{
            time_t start;
            time(&start);
            time_t timer;
            while(true){
                time(&timer);
                if(timer - start > constants::TIMEOUT_TIME){
                    return -2;
                } 
                br.send(Message::ConnectRequest, br.get_found_devices()[uuid].addr);
                br.read_received_data();
                auto [succes, other_uuid] = br.recv(Message::ConnectResponse);
                if(succes && (uuid == other_uuid)){
                    break;
                }
                usleep(constants::SLEEP_TIME);
            }
            int ret = create_tcp_connection(uuid);
            if(ret == -1){
                return -3;
            }
        }
        catch(std::exception &e){
            logerr(e.what());
            return -1;
        }
        return 0;
    }

    // 0 -> ok
    //-1 -> timeout
    int SessionInitializer::create_tcp_connection(std::string uuid){
        // server
        if(br.is_own_ip_bigger(br.get_found_devices()[uuid].addr)){
            is_server = true;
            logmsg("Role - Server");
            Server serv(constants::TCP_PORT);
            time_t start;
            time(&start);
            time_t timer;
            while(true){
                time(&timer);
                if((timer - start) > constants::TIMEOUT_TIME){
                    return -1;
                }
                if(serv.is_ready_to_accept()){
                    sock = serv.accept_conn();
                    break;
                }
                usleep(constants::SLEEP_TIME);
            }
            logmsg("Accept connection from client");
        } else {
            is_server = false;
            logmsg("Role - Client");
            try{
                sockaddr_in addr{};
                addr.sin_addr = br.get_found_devices()[uuid].addr.sin_addr;
                addr.sin_port = htons(constants::TCP_PORT);
                addr.sin_family = AF_INET;
                sock = client_connect(addr);
            }
            catch(std::runtime_error &e){
                return -1;
            }
        }
        return 0;
    }
