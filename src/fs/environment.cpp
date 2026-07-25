#include "fs/environment.hpp"
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>

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
std::ifstream FileAccess::get_fin(std::string_view path) {
    return std::ifstream(get_and_create_if_not_exists_base_path() / path);
}

std::ofstream FileAccess::get_fout(std::string_view path) {
    return std::ofstream(get_and_create_if_not_exists_base_path() / path);
}
