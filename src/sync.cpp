#include <cstdint>
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

#include "init.hpp"
#include "constants.hpp"
#include "broadcast.hpp"
#include "environment.hpp"
#include "server.hpp"
#include "TCPSocket.hpp"
#include "utils.hpp"
#include "files_opers.hpp"
#include "difference.hpp"
#include "sync.hpp"
#include <filesystem>
#include <variant>

namespace fs = std::filesystem;
bool dry_run = true;


using json = nlohmann::json;

        Sync::Sync() : broadcast(constants::BROADCAST_PORT){
        }

    void Sync::send_unit_change(UnitChange uc){
        sock.send(modify_to_string[uc.mt]);
        sock.send(unit_type_to_string[uc.ut]);
        sock.smart_send_msg(uc.name);
        if(uc.mt != ModifyType::Deleted){
            if(uc.ut == UnitType::File){
                if (std::holds_alternative<FileData>(uc.data)) {
                    auto fp = std::get<FileData>(uc.data).file_path;
                    std::ifstream fin(fp, std::ios::binary);
                    fin.seekg(0, std::ios::end);
                    auto fs = fin.tellg();
                    fin.seekg(0, std::ios::beg);
                    sock.send_file(fin, static_cast<uint64_t>(fs));
                }
                else{
                    throw std::runtime_error("Can't parse erro type");
                }
            }
            else if(uc.ut == UnitType::Directory){
                if(std::holds_alternative<DirData>(uc.data)){
                    sock.send(std::get<DirData>(uc.data).subunits_number);
                }
                else{
                    throw std::runtime_error("Can't parse erro type");
                }
            }
        }
    }

    UnitChange Sync::json_to_UC(json j, fs::path p){
        if(!j.is_string()){
            throw std::runtime_error("Not a string json");
        }
        UnitChange uc;
        if(fs::is_directory(p)){
            uc.ut = UnitType::Directory;
            size_t count = 0;
            for (const auto& entry : std::filesystem::directory_iterator(p)) {
                count++;
            }
            uc.data = DirData{count};
        }
        else if(fs::is_regular_file(p)){
            uc.ut = UnitType::File;
            uc.data = FileData{p};
        }
        auto val = j.get<std::string>();
        if(val == "A"){
            uc.mt = ModifyType::Added;
        }
        else if(val == "D"){
            uc.mt = ModifyType::Deleted;
        }
        else if(val == "M"){
            uc.mt = ModifyType::Modified;
        }
        uc.name = p.filename();
        return uc;
    }

// int main(){
//     Sync ss;
//     ss.sync();
//     return 0;
// }
