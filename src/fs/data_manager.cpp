#include "fs/data_manager.hpp"
#include "fs/environment.hpp"
#include "util/constants.hpp"
#include "util/utils.hpp"
#include <filesystem>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <sys/types.h>
#include <vector>
namespace fs = std::filesystem;
using json = nlohmann::json;

void DataManager::load_config() {
    try {
        config = json::parse(FileAccess::get_fin(constants::paths::config));
    } catch (const json::parse_error) {
        config = Config();
    } catch (fs::filesystem_error) {
        config = Config();
    } catch (std::runtime_error &e) {
        config = Config();
    }
}

void DataManager::save_config() {
    json out = config;
    std::ofstream fout = FileAccess::get_fout(constants::paths::config);
    fout << out.dump(4);
}
void DataManager::add_watch(const fs::path &path) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!is_load) {
        load_config();
        is_load = true;
    }
    config.paths.watch.push_back(path.string());
    save_config();
}
void DataManager::add_ignore(const fs::path &path) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!is_load) {
        load_config();
        is_load = true;
    }
    config.paths.ignore.push_back(path.string());
    save_config();
}
void DataManager::rm_watch(const fs::path &path) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!is_load) {
        load_config();
        is_load = true;
    }
    std::erase(config.paths.watch, path.string());
    save_config();
}
void DataManager::rm_ignore(const fs::path &path) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!is_load) {
        load_config();
        is_load = true;
    }
    std::erase(config.paths.ignore, path.string());
    save_config();
}
std::vector<std::string> DataManager::list_ignore() {
    std::lock_guard<std::mutex> lock(mtx);
    if (!is_load) {
        load_config();
        is_load = true;
    }
    return config.paths.ignore;
}
std::vector<std::string> DataManager::list_watch() {
    std::lock_guard<std::mutex> lock(mtx);
    if (!is_load) {
        load_config();
        is_load = true;
    }
    return config.paths.watch;
}

void DataManager::set_name(const std::string &name) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!is_load) {
        load_config();
        is_load = true;
    }
    config.name = name;
    save_config();
}

std::string DataManager::get_name() {
    std::lock_guard<std::mutex> lock(mtx);
    if (!is_load) {
        load_config();
        is_load = true;
    }
    return config.name;
}

void DataManager::set_uuid(const std::string &uuid) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!is_load) {
        load_config();
        is_load = true;
    }
    config.uuid = uuid;
    save_config();
}

std::string DataManager::get_uuid() {
    std::lock_guard<std::mutex> lock(mtx);
    if (!is_load) {
        load_config();
        is_load = true;
    }
    return config.uuid;
}

void DeviceManager::load_devices() {
    try {
        devices =
            json::parse(FileAccess::get_fin(constants::paths::data::devices));
    } catch (const json::parse_error) {
        logerr("Json read error");
        devices = {};
    } catch (fs::filesystem_error) {
        logerr("Json read error");
        devices = {};
    } catch (std::runtime_error &e) {
        logerr("Json read error");
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
    std::lock_guard<std::mutex> lock(mtx);
    load_devices();
    if (devices.contains(uuid)) {
        throw std::runtime_error("Device already exists");
    }
    Device d;
    d.name = name;
    devices[uuid] = d;
    d.is_first = true;
    save_devices();
}

Device DeviceManager::get_device(const std::string &uuid) {
    std::lock_guard<std::mutex> lock(mtx);
    load_devices();
    return devices[uuid];
}

bool DeviceManager::is_first(const std::string &uuid) {
    std::lock_guard<std::mutex> lock(mtx);
    load_devices();
    return devices[uuid].is_first;
}

void DeviceManager::set_is_first(const std::string &uuid, const bool is_first) {
    std::lock_guard<std::mutex> lock(mtx);
    load_devices();
    devices[uuid].is_first = is_first;
    save_devices();
}

void DeviceManager::rm_device(const std::string &uuid) {
    std::lock_guard<std::mutex> lock(mtx);
    load_devices();
    devices.erase(uuid);
    save_devices();
}

int DeviceManager::set_path_pair_to_device(const std::string &uuid,
                                           const fs::path &own_path,
                                           const fs::path &other_path) {
    std::lock_guard<std::mutex> lock(mtx);
    load_devices();
    if (!devices.contains(uuid)) {
        return 1;
    }
    devices[uuid].paths_pairs[fs::weakly_canonical(own_path).string()] =
        fs::weakly_canonical(other_path).string();
    save_devices();
    return 0;
}

int DeviceManager::rm_path_pair_from_device(const std::string &uuid,
                                            const fs::path &own_path) {
    std::lock_guard<std::mutex> lock(mtx);
    load_devices();
    if (!devices.contains(uuid)) {
        return 1;
    }
    devices[uuid].paths_pairs.erase(own_path.string());
    save_devices();
    return 0;
}

std::vector<std::string> DeviceManager::uuid_list() {
    std::lock_guard<std::mutex> lock(mtx);
    load_devices();
    std::vector<std::string> keys;
    for (const auto &[key, _] : devices) {
        keys.push_back(key);
    }
    return keys;
}
