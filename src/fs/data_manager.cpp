#include "fs/data_manager.hpp"
#include "fs/environment.hpp"
#include "util/constants.hpp"
#include "util/utils.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <sys/types.h>
#include <vector>
namespace fs = std::filesystem;
using json = nlohmann::json;

DataManager::DataManager() {
    try {
        config = json::parse(FileAccess::get_fin(constants::paths::config));
    } catch (const json::parse_error) {
        config = Config();
    } catch (fs::filesystem_error) {
        config = Config();
    } catch (std::runtime_error &e){
        config = Config();
    }
}

void DataManager::save_config() {
    json out = config;
    logmsg(out.dump(4));
    std::ofstream fout = FileAccess::get_fout(constants::paths::config);
    fout << out.dump(4);
}
void DataManager::add_watch(const fs::path &path) {
    config.paths.watch.push_back(path.string());
    save_config();
}
void DataManager::add_ignore(const fs::path &path) {
    config.paths.ignore.push_back(path.string());
    save_config();
}
void DataManager::rm_watch(const fs::path &path) {
    std::erase(config.paths.watch, path.string());
    save_config();
}
void DataManager::rm_ignore(const fs::path &path) {
    std::erase(config.paths.ignore, path.string());
    save_config();
}
std::vector<std::string> DataManager::list_ignore() const {
    return config.paths.ignore;
}
std::vector<std::string> DataManager::list_watch() const {
    return config.paths.watch;
}

void DataManager::set_name(const std::string &name) {
    config.name = name;
    save_config();
}

std::string DataManager::get_name() const { return config.name; }

void DataManager::set_uuid(const std::string &uuid) {
    config.uuid = uuid;
    save_config();
}

std::string DataManager::get_uuid() const { return config.uuid; }

DeviceManager::DeviceManager() {
    try {
        devices =
            json::parse(FileAccess::get_fin(constants::paths::data::devices));
    } catch (const json::parse_error) {
        devices = {};
    } catch (fs::filesystem_error) {
        devices = {};
    } catch (std::runtime_error &e){
        devices = {};
    }
}

void DeviceManager::save_devices() {
    json j = devices;
    std::ofstream fout = FileAccess::get_fout(constants::paths::data::devices);
    fout << j;
}
void DeviceManager::add_device(const std::string &uuid,
                               const std::string &name) {
    Device d;
    d.name = name;
    devices[uuid] = d;
    save_devices();
}

Device DeviceManager::get_device(const std::string &uuid){
    return devices[uuid];
}

bool DeviceManager::is_first(const std::string &uuid){
    return devices[uuid].is_first;
}

void DeviceManager::set_is_first(const std::string &uuid, const bool is_first){
    devices[uuid].is_first = is_first;
}

void DeviceManager::rm_device(const std::string &uuid) {
    devices.erase(uuid);
    save_devices();
}

int DeviceManager::set_path_pair_to_device(const std::string &uuid,
                                           const fs::path &own_path,
                                           const fs::path &other_path) {
    if (!devices.contains(uuid)) {
        return 1;
    }
    devices[uuid].paths_pairs[own_path.string()] = other_path.string();
    save_devices();
    return 0;
}

int DeviceManager::rm_path_pair_from_device(const std::string &uuid,
                                            const fs::path &own_path) {
    if (!devices.contains(uuid)) {
        return 1;
    }
    devices[uuid].paths_pairs.erase(own_path.string());
    save_devices();
    return 0;
}

std::vector<std::string> DeviceManager::uuid_list() const {
    std::vector<std::string> keys;
    for (const auto &[key, _] : devices) {
        keys.push_back(key);
    }
    return keys;
}
