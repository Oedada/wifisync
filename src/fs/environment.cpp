#include "fs/environment.hpp"
#include "util/constants.hpp"
#include "util/utils.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace fs = std::filesystem;

fs::path get_and_create_if_not_exists_base_path() {
    static fs::path base_path;
    if (is_dryrun()) {
        base_path = dryrun_root_path();
    } else {
        base_path = [] {
            fs::path base_path;
            char *data_home = getenv("XDG_DATA_HOME");
            if (data_home == NULL) {
                base_path =
                    fs::path(getenv("HOME")) / "/.local/share/wifisync/";
            } else {
                base_path = fs::path(data_home) / "/wifisync/";
            }
            return base_path;
        }();
    }
    fs::create_directories(base_path);
    return base_path;
}
std::ifstream FileAccess::get_fin(std::string_view path, bool bin_mode) {
    if (!fs::exists(get_and_create_if_not_exists_base_path() / path)) {
        throw std::runtime_error("File " + std::string(path) + "doesn't exist");
    }
    return std::ifstream(get_and_create_if_not_exists_base_path() / path,
                         bin_mode ? std::ios::binary : std::ios::in);
}

std::ifstream FileAccess::get_fin(std::string_view path) {
    return FileAccess::get_fin(path, false);
}

std::ofstream FileAccess::get_fout(std::string_view path, bool bin_mode) {
    return std::ofstream(get_and_create_if_not_exists_base_path() / path,
                         bin_mode ? std::ios::binary : std::ios::out);
}

std::ofstream FileAccess::get_fout(std::string_view path) {
    return FileAccess::get_fout(path, false);
}

bool FileAccess::is_directory(std::string_view rel_path) {
    if (!fs::exists(get_and_create_if_not_exists_base_path() / rel_path)) {
        return false;
    }
    return fs::is_directory(get_and_create_if_not_exists_base_path() /
                            rel_path);
}

bool FileAccess::is_file(std::string_view rel_path) {
    if (!fs::exists(get_and_create_if_not_exists_base_path() / rel_path)) {
        return false;
    }
    return fs::is_regular_file(get_and_create_if_not_exists_base_path() /
                               rel_path);
}

void FileAccess::mkdir(std::string_view rel_path) {
    fs::create_directories(rel_path);
}

void FileAccess::write_json(std::string_view rel_path, nlohmann::json j) {
    FileAccess::get_fout(rel_path) << j;
}

nlohmann::json FileAccess::read_json(std::string_view rel_path) {
    return nlohmann::json::parse(FileAccess::get_fin(rel_path));
}

bool is_env_exists(const std::string &name) {
    return std::getenv(name.c_str()) != nullptr;
}

std::string get_env(const std::string &name) {
    auto env = std::getenv(name.c_str());
    return env ? env : "";
}

bool is_dryrun() {
    if (is_env_exists(constants::env_dryrun)) {
        if (get_env(constants::env_dryrun) == "on") {
            return true;
        }
    }
    return false;
}

std::string dryrun_root_path() {
    if (is_dryrun()) {
        if (is_env_exists(constants::env_dryrun_root_path)) {
            return get_env(constants::env_dryrun_root_path);
        }
        throw std::runtime_error("Can't get dryrun path");
    }
    throw std::runtime_error("Can't get dryrun path, dryrun isn't on");
}
