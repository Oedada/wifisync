#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include "headers/hash.hpp"
#include "headers/files_opers.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

    Units::Units(const fs::path& jp) : json_path(jp){
        if(!fs::exists(json_path.parent_path())){
            throw std::runtime_error("Путь к файлу не валиден");
        }
        if(!fs::exists(json_path)){
            std::ofstream fout(json_path, std::ios::out);
            fout.write("{}", 2);
        }
        std::ifstream fin(json_path, std::ios::in);
        fin >> json_tree;
    }

    void Units::set_unit(const fs::path &p, bool ignore_registred){
        if(!fs::exists(p)){
            throw std::runtime_error(std::string("Directory ") + p.string() + " not exists");
        } 
        if(is_registred(p) & !ignore_registred){
            throw std::runtime_error(std::string("Unit ") + p.string() + " is already registred");
        }
        create_json_file_list(p, json_tree[p], true);
        write_hash(p, json_tree[p], std::vector<unsigned char> (p.string().begin(), p.string().end()), true);
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
        throw std::runtime_error("Path is uncorrect");
    }

    json* Units::get_unit_from_path(const fs::path &p){
        json* unit_pointer;
        std::vector<std::string> remaining_path;
        find_root(p, unit_pointer, remaining_path);
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

    std::vector<unsigned char> Units::ulong_to_uchar(unsigned long count){
        std::vector<unsigned char> chars;
        while(count != 0){
            chars.push_back(count % 256);
            count /= 256;
        }
        return chars;
    }

    void Units::write_hash(fs::path path, json& json_field, std::vector<unsigned char> json_field_key, bool is_first_call){
        if (is_first_call){
            path = fs::absolute(path);
        }
        Hash unit_hash;
        unit_hash.update(ulong_to_uchar(json_field_key.size()));
        unit_hash.update(json_field_key);
        if(json_field["/type"] == "dir") {
            for(const auto& [key, val] : json_field.items()){
                if(key == "/hash" || key == "/type"){
                    continue;
                }
                write_hash(path / key, val, std::vector<unsigned char> (key.begin(), key.end()), false);
                unit_hash.update(json_field[key]["/hash"]);
            }
        }
        else if(json_field["/type"] == "file"){
            std::ifstream fin(path, std::ios::binary);
            std::vector<unsigned char> buf(8192);
            while(fin.read(reinterpret_cast<char*>(buf.data()), buf.size()) || fin.gcount() > 0){
                unit_hash.update(std::vector<unsigned char> (buf.begin(), buf.begin() + fin.gcount()));
            }
        }
        unit_hash.calculate();
        json_field["/hash"] = unit_hash.hash;
    }

    void Units::create_json_file_list(fs::path path, json& json_field, bool is_first_call){
        if (is_first_call){
            path = fs::absolute(path);
        }
        std::vector<fs::path> units;
        if(fs::is_directory(path)){
            if(fs::is_empty(path)){
                json_field["/type"] = "empty_dir";
            }
            else {
                json_field["/type"] = "dir";
                for(const fs::path& entry: fs::directory_iterator(path)){
                    create_json_file_list(entry, json_field[entry.filename().c_str()], false);
                }
            }
        }
        else if(fs::is_regular_file(path)){
            json_field["/type"] = "file";
        }
    }
