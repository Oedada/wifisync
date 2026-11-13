#include "network/TCPSocket.hpp"
#include "util/constants.hpp"
#include "util/exceptions.hpp"
#include "util/utils.hpp"
#include <asm-generic/socket.h>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

void TCPSocket::check_sock() {
    if (sock == -1 || closed.load()) {
        throw std::runtime_error("Socket closed or not initialized");
    }
}

void TCPSocket::receive_callback(
    std::function<void(uint64_t received_count)> callback) {
    while (!closed.load()) {
        pollfd pfd{};
        pfd.fd = sock;
        pfd.events = POLLIN;
        auto ret = poll(&pfd, 1, -1);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                closed.store(true);
                throw SystemMethodError("Poll error check the code!!");
            }
        } else if (ret > 0) {
            if (pfd.revents & POLLNVAL) {
                closed.store(true);
                throw SystemMethodError("Invalid file descriptor");
            } else if (pfd.revents & (POLLIN | POLLHUP)) {
                std::byte buf[constants::BUFFER_SIZE];
                ssize_t n = ::recv(sock, buf, constants::BUFFER_SIZE, 0);
                if (n > 0) {
                    pipe.write(std::span<std::byte>(buf, n));
                    logmsg("Received: ");
                    logmsg(std::vector(buf, buf+constants::BUFFER_SIZE));
                    callback(n);
                } else if (n == 0) {
                    closed.store(true);
                    throw ConnectionClosedError("Connection close by other side");
                } else {
                    switch (errno) {
                    case EINTR:
                    case EWOULDBLOCK:
#if EWOULDBLOCK != EAGAIN
                    case EAGAIN:
#endif
                        continue;
                    default:
                        closed.store(true);
                        throw std::runtime_error("Error while reading socket");
                    }
                }
            }
            } else if (pfd.revents & POLLERR) {
                int err = 0;
                socklen_t len = sizeof(err);
                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len) == -1) {
                    closed.store(true);
                    throw SystemMethodError(
                        "get sockopt error, there I am a bit lazy");
                } else if (err != 0) {
                    switch (err) {
                    case ECONNRESET:
                    case ECONNREFUSED:
                        throw ConnectionClosedError(
                            "Connection refused/reset by other side");
                    case ETIMEDOUT:
                        throw ConnectionClosedError("Connection timedout");
                    case ENETUNREACH:
                    case EHOSTUNREACH:
                        throw NetworkError("Network error");
                    default:
                        closed.store(true);
                        throw TCPSocketError("Some unhandled socket error");
                        closed.store(true);
                    }
                } else if (err == 0) {
                    closed.store(true);
                    throw TCPSocketError("Unknow by getsockopt socker error");
                }
        }
    }
}

void TCPSocket::send(std::span<const std::byte> data) {
    check_sock();
    size_t sended = 0;
    if(data.size() == 0){
        return;
    }
    while (sended != data.size() && !closed.load()) {
        auto n = ::send(sock, data.data() + sended, data.size() - sended,
                        MSG_NOSIGNAL);
        if (n > 0) {
            sended += n;
        } else if (n == 0){
            throw UntypicalError("Socket send return 0 when the size of data isn't 0");
        } else if (n < 0) {
            int error = errno;
            switch (error) {
            case EPIPE:
                throw ConnectionClosedError("Socket closed for sending");
            case ECONNREFUSED:
            case ECONNRESET:
            case ENOTCONN:
                throw ConnectionClosedError("Connection are refused");
            case EINTR:
            case EAGAIN:
#if EAGAIN != EWOULDBLOCK
            case EWOULDBLOCK:
#endif
                continue;
            default:
                throw SystemMethodError("Unknow method error while sending data");
            }
        }
    }
}

void TCPSocket::send(const std::string &buf) {
    send(std::span<const std::byte>(reinterpret_cast<const std::byte*>(buf.data()), buf.size()));
}

void TCPSocket::send(const uint64_t &buf) {
    unsigned char buf_bytes[sizeof(uint64_t)];
    toBytes(buf, buf_bytes);
    ::send(sock, buf_bytes, sizeof(uint64_t), 0);
}

bool TCPSocket::receive(void *data, size_t size, bool non_block) {
    check_sock();
    if (non_block) {
        if (pipe.size() >= size) {
            std::memcpy(data, pipe.eat(size).data(), size);
            return true;
        } else {
            return false;
        }
    } else {
        std::memcpy(data, pipe.eat(size).data(), size);
        return true;
    }
}


void TCPSocket::receive(void *data, size_t size) { receive(data, size, false); }

uint64_t TCPSocket::recv_uint64() {
    unsigned char bytes[sizeof(uint64_t)];
    receive(bytes, sizeof(uint64_t));
    auto number = fromBytes64(bytes);
    return number;
}

void TCPSocket::smart_send_msg(const std::string &msg) {
    check_sock();
    if (static_cast<uint64_t>(msg.size()) > constants::MAX_ALLOWED_SIZE) {
        throw std::runtime_error("Can't send message, it's too large");
    }
    send(msg.size());
    send(msg);
}

