#pragma once
#include "concurrency/bytepipe.hpp"
#include <arpa/inet.h>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

/**
 * @brief TCP socket wrapper.
 *
 * @warning Not thread-safe.
 *          Concurrent access from multiple threads is not supported.
 *          External synchronization is required.
 */
class TCPSocket{
    public:
        TCPSocket() noexcept : pipe(), sock(-1){}
        TCPSocket(const TCPSocket&) = delete;
        TCPSocket& operator=(const TCPSocket&) = delete;

        // move-конструктор
        TCPSocket(TCPSocket&& other) noexcept : pipe(std::move(other.pipe)), sock(other.sock){
            other.sock = -1; // "обнуляем" источник
        }

        // move-оператор
        TCPSocket& operator=(TCPSocket&& other) noexcept {
            if (this != &other) {
                ::close(sock);          // освободить текущий ресурс
                sock = other.sock;
                pipe = std::move(other.pipe);
                other.sock = -1;
            }
            return *this;
        }
        std::string ip();
        TCPSocket(const int s) : pipe(), sock(s){}
        // sends
        void send(std::span<const std::byte> data);
        void send(const std::string &buf);
        void send(const uint64_t &buf);
        void send_file(std::filesystem::path file_path);
        void send_file(std::ifstream& fin, uint64_t file_size);
        void send_file_with_name(std::filesystem::path file_path);
        void smart_send_msg(const std::string &msg);
        // sends

        //recvs
        void receive_callback(std::function<void (uint64_t received_count)> callback);
        uint64_t recv_uint64();
        void receive(void *buf, size_t size);
        bool receive(void *buf, size_t size, bool non_block);
        //recvs

        // legacy
        std::string smart_recv_msg();
        void recv_file(std::ofstream& fout);
        void recv_file(std::filesystem::path file_path);
        std::filesystem::path recv_file_with_name(std::filesystem::path dir_path);
        // legacy
        void close();
        ~TCPSocket();

    private:
        
        std::atomic<bool> closed = false;
        BytePipe pipe;
        void check_sock();
        int sock;
};

TCPSocket client_connect(sockaddr_in addr);
sockaddr_in addr_from_ip_and_port(std::string ip, int port);
