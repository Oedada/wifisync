#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include "hash.hpp"
#include "files_opers.hpp"

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

void Units::set_unit(const fs::path &p){
    if(!fs::exists(p)){
        throw std::runtime_error(std::string("Directory ") + p.string() + " not exists");
    } 
    if(is_registred(p)){
        throw std::runtime_error(std::string("Unit ") + p.string() + " is already registred");
    }
    create_json_file_list(p, json_tree[p]);
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

void Units::create_json_file_list(fs::path path, json& json_field){
    path = fs::absolute(path);
    std::vector<fs::path> units;
    if(fs::is_directory(path)){
        if(fs::is_empty(path)){
            json_field = json::array({"/empty_dir"});
        }
        for(const fs::path& entry: fs::directory_iterator(path))
            create_json_file_list(entry, json_field[entry.filename().c_str()]);
    }
    else
        json_field = json::array({"/file"});
}

int main(){
    Units plugins("data/files.json");
    try{
        plugins.set_unit("data/train");
    }
    catch(std::runtime_error e){
        std::cerr << "WARN: " << e.what();
    }
    plugins.rm_unit("data/train/HiddenArmor");
    std::cout << plugins.is_registred("data/train/lol33");
    std::cout << plugins.is_registred("data/train/spark");
    json configs;
    plugins.get_unit("data/train", configs);
    std::string json_string = configs.dump();
    std::ofstream fout("data/test.json");
    fout.write(json_string.c_str(), json_string.size());
    return 0;
}
