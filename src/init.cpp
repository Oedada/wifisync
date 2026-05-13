#include "init.hpp"
#include "uuid.h"
#include "constants.hpp"
#include "utils.hpp"
#include "environment.hpp"
#include <filesystem>
#include <fstream>
#include <random>

void create_file_structure(){
    fs::path root_path = env::get_data_path();
    logmsg(root_path);
    if(!fs::exists(root_path / constants::DEVICES_FILE)){
        write_json(root_path / constants::DEVICES_FILE, {});
    }
    if(!fs::exists(root_path / constants::CONFIG_FILE)){
        json cfg;
        cfg["name"] = "tmp";
        cfg["tmp_name"] = true;
        write_json(root_path / constants::CONFIG_FILE, cfg);
    }
    auto data_dir = env::get_data_path(constants::DATA_DIR);
    if(!fs::exists(data_dir / constants::DETECTING_UNITS_FILENAME)){
        logmsg(data_dir / constants::DETECTING_UNITS_FILENAME);
        std::ofstream fout(data_dir / constants::DETECTING_UNITS_FILENAME);
    }
    if(!fs::exists(data_dir / constants::IGNORING_UNITS_FILENAME)){
        logmsg(data_dir / constants::IGNORING_UNITS_FILENAME);
        std::ofstream fout(data_dir / constants::IGNORING_UNITS_FILENAME);
    }
}

void calculate_and_write_uuid(){
    fs::path file_path = env::get_data_path(constants::UUID_FILE);
    if(!fs::exists(file_path)){
        std::ofstream fout(file_path);
        std::mt19937 rng{std::random_device{}()};
        uuids::uuid_random_generator gen{ rng };
        uuids::uuid id = gen();
        fout << uuids::to_string(id);
        fout.close();
    }
}

void init(){
    create_file_structure();
    calculate_and_write_uuid();
}
