#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include <nlohmann/json.hpp>

#include "headers/init.hpp"
#include "headers/constants.hpp"
#include "headers/broadcast.hpp"
#include "headers/environment.hpp"
#include "headers/server.hpp"
#include "headers/TCPSocket.hpp"
#include "headers/utils.hpp"
#include "headers/files_opers.hpp"
#include "headers/difference.hpp"

using json = nlohmann::json;

class Sync{
    private:
        std::string other_uuid;
        std::string own_uuid;
        std::string other_name;
        std::string own_name;
        sockaddr_in other_addr;
        TCPSocket sock;
        bool is_connected = false;
        bool is_server;
        std::vector<std::string> detecting_paths;
        std::vector<std::string> ignoring_paths;
    public:
        Sync(){
            own_name = env::get_name();
            own_uuid = env::get_uuid();
            detecting_paths = read_path(env::get_data_path(constants::DATA_DIR) / constants::DETECTING_UNITS_FILENAME, true);
            ignoring_paths = read_path(env::get_data_path(constants::DATA_DIR) / constants::IGNORING_UNITS_FILENAME, true);
        }  

        std::vector<std::string> read_path(fs::path path, bool create){
            if(!fs::exists(path) && create){
                std::ofstream fout(path);
                fout.close();
            }
            std::ifstream fin(path);
            if(!fin){
                throw std::runtime_error("Can't opend path");
            }
            std::string str;
            std::vector<std::string> strings;
            while(std::getline(fin, str)){
                fs::path w = fs::weakly_canonical(str);
                if(fs::exists(w)){
                    strings.push_back(w);
                }
            }
            return strings;
        }
        // 0 -> ok
        //-1 -> other errors
        //-2 -> timeout for broadcast
        //-3 -> timeout for recv connect from other side
        //-4 -> can't connect to server
        int connect(){
            try{
                UdpBroadcast br(constants::BROADCAST_PORT);
                time_t start;
                time(&start);
                time_t counter;
                while(!br.recieve()){
                    br.send_broadcast();
                    time(&counter);
                    if(counter - start > constants::TIMEOUT_TIME){
                        return -2;
                    }
                    usleep(constants::SLEEP_TIME);
                }
                json devices = read_json(env::get_data_path(constants::DEVICES_FILE), false);
                if(!devices.contains(br.other_uuid)){
                    devices[br.other_uuid]["name"] = br.other_name;
                    devices[br.other_uuid]["paths"] = json::object();
                    write_json(env::get_data_path(constants::DEVICES_FILE), devices);
                }
                other_uuid = br.other_uuid;
                other_name = br.other_name;
                // Создаёт сервер тот, у кого айпи больше
                if(br.is_own_ip_bigger()){
                    Server server(constants::TCP_PORT);
                    time(&start);
                    while(!server.is_ready_to_accept()){
                        time(&counter);
                        std::cout << counter - start << "\n";
                        if(counter - start > constants::TIMEOUT_TIME){
                            return -3;
                        }
                        usleep(constants::SLEEP_TIME);
                    }
                    TCPSocket s = server.accept_conn();
                    sock = std::move(s);
                }
                else{
                    try{
                        sockaddr_in addr{};
                        addr.sin_addr.s_addr = br.other_addr.sin_addr.s_addr;
                        addr.sin_port = htons(constants::TCP_PORT);
                        addr.sin_family = br.other_addr.sin_family;
                        TCPSocket s = client_connect(addr);
                        sock = std::move(s);
                    }
                    catch(std::runtime_error &e){
                        return -4;
                    }
                }
                is_connected = true;
                return 0;
            }
        catch(std::exception &e){
            std::cerr << "Error: \n" << e.what() << "\n";
            return -1;
        }
        catch(...){
            std::cerr << "Unknow or not std error!";
            return -1;
        }
    }

    void add_to_sync(fs::path path){
        if(std::find(detecting_paths.begin(), detecting_paths.end(), (std::string)path) == detecting_paths.end()){
            if(fs::exists(path)){
                detecting_paths.push_back(path);
                fs::path data = env::get_data_path(constants::DATA_DIR);
                std::ofstream fout(data / constants::DETECTING_UNITS_FILENAME, std::ios::app);
                fout << path.c_str();
                fout.close();
            }
        }
    }   

    void add_to_ignore(fs::path path){
        if(std::find(ignoring_paths.begin(), ignoring_paths.end(), (std::string)path) == ignoring_paths.end()){
            if(fs::exists(path)){
                ignoring_paths.push_back(path);
                std::ofstream fout(env::get_data_path(constants::DATA_DIR) / constants::IGNORING_UNITS_FILENAME, std::ios::app);
                fout << path.c_str();
                fout.close();
            }
        }
    }

    void change_snapshots(){
        fs::path lasts = env::get_data_path(constants::DATA_DIR) / constants::LAST_SNAPSHOT_FILENAME;
        fs::path curs = env::get_data_path(constants::DATA_DIR) / constants::CURRENT_SNAPSHOT_FILENAME;
        if(fs::exists(curs)){
            Units cur(curs, false);
            for(const auto &path : ignoring_paths){
                if(cur.is_registred(path)){
                    cur.rm_unit(path);
                }
            }
            for(const auto &path :detecting_paths){
                if(!cur.is_registred(path)){
                    cur.json_tree[path] = json::object();
                    if(fs::is_regular_file(path)){
                        cur.json_tree[path][constants::JSON_FIELD_NAME_TYPE] = constants::UNIT_TYPE_FILE;
                    }
                    else if(fs::is_directory(path)){
                        cur.json_tree[path][constants::JSON_FIELD_NAME_TYPE] = constants::UNIT_TYPE_DIR;
                    }
                    for(int i = 0; i < constants::HASH_BYTE_LENGTH; i++){
                        cur.json_tree[path][constants::JSON_FIELD_NAME_HASH][i] = 0;
                    }
                    cur.save();
                }
            }
            fs::copy_file(curs, lasts, fs::copy_options::overwrite_existing);
        }
        Units cur(curs, true);
        for(const auto &path : detecting_paths){
            cur.set_unit(path, true);
        }
        for(const auto &path : ignoring_paths){
            cur.rm_unit(path);
        }
        if(!fs::exists(lasts)){
            fs::copy_file(curs, lasts, fs::copy_options::overwrite_existing);
        }
    }

    //-1 -> not connected
    int sync(){
        if(!is_connected){
            return -1;
        }
        change_snapshots();
        fs::path data = env::get_data_path(constants::DATA_DIR);
        Units last(data / constants::LAST_SNAPSHOT_FILENAME, false);
        Units cur(data / constants::CURRENT_SNAPSHOT_FILENAME, false);
        Difference dif(data / constants::DIFFERENCE_FILENAME, last.json_tree, cur.json_tree);
        dif.calculate_difference();
        json other_paths_dif = read_json(data / constants::DIFFERENCE_FILENAME, false);
        json devices_path_table = read_json(env::get_data_path() / constants::DEVICES_FILE, false);
        for(const auto &[key, val] : other_paths_dif.items()){
            if(!devices_path_table[other_uuid]["paths"].contains(key)){
                throw std::runtime_error(std::string("Path must be mapped to another path\n Not mapped path: ") + std::string(key));
            }
        }
        for(const auto &[key, val] : other_paths_dif.items()){
            other_paths_dif[devices_path_table[other_uuid]["paths"][key]] = other_paths_dif[key];
            other_paths_dif.erase(key);
        }
        write_json(data / "new_dif.json", other_paths_dif);
        return 0;
    }

};

int main(){
    init();
    Sync session;
    session.connect();
    session.sync();
}
