#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <unistd.h>
#include <fstream>

class TCPSocket{
    public:
        TCPSocket(const TCPSocket&) = delete;
        TCPSocket& operator=(const TCPSocket&) = delete;

        // move-конструктор
        TCPSocket(TCPSocket&& other) noexcept : sock(other.sock) {
            other.sock = -1; // "обнуляем" источник
        }

        // move-оператор
        TCPSocket& operator=(TCPSocket&& other) noexcept {
            if (this != &other) {
                close(sock);          // освободить текущий ресурс
                sock = other.sock;
                other.sock = -1;
            }
            return *this;
        }

        TCPSocket(const int s) : sock(s){}
        void send(const std::vector<char> &buf, const size_t size);
        size_t receive(void *buf, size_t size);
        void smart_send_msg(const std::vector<char> &msg);
        std::vector<unsigned char> smart_recv_msg();
        void smart_send_file(std::ifstream& fin, uint64_t file_size);
        void smart_recv_file(std::ofstream& fout);
        ~TCPSocket();

    private:
        int sock;
};

TCPSocket client_connect(sockaddr_in addr);
