#include "TCPSocket.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <filesystem>
#include "utils.hpp"
#include "constants.hpp"

namespace fs = std::filesystem;

void TCPSocket::check_sock(){
    if(sock == -1){
        throw std::runtime_error("Socket not initialized!");
    }
}

void TCPSocket::send(const std::vector<char> &buf, const size_t size){
    check_sock();
    ::send(sock, buf.data(), size, 0);
}

void TCPSocket::send(const std::string &buf){
    std::vector<char> vect(buf.begin(), buf.end());
    send(vect, vect.size());
}

void TCPSocket::send(const uint64_t &buf){
    unsigned char buf_bytes[sizeof(uint64_t)];
    toBytes(buf, buf_bytes);
    ::send(sock, buf_bytes, sizeof(uint64_t), 0);
}

size_t TCPSocket::receive(void *data, size_t size){
    check_sock();
    size_t total = 0;
    char* ptr = static_cast<char*>(data);
    while(total < size){
        ssize_t n = ::recv(sock, ptr + total, size-total, 0);
        if(n < 0){
            throw std::runtime_error("Failed receiving data");
        } else if(n == 0){
            throw std::runtime_error("Socket has closed by other side");
        } else{
            total += n;
        }
    }
    return total;
}

uint64_t TCPSocket::recv_uint64(){
    unsigned char bytes[sizeof(uint64_t)];
    receive(bytes, sizeof(uint64_t));
    auto number = fromBytes64(bytes);
    return number;
}

void TCPSocket::smart_send_msg(const std::string &msg){
    check_sock();
    if (static_cast<uint64_t>(msg.size()) > constants::MAX_ALLOWED_SIZE){
        throw std::runtime_error("Can't send message, it's too large");
    }
    send((uint64_t)msg.size());
    send(msg);
}

std::string TCPSocket::smart_recv_msg(){
    check_sock();
    uint64_t data_size = recv_uint64();
    if (data_size > constants::MAX_ALLOWED_SIZE || data_size < 0){
        throw std::runtime_error("Invalide message size");
    }
    std::string buf;
    buf.resize(data_size);
    receive(buf.data(), buf.size());
    return buf;
}

void TCPSocket::send_file(std::ifstream& fin, uint64_t file_size){
    check_sock();
    std::vector<char> buf(constants::BUFFER_SIZE);
    send(file_size);
    if(fin){
        while(fin.read(buf.data(), buf.size() || fin.gcount() > 0)){
            send(buf, fin.gcount());
        }
    }
    else{
        throw std::runtime_error("Input stream doesn't correct");
    }
}

void TCPSocket::recv_file(std::ofstream& fout){
    check_sock();
    std::vector<char> buf(constants::BUFFER_SIZE);
    uint64_t file_size = recv_uint64();
    uint64_t remaining_bytes = file_size;
    int read_size;
    while(remaining_bytes > 0){
        if(remaining_bytes >= constants::BUFFER_SIZE){
            remaining_bytes -= constants::BUFFER_SIZE;
            read_size = constants::BUFFER_SIZE;
        }
        else{
            read_size =  remaining_bytes;
            remaining_bytes = 0;
        }
        receive(buf.data(), read_size);
        fout.write(buf.data(), read_size);
    }
}

void TCPSocket::send_file_with_name(fs::path file_path){
    check_sock();
    file_path = fs::weakly_canonical(file_path);
    if(!fs::exists(file_path) || !fs::is_regular_file(file_path)){
        throw std::runtime_error("Invalid file path for send with name");
    }
    std::string file_name = file_path.filename();
    std::ifstream fin(file_path, std::ios::binary | std::ios::ate);
    uint64_t file_size = static_cast<uint64_t>(fin.tellg());
    fin.seekg(0, std::ios::beg);
    send(static_cast<uint64_t>(file_name.size()));
    send(file_name);
    send_file(fin, file_size);
}


std::filesystem::path TCPSocket::recv_file_with_name(std::filesystem::path dir_path){
    check_sock();
    dir_path = fs::weakly_canonical(dir_path);
    if(!fs::exists(dir_path) || !fs::is_directory(dir_path)){
        throw std::runtime_error("Inavalid directory path for receive file with name");
    }
    uint64_t name_size = recv_uint64();
    std::string file_name;
    file_name.resize(name_size);
    receive(file_name.data(), name_size);
    std::ofstream fout((dir_path / file_name), std::ios::binary);
    recv_file(fout);
    return (dir_path / file_name);
}

std::string TCPSocket::ip(){
    sockaddr_in addr;
    socklen_t len = sizeof(addr);

    if (getpeername(sock, (sockaddr*)&addr, &len) == 0) {
        std::cout << inet_ntoa(addr.sin_addr) << std::endl;
    }
    std::string ip;
    ip.resize(INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &addr.sin_addr, ip.data(), ip.size());
    return ip;
}

TCPSocket::~TCPSocket(){
    if (sock >= 0)
        ::close(sock);
}

TCPSocket client_connect(sockaddr_in addr){
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    int retries = constants::CONNECT_RETRIES_COUNT;
    while(retries-- > 0) {
        if(::connect(client_sock, (sockaddr*)&addr, sizeof(addr)) == 0) break;
        else{
            std::cerr << "Can't connect to server with error " << strerror(errno) << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    if(retries <= 0) {
        throw std::runtime_error("connect failed after retries");
    }
    return TCPSocket(client_sock);
}
