#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <unistd.h>
#include <fstream>
#include <filesystem>

class TCPSocket{
    public:
        TCPSocket() noexcept : sock(-1){}
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
        std::string ip();
        TCPSocket(const int s) : sock(s){}
        void send(const std::vector<char> &buf, const size_t size);
        void send(const std::string &buf);
        void send(const uint64_t &buf);
        uint64_t recv_uint64();
        size_t receive(void *buf, size_t size);
        void smart_send_msg(const std::string &msg);
        std::string smart_recv_msg();
        void send_file(std::ifstream& fin, uint64_t file_size);
        void send_file(std::filesystem::path file_path);
        void recv_file(std::ofstream& fout);
        void recv_file(std::filesystem::path file_path);
        void send_file_with_name(std::filesystem::path file_path);
        std::filesystem::path recv_file_with_name(std::filesystem::path dir_path);
        ~TCPSocket();

    private:
        void check_sock();
        int sock;
};

TCPSocket client_connect(sockaddr_in addr);
