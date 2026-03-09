#include "headers/TCPSocket.hpp"
#include <cstdint>
#include <stdexcept>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
uint64_t MAX_ALLOWED_SIZE = 8388608;

void toBytes(uint64_t x, unsigned char* out_b){
    for(int i = 0; i < 8; i++){
        out_b[7-i] = (x >> i*8) & 0xFF;
    }
}

void TCPSocket::send(const std::vector<char> &buf, const size_t size){
    ::send(sock, buf.data(), size, 0);
}

void TCPSocket::smart_send(const std::vector<char> &buf){
    unsigned char size_bytes[8];
    toBytes((uint64_t)buf.size(), size_bytes);
    ::send(sock, size_bytes, 8, 0);
    ::send(sock, buf.data(), buf.size(), 0);
}

uint64_t fromBytes(unsigned char* in_b){
    uint64_t x = 0;
    for(int i = 0; i < 8; i++){
        x |= (uint64_t)in_b[7-i] << i*8;
    }
    return x;
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

std::vector<unsigned char> TCPSocket::smart_recv(){
    unsigned char size_bytes[8];
    receive(size_bytes, 8);
    uint64_t data_size = fromBytes(size_bytes);
    if (data_size > MAX_ALLOWED_SIZE){
        throw std::runtime_error("Message too large");
    }
    std::vector<unsigned char> buf(data_size);
    receive(buf.data(), buf.size());
    return buf;
}

TCPSocket::~TCPSocket(){
    if (sock >= 0)
        ::close(sock);
}

