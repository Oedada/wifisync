#include "headers/TCPSocket.hpp"
#include <cstdint>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <iostream>
constexpr uint64_t MAX_ALLOWED_SIZE = 8388608;
constexpr uint64_t BUFFER_SIZE = 8192;

void toBytes(uint64_t x, unsigned char* out_b){
    for(int i = 0; i < 8; i++){
        out_b[7-i] = (x >> i*8) & 0xFF;
    }
}

uint64_t fromBytes(unsigned char* in_b){
    uint64_t x = 0;
    for(int i = 0; i < 8; i++){
        x |= (uint64_t)in_b[7-i] << i*8;
    }
    return x;
}

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
    if ((uint64_t)msg.size() > MAX_ALLOWED_SIZE){
        throw std::runtime_error("Can't send message, it's too large");
    }
    toBytes((uint64_t)msg.size(), size_bytes);
    ::send(sock, size_bytes, 8, 0);
    ::send(sock, msg.data(), msg.size(), 0);
}

std::vector<unsigned char> TCPSocket::smart_recv_msg(){
    unsigned char size_bytes[8];
    receive(size_bytes, 8);
    uint64_t data_size = fromBytes(size_bytes);
    if (data_size > MAX_ALLOWED_SIZE || data_size < 0){
        throw std::runtime_error("Invalide message size");
    }
    std::vector<unsigned char> buf(data_size);
    receive(buf.data(), buf.size());
    return buf;
}

void TCPSocket::smart_send_file(std::ifstream& fin, uint64_t file_size){
    std::vector<char> buf(BUFFER_SIZE);
    unsigned char file_size_bytes[8];
    toBytes(file_size, file_size_bytes);
    ::send(sock, file_size_bytes, 8, 0);
    if(fin){
        while(fin.read(buf.data(), buf.size() || fin.gcount() > 0)){
            ::send(sock, buf.data(), fin.gcount(), 0);
        }
    }
    else{
        throw std::runtime_error("Input stream doesn't correct");
    }
}

void TCPSocket::smart_recv_file(std::ofstream& fout){
    std::vector<char> buf(BUFFER_SIZE);
    unsigned char file_size_bytes[8];
    receive(file_size_bytes, 8);
    uint64_t file_size = fromBytes(file_size_bytes);
    std::cout << file_size;
    uint64_t remaining_bytes = file_size;
    int read_size;
    while(remaining_bytes > 0){
        if(remaining_bytes >= BUFFER_SIZE){
            remaining_bytes -= BUFFER_SIZE;
            read_size = BUFFER_SIZE;
        }
        else{
            read_size =  remaining_bytes;
            remaining_bytes = 0;
        }
        receive(buf.data(), buf.size());
        fout.write(buf.data(), read_size);
    }
}

TCPSocket::~TCPSocket(){
    if (sock >= 0)
        ::close(sock);
}

TCPSocket client_connect(sockaddr_in addr){
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(::connect(client_sock, (sockaddr*)&addr, sizeof(addr)) != 0){
        throw std::runtime_error("Error with connect to server");
    }
    return TCPSocket(client_sock);
}
