#include "util/init.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "util/utils.hpp"
#include "fs/data_manager.hpp"
#include "util/utils.hpp"
#include "uuid.h"
#include <fstream>
#include <memory>
#include <random>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

void register_loggers(){
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("(%l)[%H:%M:%S] [%n, %t] %v");
}

void calculate_and_set_uuid() {
    if (DataManager::get_uuid() == "") {
        std::mt19937 rng{std::random_device{}()};
        uuids::uuid_random_generator gen{rng};
        uuids::uuid id = gen();
        DataManager::set_uuid(uuids::to_string(id));
        logmsg("uuid: " + DataManager::get_uuid());
    }
}

void get_and_set_name() {
    if (DataManager::get_name() == "") {
        std::ifstream fin("/etc/hostname", std::ios::in);
        std::string name;
        std::getline(fin, name);
        DataManager::set_name(name);
    }
}

void init() {
    calculate_and_set_uuid();
    get_and_set_name();
}
