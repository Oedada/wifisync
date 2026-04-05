#pragma  once
#include <filesystem>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include "broadcast.hpp"
#include "constants.hpp"
#include "init.hpp"
#include "environment.hpp"
#include "TCPSocket.hpp"
#include "difference.hpp"

using json = nlohmann::json;

class Sync{
    private:
    std::string other_uuid;
    std::string own_uuid;
    std::string other_name;
    std::string own_name;
    TCPSocket sock;
    bool is_connected = false;
    bool is_server;
    std::vector<std::string> detecting_paths;
    std::vector<std::string> ignoring_paths;
    void change_snapshots();
    json get_others_paths_difference();
    // void recv(ModifyType mt, fs::path dir_path);
    public:
        Sync();
        int create_tcp_connection(std::string uuid);
        std::vector<std::string> read_paths_in_file(fs::path path, bool create);
        // 0 -> ok
        //-1 -> other errors
        //-2 -> timeout for broadcast
        //-3 -> timeout for recv connect from other side
        //-4 -> can't connect to server
        int connect(std::string uuid);
        bool connect_to_device(std::string uuid);
        void add_to_sync(fs::path path);
        void add_to_ignore(fs::path path);
        json find_devices();
        //-1 -> not connected
        int sync();
        UdpBroadcast broadcast;
        bool is_connecting_process;

};
