#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
using json = nlohmann::json;
namespace fs = std::filesystem;

class Units{
    public:

        fs::path json_path;
        json units_list;
        Units(const fs::path jp) : json_path(jp){
            if(!fs::exists(json_path)){
                std::ofstream fout(json_path, std::ios::out);
                fout.write("{}", 2);
            }

            std::ifstream fin(json_path, std::ios::in);
            fin >> units_list;
        }

        void set_unit(const fs::path &dp){
            fs::path adp = fs::absolute(dp);
            if(!fs::exists(adp)){
                throw std::runtime_error(std::string("Directory ") + adp.string() + " not exists");
            } 
            if(is_registred(adp)){
                throw std::runtime_error(std::string("Unit ") + adp.string() + " is already registred");
            }
            create_json_file_list(adp, units_list[adp]);
        }

        void get_unit(const fs::path p, json &unit){
            json *pointer;
            if(get_unit_from_path(p, pointer))
                unit = *pointer;
            else
                throw std::runtime_error("This unit not registred");
        }

        void rm_unit(const fs::path dp){
            json* parent;
            get_unit_from_path(dp.parent_path().string(), parent);
            parent->erase(dp.filename().c_str());
        }

        bool is_registred(const fs::path p){
            json* pointer;
            return get_unit_from_path(p, pointer);
        }

        ~Units(){
            std::ofstream fout(json_path, std::ios::out);
            std::string jstring = units_list.dump();
            fout.write(jstring.c_str(), jstring.size());
        }

    private:

        bool get_unit_from_path(const std::string &p, json* &unit_path_part_pointer){
            std::string ap = fs::absolute(p).string();
            std::string path_in_units;
            for( auto& [key, value] : units_list.items()){
                auto pos = ap.find(key);
                if(pos != std::string::npos && pos == 0){
                    path_in_units = key;
                }
            }
            if(path_in_units.empty()){
                std::cerr << "Путь не зарегестрирован в системе\n";
                return false;
            }
            std::string path_without_root = ap;
            path_without_root.erase(0, path_in_units.size());
            unit_path_part_pointer = &units_list[path_in_units];
            for(auto &part : fs::path(path_without_root)){
                if(part == "/")
                    continue;
                try{
                    unit_path_part_pointer = &((*unit_path_part_pointer).at(part));
                }
                catch(...){
                    std::cerr << "Путь слишком длинный" << "\n";
                    return false;
                }
            }
            return true;

        }

        void create_json_file_list(const fs::path path, json& json_field){
            std::vector<fs::path> units;
            if(fs::is_directory(path)){
                if(fs::is_empty(path))
                    json_field = "/empty_dir";
                for(const fs::path& entry: fs::directory_iterator(path))
                    create_json_file_list(entry, json_field[entry.filename().c_str()]);
            }
            else
                json_field = "/file";
        }
};

int main(){
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
    return 0;
}
