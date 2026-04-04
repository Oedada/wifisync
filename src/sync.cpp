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
#include "headers/sync.hpp"

using json = nlohmann::json;

        Sync::Sync(){
            own_name = env::get_name();
            own_uuid = env::get_uuid();
            detecting_paths = read_paths_in_file(env::get_data_path(constants::DATA_DIR) / constants::DETECTING_UNITS_FILENAME, true);
            ignoring_paths = read_paths_in_file(env::get_data_path(constants::DATA_DIR) / constants::IGNORING_UNITS_FILENAME, true);
        }  

        std::vector<std::string> Sync::read_paths_in_file(fs::path path, bool create){
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
        //-3 -> timeout for TCP
        int Sync::connect(std::string uuid){
            is_connecting_process = true;
            try{
                time_t start;
                time(&start);
                time_t timer;
                while(true){
                    time(&timer);
                    if(timer - start > constants::TIMEOUT_TIME){
                        return -2;
                    } 
                    broadcast.send(Message::ConnectRequest, broadcast.get_found_devices()[uuid].first);
                    broadcast.read_received_data();
                    auto [succes, other_uuid] = broadcast.recv(Message::ConnectResponse);
                    if(succes && uuid == other_uuid){
                        break;
                    }
                    usleep(constants::SLEEP_TIME);
                }
                int ret = create_tcp_connection(uuid);
                if(ret == -1){
                    return -3;
                }
            }
            catch(std::exception &e){
                std::cout << "Error: " << e.what();
                return -1;
            }
            return 0;
        }

    // 0 -> ok
    //-1 -> timeout
    int Sync::create_tcp_connection(std::string uuid){
        // server
        if(broadcast.is_own_ip_bigger(broadcast.get_found_devices()[uuid].first)){
            std::cout << "Server\n";
            Server serv(constants::TCP_PORT);
            time_t start;
            time(&start);
            time_t timer;
            while(true){
                time(&timer);
                if((timer - start) > constants::TIMEOUT_TIME){
                    return -1;
                }
                if(serv.is_ready_to_accept()){
                    sock = serv.accept_conn();
                    break;
                }
                usleep(constants::SLEEP_TIME);
            }
            std::cout << "Accept connection from client\n";
        } else {
            std::cout << "Client\n";
            try{
                sockaddr_in addr{};
                addr.sin_addr = broadcast.get_found_devices()[uuid].first.sin_addr;
                addr.sin_port = htons(constants::TCP_PORT);
                addr.sin_family = AF_INET;
                sock = client_connect(addr);
            }
            catch(std::runtime_error &e){
                return -1;
            }
        }
        return 0;
    }

    void Sync::add_to_sync(fs::path path){
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

    void Sync::add_to_ignore(fs::path path){
        if(std::find(ignoring_paths.begin(), ignoring_paths.end(), (std::string)path) == ignoring_paths.end()){
            if(fs::exists(path)){
                ignoring_paths.push_back(path);
                std::ofstream fout(env::get_data_path(constants::DATA_DIR) / constants::IGNORING_UNITS_FILENAME, std::ios::app);
                fout << path.c_str();
                fout.close();
            }
        }
    }

    json Sync::find_devices(){
        for(int i = 0; i < constants::COUNT_FIND_DEVICE; i++){
            broadcast.send(Message::Broadcast, broadcast.get_broadcast_addr());
            broadcast.read_received_data();
            broadcast.recv(Message::Broadcast);
            usleep(constants::SLEEP_TIME);
        }
        json ret;
        for(const auto & [key, val] : broadcast.get_found_devices()){
            ret[val.second] = key;
        }
        return ret;
    }

    bool Sync::connect_to_device(std::string uuid){
        is_connecting_process = true;
        std::string other_uuid;
        time_t start;
        time(&start);
        time_t timer;
        while(true){
            time(&timer);
            if(timer - start > constants::TIMEOUT_TIME){
                other_uuid = "timeout";
                break;
            } 
            broadcast.send(Message::ConnectRequest, broadcast.get_found_devices()[uuid].first);
            broadcast.read_received_data();
            auto [succes, uuid] = broadcast.recv(Message::ConnectResponse);
            if(succes){
                other_uuid = uuid;
                break;
            }
        }
        is_connecting_process = false;
        if(other_uuid != "timeout"){
            return true;
        }
        return false;
    }

    void Sync::change_snapshots(){
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

    json Sync::get_others_paths_difference(){
        fs::path data = env::get_data_path(constants::DATA_DIR);
        Units last(data / constants::LAST_SNAPSHOT_FILENAME, false);
        Units cur(data / constants::CURRENT_SNAPSHOT_FILENAME, false);
        Difference dif(data / constants::DIFFERENCE_FILENAME, last.json_tree, cur.json_tree);
        dif.calculate_difference();
        json other_paths_dif = read_json(data / constants::DIFFERENCE_FILENAME, false);
        json devices_path_table = read_json(env::get_data_path() / constants::DEVICES_FILE, false);
        std::vector<std::string> corresponding_strings;
        std::vector<std::string> not_corresponding_strings;
        for(const auto &[key, val] : other_paths_dif.items()){
            if(!devices_path_table[other_uuid]["paths"].contains(key)){
                std::cerr << "There is no corresponding path to the path " << key;
                not_corresponding_strings.push_back(key);
            }
            else{
                corresponding_strings.push_back(key);
            }
        }
        for(const auto &key : not_corresponding_strings){
            other_paths_dif.erase(key);
        }
        for(const auto &key : corresponding_strings){
            other_paths_dif[devices_path_table[other_uuid]["paths"][key]] = other_paths_dif.at(key);
            if(other_paths_dif.contains(key)){
                other_paths_dif.erase(key);
            }
        }
        return other_paths_dif;
    }

    // bool check_difs(json first_dif, json second_dif){
    //     for(const auto &[key, val] : first_dif.items()){
    //         if(second_dif.contains(key)){
    //             std::cout << 
    //             check_difs(first_dif[key],second_dif[key]);
    //         }
    //     }
    // }

    //-1 -> not connected
    int Sync::sync(){
        if(!is_connected){
            return -1;
        }
        change_snapshots();
        json other_dif = get_others_paths_difference();
        write_json(env::get_data_path(constants::DATA_DIR) / "new_dif.json", other_dif);
        return 0;
    }

// int main(){
//     UdpBroadcast br(constants::BROADCAST_PORT);
//     while(true){
//         br.send(Message::Broadcast, br.get_broadcast_addr());
//         br.read_received_data();
//         auto [succes, uuid] = br.recv(Message::ConnectRequest);
//         if(succes){
//             printf("%s\n", uuid.c_str());
//             br.send(Message::ConnectResponse, br.get_found_devices()[uuid].first);
//         }
//         usleep(constants::SLEEP_TIME);
//     }
//     return 0;
// }
