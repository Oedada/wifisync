#include <iostream>
#include "hash.hpp"
#include <fstream>
#include <filesystem>
#include "files_opers.hpp"
using json = nlohmann::json;

void test_hash(){
    Hash h;
    std::string hello = "hello";
    std::vector<unsigned char> msg(hello.begin(), hello.end());
    h.update(msg);
    h.calculate();
    std::cout << h.to_hex();
}

void test_files(){
    Units plugins("data/files.json");
    try{
        plugins.set_unit("data/train");
    }
    catch(std::runtime_error e){
        std::cerr << "WARN: " << e.what();
    }
    plugins.rm_unit("data/train/LuckPerms/translations/repository");
    json configs;
    plugins.get_unit("data/train/LuckPerms", configs);
    std::string json_string = configs.dump();
    std::ofstream fout("test.json");
    fout.write(json_string.c_str(), json_string.size());
}

int main(){
    test_files();
    return 0;
}
