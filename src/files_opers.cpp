#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include "hash.hpp"
#include "files_opers.hpp"
#include "constants.hpp"
#include "utils.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

    Units::Units(const fs::path& jp, bool clear_create) : json_path(jp){
        if(!fs::exists(json_path.parent_path())){
            throw std::runtime_error("Путь к файлу не валиден");
        }
        if(!fs::exists(json_path) || clear_create){
            std::ofstream fout(json_path, std::ios::out);
            fout << "{}";
        }
        std::ifstream fin(json_path, std::ios::in);
        fin >> json_tree;
        
    }

    void Units::set_unit(const fs::path &p, bool ignore_registred){
        fs::path np = fs::weakly_canonical(p);
        if(!fs::exists(np)){
            throw std::runtime_error(std::string("Directory ") + np.string() + " not exists");
        } 
        if(is_registred(np) & !ignore_registred){
            throw std::runtime_error(std::string("Unit ") + np.string() + " is already registred");
        }
        create_json_file_list(np, json_tree[np], true);
        std::string str = np.string();
        write_hash(np, json_tree[np], std::vector<unsigned char> (str.begin(), str.end()), true);
        save();
    }

    void Units::get_unit(const fs::path p, json &unit){
        unit = *get_unit_from_path(p);
    }

    void Units::rm_unit(const fs::path &p){
        json* parent;
        parent = get_unit_from_path(p.parent_path());
        parent->erase(p.filename().c_str());
        save();
    }

    bool Units::is_registred(const fs::path p){
        try{
            get_unit_from_path(p);
        }
        catch(std::runtime_error &e){
            return false;
        }
        return true;
    }

    void Units::save(){
        std::ofstream fout(json_path, std::ios::out);
        std::string jstring = json_tree.dump();
        fout.write(jstring.c_str(), jstring.size());
    }

    std::vector<std::string> Units::vector_from_path(const fs::path &p){
        std::vector<std::string> path_parts;
        for(std::string part : fs::absolute(p)){
            path_parts.push_back(part);
        }
        return path_parts;
    }

    //находит рут-юнит(если нет выбрасывает runtime error),  а также записывает остаток пути.
    void Units::find_root(const fs::path &p, json* &unit_path_part_pointer, std::vector<std::string> &remaining_path_link){
        std::vector<std::string> path_parts = vector_from_path(p);
        for(auto [key, val] : json_tree.items()){
            std::vector<std::string> json_parts = vector_from_path(key);
            if(json_parts.size() == path_parts.size()){
                if(json_parts == path_parts){
                    unit_path_part_pointer = &json_tree[key];
                    remaining_path_link = std::vector<std::string>{};
                    return;
                }
            }
            else if(json_parts.size() < path_parts.size()){
                if(std::equal(json_parts.begin(), json_parts.end(), path_parts.begin())){
                    unit_path_part_pointer = &json_tree[key];
                    remaining_path_link = std::vector<std::string> (path_parts.begin() + json_parts.size(), path_parts.end());
                    return;
                }
            }
        }
        throw std::runtime_error("Path is uncorrect this");
    }

    json* Units::get_unit_from_path(const fs::path &p){
        fs::path np = fs::weakly_canonical(p);
        json* unit_pointer;
        std::vector<std::string> remaining_path;
        find_root(np, unit_pointer, remaining_path);
        if(remaining_path.size() == 0){
            return unit_pointer;
        }
        for(std::string part : remaining_path){
            try{
                unit_pointer = &((*unit_pointer).at(part));
            }
            catch(json::out_of_range &e){
                throw std::runtime_error("Path is uncorrect");
            }
        }
        return unit_pointer;
    }

    void Units::write_hash(fs::path path, json& json_field, std::vector<unsigned char> json_field_key, bool is_first_call){
        if (is_first_call){
            path = fs::absolute(path);
        }
        Hash unit_hash;
        std::vector<unsigned char> bytes_of_size(8, 0);
        toBytes(json_field.size(), bytes_of_size.data());
        unit_hash.update(bytes_of_size);
        unit_hash.update(json_field_key);
        if(json_field[constants::JSON_FIELD_NAME_TYPE] == constants::UNIT_TYPE_DIR) {
            for(const auto& [key, val] : json_field.items()){
                if(key == constants::JSON_FIELD_NAME_HASH || key == constants::JSON_FIELD_NAME_TYPE){
                    continue;
                }
                write_hash(path / key, val, std::vector<unsigned char> (key.begin(), key.end()), false);
                unit_hash.update(json_field[key][constants::JSON_FIELD_NAME_HASH]);
            }
        }
        else if(json_field[constants::JSON_FIELD_NAME_TYPE] == constants::UNIT_TYPE_FILE){
            std::ifstream fin(path, std::ios::binary);
            std::vector<unsigned char> buf(constants::BUFFER_SIZE);
            while(fin.read(reinterpret_cast<char*>(buf.data()), buf.size()) || fin.gcount() > 0){
                unit_hash.update(std::vector<unsigned char> (buf.begin(), buf.begin() + fin.gcount()));
            }
        }
        unit_hash.calculate();
        json_field[constants::JSON_FIELD_NAME_HASH] = unit_hash.hash;
    }

    void Units::create_json_file_list(fs::path path, json& json_field, bool is_first_call){
        if (is_first_call){
            path = fs::absolute(path);
        }
        std::vector<fs::path> units;
        if(fs::is_directory(path)){
            if(fs::is_empty(path)){
                json_field[constants::JSON_FIELD_NAME_TYPE] = constants::UNIT_TYPE_EMPTY_DIR;
            }
            else {
                json_field[constants::JSON_FIELD_NAME_TYPE] = constants::UNIT_TYPE_DIR;
                for(const fs::path& entry: fs::directory_iterator(path)){
                    create_json_file_list(entry, json_field[entry.filename().c_str()], false);
                }
            }
        }
        else if(fs::is_regular_file(path)){
            json_field[constants::JSON_FIELD_NAME_TYPE] = constants::UNIT_TYPE_FILE;
        }
    }
