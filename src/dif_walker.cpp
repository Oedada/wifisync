#include "dif_walker.hpp"
#include "constants.hpp"
#include "difference.hpp"
#include "environment.hpp"
#include "dif_work.hpp"
#include "utils.hpp"
#include <filesystem>
#include <stdexcept>
#include <sys/types.h>
#include <variant>

DifWalker::DifWalker(json d) : dif(d){}

void DifWalker::walk(auto&& emit){
    for(auto [key, val] : dif.items()){
        if(!dif.at(key).contains(constants::JSON_FIELD_NAME_OTHER_PATH)){
            continue;
        }
        walk_node(dif.at(key), dif.at(key).at(constants::JSON_FIELD_NAME_OTHER_PATH), key, dif.at(key).at(constants::JSON_FIELD_NAME_OTHER_PATH), emit);
    }
}

void DifWalker::walk_node(const json &node,const std::string &node_name, const fs::path &cur_path, const fs::path &other_path, auto&& emit){
    SUnit sunit = json_to_sunit(node, cur_path, other_path, node_name);
    emit(sunit);
    if(node.is_object()){
        for(const auto &[key, val] : node.items()){
            if(key != constants::JSON_FIELD_NAME_OTHER_PATH && key != constants::JSON_FIELD_NAME_TYPE){
                if(val.is_object()){
                    walk_node(val, key, cur_path / key, other_path / key, emit);
                }
            }
        }
    }
}

SUnit DifWalker::json_to_sunit(const json &node, const fs::path &cur_path, const fs::path &other_path, const std::string &name){
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
    }
    else{
        u.data = FileData{other_path};
    }
    return u;
}

void print_sunit(SUnit u){
    std::cout << u;
    std::cout << "---------------\n";
}


int main(){
    DifWork difwork("534b4b56-d809-482b-865a-48cf82121882");
    difwork.calculate_dif();
    json dif = read_json(env::get_data_path(constants::DATA_DIR) / constants::DIFFERENCE_FILENAME, false);
    DifWalker dw(dif);
    dw.walk(print_sunit);
}