std::string TCPSocket::smart_recv_msg() {
    check_sock();
    uint64_t data_size = recv_uint64();
    if (data_size > constants::MAX_ALLOWED_SIZE || data_size < 0) {
        throw std::runtime_error("Invalide message size");
    }
    std::string buf;
    buf.resize(data_size);
    receive(buf.data(), buf.size());
    return buf;
}

void TCPSocket::send_file(std::ifstream &fin, uint64_t file_size) {
    check_sock();
    std::vector<std::byte> buf(constants::BUFFER_SIZE);
    send(file_size);
    if (fin) {
        while (fin.read(reinterpret_cast<char*>(buf.data()), buf.size()) || fin.gcount() > 0) {
            send(std::span<std::byte>(buf.data(), buf.size()));
        }
    } else {
        throw std::runtime_error("Input stream doesn't correct");
    }
}

void TCPSocket::send_file(fs::path file_path) {
    if (!fs::exists(file_path)) {
        throw std::runtime_error("File not exist");
    }
    if (!fs::is_regular_file(file_path)) {
        throw std::runtime_error("This isn't file");
    }
    std::ifstream fin(file_path, std::ios::binary | std::ios::ate);
    uint64_t file_size = static_cast<uint64_t>(fin.tellg());
    fin.seekg(0, std::ios::beg);
    send_file(fin, file_size);
}

void TCPSocket::recv_file(std::ofstream &fout) {
    check_sock();
    std::vector<char> buf(constants::BUFFER_SIZE);
    uint64_t file_size = recv_uint64();
    uint64_t remaining_bytes = file_size;
    int read_size;
    while (remaining_bytes > 0) {
        if (remaining_bytes >= constants::BUFFER_SIZE) {
            remaining_bytes -= constants::BUFFER_SIZE;
            read_size = constants::BUFFER_SIZE;
        } else {
            read_size = remaining_bytes;
            remaining_bytes = 0;
        }
        receive(buf.data(), read_size);
        fout.write(buf.data(), read_size);
    }
}

void TCPSocket::recv_file(fs::path file_path) {
    if (!fs::exists(file_path.parent_path())) {
        throw std::runtime_error("Parent dir doesn't exists");
    }
    std::ofstream fout(file_path, std::ios::binary);
    recv_file(fout);
}

void TCPSocket::send_file_with_name(fs::path file_path) {
    check_sock();
    file_path = fs::weakly_canonical(file_path);
    if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
        throw std::runtime_error(
            std::string("Invalid file path for send with name: ") +
            std::string(file_path));
    }
    std::string file_name = file_path.filename();
    std::ifstream fin(file_path, std::ios::binary | std::ios::ate);
    uint64_t file_size = static_cast<uint64_t>(fin.tellg());
    fin.seekg(0, std::ios::beg);
    send(static_cast<uint64_t>(file_name.size()));
    send(file_name);
    send_file(fin, file_size);
}

std::filesystem::path
TCPSocket::recv_file_with_name(std::filesystem::path dir_path) {
    check_sock();
    dir_path = fs::weakly_canonical(dir_path);
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
        throw std::runtime_error(
            "Inavalid directory path for receive file with name");
    }
    uint64_t name_size = recv_uint64();
    std::string file_name;
    file_name.resize(name_size);
    receive(file_name.data(), name_size);
    std::ofstream fout((dir_path / file_name), std::ios::binary);
    recv_file(fout);
    return (dir_path / file_name);
}

std::string TCPSocket::ip() {
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getpeername(sock, (sockaddr *)&addr, &len);
    // if (getpeername(sock, (sockaddr*)&addr, &len) == 0) {
    //     std::cout << inet_ntoa(addr.sin_addr) << std::endl;
    // }
    std::string ip;
    ip.resize(INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &addr.sin_addr, ip.data(), ip.size());
    return ip;
}

void TCPSocket::close() {
    closed.store(true);
    if (sock >= 0) {
        logwarn("Socket closed");
        ::close(sock);
    }
}

TCPSocket::~TCPSocket() {
    if (sock >= 0) {
        logwarn("Socket closed");
        ::close(sock);
    }
}

TCPSocket client_connect(sockaddr_in addr) {
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    int retries = constants::CONNECT_RETRIES_COUNT;
    while (retries-- > 0) {
        if (::connect(client_sock, (sockaddr *)&addr, sizeof(addr)) == 0)
            break;
        else {
            std::cerr << "Can't connect to server with error "
                      << strerror(errno) << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    if (retries <= 0) {
        throw std::runtime_error("connect failed after retries");
    }
    return TCPSocket(client_sock);
}

sockaddr_in addr_from_ip_and_port(std::string ip, int port) {
    sockaddr_in addr{};
    int res = inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);
    if (res == 0) {
        throw std::runtime_error("Ip address isn't correct");
    } else if (res != 1) {
        throw std::runtime_error("Error while convert ip addr");
    }
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    return addr;
}
