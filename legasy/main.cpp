#include <filesystem>
#include <fstream>
#include <iostream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

bool get_json(fs::path &p, json &file){
    std::ifstream fin("/home/oedada/Projects/apps/Wifisync/wifisync/data/files.json");
    json json_tree = json(fin);

}

int main(){
    fs::path true_path = "/home/oedada/Projects/apps/Wifisync/wifisync/data/train/Brewery";
    fs::path wrong_path = "/home/oedada/Projects/apps/Wifisync/wifisync/data/train/Brewery";

    for(auto &part : p){
        std::cout << part.string();
    }
}
