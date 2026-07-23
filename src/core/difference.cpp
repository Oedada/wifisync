#include "core/difference.hpp"
#include "nlohmann/json.hpp"
#include "util/constants.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

Difference::Difference(std::filesystem::path file_name, json last_tree, json current_tree) :  ltree(last_tree), ctree(current_tree), fname(file_name){
    if(!std::filesystem::exists(file_name.parent_path())){
        throw std::runtime_error("Json dif parent dir doesn't exists");
    }
    std::ofstream fout(file_name, std::ios::out);
    fout << "{}";
    fout.close();
    std::ifstream fin(file_name, std::ios::in);
    fin >> dif;
}
void Difference::calculate_and_write_difference(){
    // if(ltree.size() != ctree.size()){
    //     throw std::runtime_error("Initial json tree should be the same size");
    // }
    for(auto &[dir_name, dir] : ctree.items()){
        if(ltree.contains(dir_name)){
            calc_dif(ltree.at(dir_name), ctree.at(dir_name), dif, dir_name);
        }
        else{
            json empty_json;
            empty_json[constants::JSON_FIELD_NAME_TYPE] = constants::UNIT_TYPE_EMPTY_DIR;
            empty_json[constants::JSON_FIELD_NAME_HASH] = json::array();
            calc_dif(empty_json, ctree.at(dir_name), dif, dir_name);
        }
    }
    std::ofstream fout(fname, std::ios::out);
    std::string jstring = dif.dump();
    fout.write(jstring.c_str(), jstring.size());
}


std::array<unsigned char, constants::HASH_BYTE_LENGTH> Difference::get_hash_from_json(const json &j){
    std::array<unsigned char, constants::HASH_BYTE_LENGTH> hash;
    for(int i = 0; i < constants::HASH_BYTE_LENGTH; i++){
        try{
            hash[i] = j.at(constants::JSON_FIELD_NAME_HASH).at(i).get<unsigned char>();
        }
        catch(json::out_of_range &e){
            throw std::runtime_error("Can't find /hash json data or index");
        }
    }
    return hash;   
}

bool Difference::equal_unit(const json& l,const json& c){
    return get_hash_from_json(l) == get_hash_from_json(c);
}

std::vector<std::string> Difference::get_keys(json j){
    std::vector<std::string> keys;
    for(auto &[key, val] : j.items()){
        keys.push_back(key);
    }
    return keys;
}

std::vector<std::string> Difference::exluding_dif(std::vector<std::string> m1, std::vector<std::string> m2){
    std::vector<std::string> dif;
    for(auto s : m1){
        if(std::find(m2.begin(), m2.end(), s.c_str()) == m2.end()){
            dif.push_back(s);
        }
    }
    return dif;
}

// возращает те элементы, что есть в первом, но нет во втром
std::vector<std::string> Difference::exluding_dif(json m1, json m2){
    return exluding_dif(get_keys(m1), get_keys(m2));
}

std::vector<std::string> Difference::shared_elements(std::vector<std::string> m1, std::vector<std::string> m2){
    std::vector<std::string> elts;
    for(auto s : m1){
        if(std::find(m2.begin(), m2.end(), s.c_str()) != m2.end()){
            elts.push_back(s);
        }
    }
    return elts;
}

std::vector<std::string> Difference::shared_elements(json m1, json m2){    
    return shared_elements(get_keys(m1), get_keys(m2));
}

    void Difference::calc_dif(json lj, json cj, json &res, std::string res_name){
        if(lj.at(constants::JSON_FIELD_NAME_TYPE) == constants::UNIT_TYPE_EMPTY_DIR){
            res[res_name][constants::JSON_FIELD_NAME_TYPE] = constants::UNIT_TYPE_DIR;
        }
        else{
            res[res_name][constants::JSON_FIELD_NAME_TYPE] = lj.at(constants::JSON_FIELD_NAME_TYPE);
        }
        if(lj.is_array() || lj.is_string()){
            return;
        }
        if(lj.at(constants::JSON_FIELD_NAME_TYPE) == constants::UNIT_TYPE_FILE){
            res[res_name][constants::JSON_FIELD_NAME_MOD_TYPE] = constants::MODIFIED_UNIT;
        }
        else{
            res[res_name][constants::JSON_FIELD_NAME_MOD_TYPE] = constants::MODIFIED_UNIT;
            std::vector<std::string> deleted_files = exluding_dif(lj, cj);
            std::vector<std::string> added_files = exluding_dif(cj, lj);
            for(std::string &name : deleted_files){
                res[res_name][name][constants::JSON_FIELD_NAME_MOD_TYPE] = constants::DELETED_UNIT;
                res[res_name][name][constants::JSON_FIELD_NAME_TYPE] = lj.at(name).at(constants::JSON_FIELD_NAME_TYPE);
            }
            for(std::string &name : added_files){
                res[res_name][name][constants::JSON_FIELD_NAME_MOD_TYPE] = constants::ADDED_UNIT;
                res[res_name][name][constants::JSON_FIELD_NAME_TYPE] = cj.at(name).at(constants::JSON_FIELD_NAME_TYPE);
            }
            std::vector<std::string> shared_files = shared_elements(lj, cj);
            // Модификация файлов
            for(auto &key : shared_files){
                if(lj.at(key).is_array() || lj.at(key).is_string()){
                    continue;
                }
                if(!equal_unit(lj.at(key), cj.at(key))){
                    res[res_name][key] = json::object();
                    calc_dif(lj.at(key), cj.at(key), res[res_name], key);
                }
            }
        }
}
