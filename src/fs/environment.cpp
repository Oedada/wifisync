#include "fs/environment.hpp"
#include <nlohmann/json.hpp>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

fs::path get_and_create_if_not_exists_base_path() {
    static fs::path base_path = [] {
        fs::path base_path;
        char *data_home = getenv("XDG_DATA_HOME");
        if (data_home == NULL) {
            base_path = fs::path(getenv("HOME")) / "/.local/share/wifisync/";
        } else {
            base_path = fs::path(data_home) / "/wifisync/";
        }
        fs::create_directory(base_path);
        return base_path;
    }();
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

void FileAccess::write_json(std::string_view rel_path, nlohmann::json j){
    FileAccess::get_fout(rel_path) << j;
}

nlohmann::json FileAccess::read_json(std::string_view rel_path){
    return nlohmann::json::parse(FileAccess::get_fin(rel_path));
}
