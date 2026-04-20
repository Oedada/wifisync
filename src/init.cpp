#include <iostream>
#include "init.hpp"
#include "uuid.h"
#include "constants.hpp"
#include "environment.hpp"
#include <fstream>
#include <random>

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
    calculate_and_write_uuid();
}
