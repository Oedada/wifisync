#include "util/init.hpp"
#include "spdlog/logger.h"
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
    // std::vector<std::shared_ptr<spdlog::sinks::stdout_color_sink_mt>> sinks;
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%n, %t] [%H:%M:%S] (%l) %v");
    std::vector<std::shared_ptr<spdlog::logger>> loggers;
    std::vector<std::string> loggers_names;
    loggers_names.push_back("session");
    loggers_names.push_back("tcpsocket");
    loggers_names.push_back("main");
    loggers_names.push_back("difwork");
    loggers_names.push_back("transport");
    loggers_names.push_back("difwalk");
    loggers_names.push_back("default");
    loggers_names.push_back("init");
    for(auto logger_name : loggers_names){
        loggers.push_back(std::make_shared<spdlog::logger>(logger_name, console_sink));
        spdlog::register_logger(loggers[loggers.size()-1]);
    }
}

void calculate_and_set_uuid() {
    if (DataManager::get_uuid() == "") {
        std::mt19937 rng{std::random_device{}()};
        uuids::uuid_random_generator gen{rng};
        uuids::uuid id = gen();
        DataManager::set_uuid(uuids::to_string(id));
        spdlog::get("init")->info("Created uuid: {}", DataManager::get_uuid());
    }
}

void get_and_set_name() {
    if (DataManager::get_name() == "") {
        std::ifstream fin("/etc/hostname", std::ios::in);
        std::string name;
        std::getline(fin, name);
        DataManager::set_name(name);
        spdlog::get("init")->info("Created name: {}", DataManager::get_name());
    }
}

void init() {
    register_loggers();
    calculate_and_set_uuid();
    get_and_set_name();
}
