#include "dif_walker.hpp"
#include "TCPSocket.hpp"
#include "constants.hpp"
#include "difference.hpp"
#include "environment.hpp"
#include "dif_work.hpp"
#include "server.hpp"
#include "transport.hpp"
#include "utils.hpp"
#include "change_applier.hpp"
#include <arpa/inet.h>
#include <filesystem>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <variant>



DifWalker::DifWalker(json d) : dif(d){}

void DifWalker::walk(const std::function<void(SUnit)>& emit){
    for(auto [key, val] : dif.items()){
        if(!dif.at(key).contains(constants::JSON_FIELD_NAME_OTHER_PATH)){
            continue;
        }
        if(dif.at(key).at(constants::JSON_FIELD_NAME_OTHER_PATH) == "/None"){
            continue;
        }
        walk_node(dif.at(key), dif.at(key).at(constants::JSON_FIELD_NAME_OTHER_PATH), key, emit);
    }
}

void walk_added_directory_in_fs(fs::path cur_path, const std::function<void(SUnit)>& emit){
    for(const auto &p : fs::directory_iterator(cur_path)){
        SUnit sunit{};
        sunit.name = p.path().filename();
        if(p.is_directory()){
            sunit.ut = UnitType::Directory;
            uint64_t count_sp = 0;
            for([[maybe_unused]]auto const& sp : fs::directory_iterator(p)){
                count_sp++;
            }
            sunit.data = DirData{count_sp};
        }
        else if(p.is_regular_file()){
            sunit.ut = UnitType::File;
            sunit.data = FileData{p.path()};
        }
        else{
            throw std::runtime_error("Unknow type file");
        }
        sunit.mt = ModifyType::Added;
        emit(sunit);
        if(sunit.ut == UnitType::Directory){
            walk_added_directory_in_fs(cur_path / p.path().filename(), emit);
        }
    }
}

void DifWalker::walk_node(const json &node,const std::string &node_name, const fs::path &cur_path, const std::function<void(SUnit)>& emit){
    SUnit sunit = json_to_sunit(node, cur_path, node_name);
    emit(sunit);
    if(sunit.ut == UnitType::Directory){
        if(sunit.mt == ModifyType::Modified){
            for(const auto &[key, val] : node.items()){
                if(key != constants::JSON_FIELD_NAME_OTHER_PATH && key != constants::JSON_FIELD_NAME_TYPE){
                    if(val.is_object()){
                        walk_node(val, key, cur_path / key, emit);
                    }
                }
            }
        }
        else if(sunit.mt == ModifyType::Added){
            walk_added_directory_in_fs(cur_path, emit);
        }
    }
}

SUnit DifWalker::json_to_sunit(const json &node, const fs::path &cur_path, const std::string &name){
    SUnit u;
    u.name = name;
    if(node.at(constants::JSON_FIELD_NAME_TYPE) == constants::UNIT_TYPE_DIR){
        u.ut = UnitType::Directory;
    } else if(node.at(constants::JSON_FIELD_NAME_TYPE) == constants::UNIT_TYPE_FILE){
        u.ut = UnitType::File;
    } else if(node.at(constants::JSON_FIELD_NAME_TYPE) == constants::UNIT_TYPE_EMPTY_DIR){
        u.ut = UnitType::Directory;
        u.mt = string_to_modify[node.at(constants::JSON_FIELD_NAME_MOD_TYPE)];
        u.data = DirData{0};
        return u;
    } else{
        std::cerr << node.at(constants::JSON_FIELD_NAME_TYPE);
        throw std::runtime_error(std::string("This isn't file or directory: ") + std::string(cur_path));
    }
    if(node.contains(constants::JSON_FIELD_NAME_MOD_TYPE)){
        u.mt = string_to_modify[node.at(constants::JSON_FIELD_NAME_MOD_TYPE)];
    }
    else{
        throw std::runtime_error(cur_path);
    }
    if(u.ut == UnitType::Directory){
        u.data = DirData{count_subelements(node)};
        if(u.mt == ModifyType::Added){
            u.data = DirData{(uint64_t)std::distance(fs::directory_iterator(cur_path), fs::directory_iterator{})};
        }
    }
    else{
        u.data = FileData{cur_path};
    }
    return u;
}


// int main(int argc, char** argv){
//     bool server = false;
//     if(argc > 1){
//         if(argv[1][0] == 's'){
//             server = true;
//         }
//     }
//     if(server){
//         DifWork difwork("534b4b56-d809-482b-865a-48cf82121882");
//         difwork.shift_snapshots();
//         difwork.calculate_dif();
//         json dif = read_json(env::get_data_path(constants::DATA_DIR) / constants::DIFFERENCE_FILENAME, false);
//         DifWalker dw(dif);
//         Server s(12345);
//         std::cout << "Done\n";
//         TCPSocket sock = s.accept_conn();
//         sock.send(1);
//         auto tr = std::make_unique<Transport>(std::move(sock));
//         init_transport(std::move(tr));
//         dw.walk(send_u);
//     }
//     else{
//         sockaddr_in addr{};
//         addr.sin_family = AF_INET;
//         addr.sin_port = htons(12345);
//         inet_pton(AF_INET, "192.168.0.104", &addr.sin_addr);
//         TCPSocket sock = client_connect(addr);
//         Transport tr(std::move(sock));
//         tr.walk_received(print_runit);
//     }
// }
