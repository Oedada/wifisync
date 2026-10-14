#pragma once

#include <SDL2/SDL.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

namespace FileAccess {
bool is_directory(std::string_view rel_path);
bool is_file(std::string_view rel_path);
void mkdir(std::string_view rel_path);
std::ifstream get_fin(std::string_view rel_path);
std::ifstream get_fin(std::string_view rel_path, bool bin_mode);
void write_json(std::string_view rel_path, nlohmann::json j);
nlohmann::json read_json(std::string_view rel_path);
std::ofstream get_fout(std::string_view rel_path);
std::ofstream get_fout(std::string_view rel_path, bool bin_mode);
} // namespace FileAccess
