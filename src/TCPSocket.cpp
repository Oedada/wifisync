#include "headers/TCPSocket.hpp"
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

void TCPSocket::send_file_with_name(fs::path file_path){
    file_path = fs::weakly_canonical(file_path);
    if(!fs::exists(file_path) || !fs::is_regular_file(file_path)){
        throw std::runtime_error("Invalid file path for send with name");
    }
    std::string file_name = file_path.filename();
    std::ifstream fin(file_path, std::ios::binary | std::ios::ate);
    uint64_t file_size = static_cast<uint64_t>(fin.tellg());
    fin.seekg(0, std::ios::beg);
    unsigned char name_size_bytes[sizeof(uint64_t)];
    toBytes(file_name.size(), name_size_bytes);
    ::send(sock, name_size_bytes, sizeof(uint64_t), 0); 
    ::send(sock, file_name.data(), file_name.size(), 0);
    send_file(fin, file_size);
}


std::filesystem::path TCPSocket::recv_file_with_name(std::filesystem::path dir_path){

    dir_path = fs::weakly_canonical(dir_path);
    if(!fs::exists(dir_path) || !fs::is_directory(dir_path)){
        throw std::runtime_error("Inavalid directory path for receive file with name");
    }
    unsigned char name_size_bytes[sizeof(uint64_t)];
    ::recv(sock, name_size_bytes, sizeof(uint64_t), 0);
    uint64_t name_size = fromBytes64(name_size_bytes);
    std::string file_name;
    file_name.resize(name_size);
    ::recv(sock, file_name.data(), name_size, 0);
    std::ofstream fout((dir_path / file_name), std::ios::binary);
    recv_file(fout);
    return (dir_path / file_name);
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
