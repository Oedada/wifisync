#include "constants.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>
// #include <iostream>
namespace fs = std::filesystem;
using json = nlohmann::json;

void logwarn(std::string warning){
    std::cout << "[Warning]: " << warning << std::endl;
}

void logerr(std::string error){
    std::cerr << "[Error]: " << error << std::endl;
}

void logmsg(std::string msg){
    std::cout << "[Dev]: " << msg << std::endl;
}

uint64_t count_subelements(json node){
    uint64_t count = 0;
    for(const auto & [key, val]: node.items()){
        if(key != constants::JSON_FIELD_NAME_TYPE && key != constants::JSON_FIELD_NAME_MOD_TYPE && key != constants::JSON_FIELD_NAME_OTHER_PATH){
            count++;
        }
    }
    return count;
}

void catch_error(int return_code,std::string error_message){
    if(return_code != 1){
        throw std::runtime_error(error_message);
    }
}

void toBytes(uint64_t x, unsigned char* out_b){
    for(int i = 0; i < 8; i++){
        out_b[7-i] = (x >> i*8) & 0xFF;
    }
}

uint64_t fromBytes64(unsigned char* in_b){
    uint64_t x = 0;
    for(int i = 0; i < 8; i++){
        x |= (uint64_t)in_b[7-i] << i*8;
    }
    return x;
}

void toBytes(uint16_t x, unsigned char* out_b){
    for(int i = 0; i < 2; i++){
        out_b[1-i] = (x >> i*2) & 0xFF;
    }
}

uint16_t fromBytes16(unsigned char* in_b){
    uint16_t x = 0;
    for(int i = 0; i < 2; i++){
        x |= (uint16_t)in_b[1-i] << i*2;
    }
    return x;
}

json read_json(fs::path path, bool not_exists_create){
    if(!fs::exists(path)){
        if(not_exists_create){
            std::fstream fout(path);
            fout << "{}";
        }
        if(!fs::is_regular_file(path)){
            throw std::runtime_error(std::string("Can't read json, file is uncorrect: ") + std::string(path));
        }
    }
    json j;
    std::ifstream fin(path);
    fin >> j;
    return j;
}

void write_json(fs::path path, json j){
    if(!fs::exists(path.parent_path())){
        throw std::runtime_error("Can't write json, file dir is uncorrect");
    }
    std::ofstream fout(path);
    std::string jstring = j.dump();
    fout.write(jstring.c_str(), jstring.size());
}
