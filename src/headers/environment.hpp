#include "constants.hpp"
#include <SDL2/SDL.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
namespace fs = std::filesystem;

namespace env{
    // paths
    inline const fs::path get_data_path(){
        static fs::path root_data = []{
            char* char_path = SDL_GetPrefPath(constants::author_name.c_str(), constants::project_name.c_str());
            if(!char_path){
                throw std::runtime_error("Can't get data path");
            }
            fs::path path = char_path;
            SDL_free(char_path);
            return path;
        }();
        return root_data;
    }
    inline const fs::path get_data_path(std::string sub_name){
        static fs::path root_data = []{
            char* char_path = SDL_GetPrefPath(constants::author_name.c_str(), constants::project_name.c_str());
            if(!char_path){
                throw std::runtime_error("Can't get data path");
            }
            fs::path path = char_path;
            SDL_free(char_path);
            return path;
        }();
        if(!fs::path(sub_name).has_extension()){
            fs::create_directory(root_data / sub_name);
        }
        return root_data / sub_name;
    }
    inline const std::string get_uuid(){
        std::ifstream fin(get_data_path(constants::UUID_FILE));
        if(!fin){
            std::cerr << "aaaaaaaaaa";
        }
        std::stringstream buf;
        buf << fin.rdbuf();
        return buf.str();
    }
    inline const std::string get_name(){
        std::ifstream fin(get_data_path(constants::CONFIG_FILE));
        json cfg;
        fin >> cfg;
        return cfg.at("name").get<std::string>();
    }
}
