#include "util/init.hpp"
#include "fs/data_manager.hpp"
#include "util/utils.hpp"
#include "uuid.h"
#include <fstream>
#include <random>

using json = nlohmann::json;

void calculate_and_set_uuid(DataManager dm) {
    if (dm.get_uuid() == "") {
        std::mt19937 rng{std::random_device{}()};
        uuids::uuid_random_generator gen{rng};
        uuids::uuid id = gen();
        dm.set_uuid(uuids::to_string(id));
    }
}

void get_and_set_name(DataManager dm) {
    if (dm.get_name() == "") {
        std::ifstream fin("/etc/hostname", std::ios::in);
        std::string name;
        std::getline(fin, name);
        dm.set_name(name);
    }
}

void init() {
    DataManager dm;
    calculate_and_set_uuid(dm);
    get_and_set_name(dm);
}
