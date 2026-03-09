#pragma once
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>

class TCPSocket{
    public:
        TCPSocket(const TCPSocket&) = delete;
        TCPSocket& operator=(const TCPSocket&) = delete;

        TCPSocket(const int s) : sock(s){}
        void send(const std::vector<char> &buf, const size_t size);
        size_t receive(void *buf, size_t size);
        void smart_send(const std::vector<char> &buf);
        std::vector<unsigned char> smart_recv();
        ~TCPSocket();

    private:
        int sock;
};
