#include "headers/TCPSocket.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <filesystem>
#include "headers/utils.hpp"
#include "headers/constants.hpp"

namespace fs = std::filesystem;

void TCPSocket::send(const std::vector<char> &buf, const size_t size){
    ::send(sock, buf.data(), size, 0);
}

size_t TCPSocket::receive(void *data, size_t size){
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

void TCPSocket::smart_send_msg(const std::vector<char> &msg){
    unsigned char size_bytes[8];
    if ((uint64_t)msg.size() > constants::MAX_ALLOWED_SIZE){
        throw std::runtime_error("Can't send message, it's too large");
    }
    toBytes((uint64_t)msg.size(), size_bytes);
    ::send(sock, size_bytes, sizeof(uint64_t), 0);
    ::send(sock, msg.data(), msg.size(), 0);
}

std::vector<unsigned char> TCPSocket::smart_recv_msg(){
    unsigned char size_bytes[sizeof(uint64_t)];
    receive(size_bytes, sizeof(uint64_t));
    uint64_t data_size = fromBytes64(size_bytes);
    if (data_size > constants::MAX_ALLOWED_SIZE || data_size < 0){
        throw std::runtime_error("Invalide message size");
    }
    std::vector<unsigned char> buf(data_size);
    receive(buf.data(), buf.size());
    return buf;
}

void TCPSocket::send_file(std::ifstream& fin, uint64_t file_size){
    std::vector<char> buf(constants::BUFFER_SIZE);
    unsigned char file_size_bytes[sizeof(uint64_t)];
    toBytes(file_size, file_size_bytes);
    ::send(sock, file_size_bytes, sizeof(uint64_t), 0);
    if(fin){
        while(fin.read(buf.data(), buf.size() || fin.gcount() > 0)){
            ::send(sock, buf.data(), fin.gcount(), 0);
        }
    }
    else{
        throw std::runtime_error("Input stream doesn't correct");
    }
}

void TCPSocket::recv_file(std::ofstream& fout){
    std::vector<char> buf(constants::BUFFER_SIZE);
    unsigned char file_size_bytes[sizeof(uint64_t)];
    receive(file_size_bytes, sizeof(uint64_t));
    uint64_t file_size = fromBytes64(file_size_bytes);
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

void TCPSocket::send_dir(fs::path root){
    if(!fs::is_directory(root)){
        throw std::runtime_error("Root path should be directory");
    }
    for(const auto& unit : fs::directory_iterator(root)){
        std::vector<char> type;
        if(fs::is_directory(unit)){
            type = std::vector<char>('d');
            send(type, 1);
            uint64_t count_unit = std::distance(
            std::filesystem::directory_iterator(unit),
            std::filesystem::directory_iterator{}
            );
            std::vector<char> count_unit_bytes;
            toBytes(count_unit, reinterpret_cast<unsigned char*>(count_unit_bytes.data()));
            send(count_unit_bytes, 8);
            send_dir(root / unit);
        }
        else if(fs::is_regular_file(unit)){
            type = std::vector<char>('f');
            send(type, 1);
            std::ifstream fin(fs::absolute(unit), std::ios::binary | std::ios::ate);
            uint64_t fsize = fin.tellg();
            fin.seekg(0);
            send_file(fin, fsize);
        }
        else{
            throw std::runtime_error("Unknow file type");
        }
    }
}

void TCPSocket::recv_dir(fs::path root, uint64_t count_of_enclosure){
    if(!fs::is_directory(root)){
        throw std::runtime_error("Root path should be directory");
    }
    for(const auto& unit : fs::directory_iterator(root)){
        std::vector<char> type;
        if(fs::is_directory(unit)){
            type = std::vector<char>('d');
            send(type, 1);
            uint64_t count_unit = std::distance(
            std::filesystem::directory_iterator(unit),
            std::filesystem::directory_iterator{}
            );
            std::vector<char> count_unit_bytes;
            toBytes(count_unit, reinterpret_cast<unsigned char*>(count_unit_bytes.data()));
            send(count_unit_bytes, 8);
            send_dir(root / unit);
        }
        else if(fs::is_regular_file(unit)){
            type = std::vector<char>('f');
            send(type, 1);
            std::ifstream fin(fs::absolute(unit), std::ios::binary | std::ios::ate);
            uint64_t fsize = fin.tellg();
            fin.seekg(0);
            send_file(fin, fsize);
        }
        else{
            throw std::runtime_error("Unknow file type");
        }
    }
}

void TCPSocket::recv_dir(){
    char type[1];
    receive(type, 1);
    if(type[0] == 'd'){
        recv_dir();
    }
    else{
        throw std::runtime_error("First unit should have dir type");
    }
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
