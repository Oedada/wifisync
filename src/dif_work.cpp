#include <filesystem>
#include <fstream>
#include "constants.hpp"
#include "difference.hpp"
#include "environment.hpp"
#include "files_opers.hpp"
#include "utils.hpp"
#include "dif_work.hpp"

namespace fs = std::filesystem;

    DifWork::DifWork(std::string uuid) : other_uuid(uuid){
        own_name = env::get_name();
        own_uuid = env::get_uuid();
        detecting_paths = read_paths_from_file(env::get_data_path(constants::DATA_DIR) / constants::DETECTING_UNITS_FILENAME, true);
        ignoring_paths = read_paths_from_file(env::get_data_path(constants::DATA_DIR) / constants::IGNORING_UNITS_FILENAME, true);
    }


    void DifWork::add_to_sync(fs::path path){
        if(std::find(detecting_paths.begin(), detecting_paths.end(), (std::string)path) == detecting_paths.end()){
            if(fs::exists(path)){
                detecting_paths.push_back(path);
                fs::path data = env::get_data_path(constants::DATA_DIR);
                std::ofstream fout(data / constants::DETECTING_UNITS_FILENAME, std::ios::app);
                fout << path.c_str();
                fout.close();
            }
        }
    }   

    void DifWork::add_to_ignore(fs::path path){
        if(std::find(ignoring_paths.begin(), ignoring_paths.end(), (std::string)path) == ignoring_paths.end()){
            if(fs::exists(path)){
                ignoring_paths.push_back(path);
                std::ofstream fout(env::get_data_path(constants::DATA_DIR) / constants::IGNORING_UNITS_FILENAME, std::ios::app);
                fout << path.c_str();
                fout.close();
            }
        }
    }

    std::vector<std::string> DifWork::read_paths_from_file(fs::path path, bool create){
        if(!fs::exists(path) && create){
            std::ofstream fout(path);
            fout.close();
        }
        std::ifstream fin(path);
        if(!fin){
            throw std::runtime_error("Can't opend path");
        }
        std::string str;
        std::vector<std::string> strings;
        while(std::getline(fin, str)){
            fs::path w = fs::weakly_canonical(str);
            if(fs::exists(w)){
                strings.push_back(w);
            }
        }
        return strings;
    }

    void DifWork::shift_snapshots(){
        fs::path last_shapshot_path = env::get_data_path(constants::DATA_DIR) / constants::LAST_SNAPSHOT_FILENAME;
        fs::path cur_snapshot_path = env::get_data_path(constants::DATA_DIR) / constants::CURRENT_SNAPSHOT_FILENAME;
        if(fs::exists(cur_snapshot_path)){
            Units cur_snapshot(cur_snapshot_path, false);
            for(const auto &path : ignoring_paths){
                if(cur_snapshot.is_registred(path)){
                    cur_snapshot.rm_unit(path);
                }
            }
            for(const auto &path :detecting_paths){
                if(!cur_snapshot.is_registred(path)){
                    cur_snapshot.json_tree[path] = json::object();
                    if(fs::is_regular_file(path)){
                        cur_snapshot.json_tree[path][constants::JSON_FIELD_NAME_TYPE] = constants::UNIT_TYPE_FILE;
                    }
                    else if(fs::is_directory(path)){
                        cur_snapshot.json_tree[path][constants::JSON_FIELD_NAME_TYPE] = constants::UNIT_TYPE_DIR;
                    }
                    for(int i = 0; i < constants::HASH_BYTE_LENGTH; i++){
                        cur_snapshot.json_tree[path][constants::JSON_FIELD_NAME_HASH][i] = 0;
                    }
                    cur_snapshot.save();
                }
            }
            fs::copy_file(cur_snapshot_path, last_shapshot_path, fs::copy_options::overwrite_existing);
        }
        Units cur_snapshot(cur_snapshot_path, true);
        for(const auto &path : detecting_paths){
            cur_snapshot.set_unit(path, true);
        }
        for(const auto &path : ignoring_paths){
            if(fs::is_regular_file(path)){
                cur_snapshot.rm_unit(path);
            }
        }
        if(!fs::exists(last_shapshot_path)){
            fs::copy_file(cur_snapshot_path, last_shapshot_path, fs::copy_options::overwrite_existing);
        }
    }

    void DifWork::calculate_dif(){
        fs::path data_dir = env::get_data_path(constants::DATA_DIR);
        Units last_snapshot(data_dir / constants::LAST_SNAPSHOT_FILENAME, false);
        Units cur_snapshot(data_dir / constants::CURRENT_SNAPSHOT_FILENAME, false);
        json devices_path_table = read_json(env::get_data_path() / constants::DEVICES_FILE, false);
        if(devices_path_table[other_uuid]["first_connect"]){
            Difference dif(data_dir / constants::DIFFERENCE_FILENAME, {}, cur_snapshot.json_tree);
            dif.calculate_and_write_difference();
            devices_path_table[other_uuid]["first_connect"] = false;
            write_json(env::get_data_path() / constants::DEVICES_FILE, devices_path_table);
        }
        else{
            Difference dif(data_dir / constants::DIFFERENCE_FILENAME, last_snapshot.json_tree, cur_snapshot.json_tree);
            dif.calculate_and_write_difference();
        }
        json difference = read_json(data_dir / constants::DIFFERENCE_FILENAME, false);
        for(const auto &[key, val] : difference.items()){
            if(key == constants::JSON_FIELD_NAME_TYPE){
                continue;
            }
            if(!devices_path_table[other_uuid]["paths"].contains(key)){
                std::cerr << "There is no corresponding path to the path " << key;
                difference[key]["/other_path"] = "/None";
            }
            else{
                difference[key]["/other_path"] = devices_path_table[other_uuid]["paths"][key];
            }
        }
        write_json(data_dir / constants::DIFFERENCE_FILENAME, difference);
    }
