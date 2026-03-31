#include "nlohmann/json.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include "headers/files_opers.hpp"
#include "headers/utils.hpp"

using json = nlohmann::json;
const int HASH_BYTE_LENGHT = 32;

class Difference{
    public: 
        json ltree;
        json ctree;
        json dif;
        std::filesystem::path fname;
        Difference(std::filesystem::path file_name, json last_tree, json current_tree) : fname(file_name), ltree(last_tree), ctree(current_tree){
            if(!std::filesystem::exists(file_name.parent_path())){
                throw std::runtime_error("Json dif parent dir doesn't exists");
            }
            std::ofstream fout(file_name, std::ios::out);
            fout.write("{}", 2);
            fout.close();
            std::ifstream fin(file_name, std::ios::in);
            fin >> dif;
        }
        void calculate_difference(){
            if(ltree.size() != ctree.size()){
                throw std::runtime_error("Initial json tree should be the same size");
            }
            for(auto &[dir_name, dir] : ctree.items()){
                calc_dif(ltree[dir_name], ctree[dir_name], dif, dir_name);
            }
            std::ofstream fout(fname, std::ios::out);
            std::string jstring = dif.dump();
            fout.write(jstring.c_str(), jstring.size());
        }
        ~Difference(){

        }


    std::array<unsigned char, HASH_BYTE_LENGHT> get_hash_from_json(const json &j){
        std::array<unsigned char, HASH_BYTE_LENGHT> hash;
        for(int i = 0; i < HASH_BYTE_LENGHT; i++){
            try{
                hash[i] = j.at("/hash").at(i).get<unsigned char>();
            }
            catch(json::out_of_range &e){
                std::cerr << "Can't find /hash json data or index";
            }
        }
        return hash;   
    }

    bool equal_unit(json l, json c){
        return get_hash_from_json(l) == get_hash_from_json(c);
    }

    std::vector<std::string> get_keys(json j){
        std::vector<std::string> keys;
        for(auto &[key, val] : j.items()){
            keys.push_back(key);
        }
        return keys;
    }

    std::vector<std::string> exluding_dif(std::vector<std::string> m1, std::vector<std::string> m2){
        std::vector<std::string> dif;
        for(auto s : m1){
            if(std::find(m2.begin(), m2.end(), s.c_str()) == m2.end()){
                dif.push_back(s);
            }
        }
        return dif;
    }

    // возращает те элементы, что есть в первом, но нет во втром
    std::vector<std::string> exluding_dif(json m1, json m2){
        return exluding_dif(get_keys(m1), get_keys(m2));
    }

    std::vector<std::string> shared_elements(std::vector<std::string> m1, std::vector<std::string> m2){
        std::vector<std::string> elts;
        for(auto s : m1){
            if(std::find(m2.begin(), m2.end(), s.c_str()) != m2.end()){
                elts.push_back(s);
            }
        }
        return elts;
    }

    std::vector<std::string> shared_elements(json m1, json m2){    
        return shared_elements(get_keys(m1), get_keys(m2));
    }

    void calc_dif(json lj, json cj, json &res, std::string res_name){
        if(lj.is_array() || lj.is_string()){
            return;
        }
        if(lj.at("/type") == "file"){
            res[res_name] = "M";
        }
        else{
            std::vector<std::string> deleted_files = exluding_dif(lj, cj);
            std::vector<std::string> added_files = exluding_dif(cj, lj);
            for(std::string &name : deleted_files){
                res[res_name][name] = "D";
            }
            for(std::string &name : added_files){
                res[res_name][name] = "A";
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
};

int main(){
    Units cur_dir("data/ctest.json", true);
    cur_dir.set_unit("data/train", false);
    Units last_dir("data/ltest.json", false);
    Difference dif("data/test_dif.json", last_dir.json_tree, cur_dir.json_tree);
    dif.calculate_difference();
}
