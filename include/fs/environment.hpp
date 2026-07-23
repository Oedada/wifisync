#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <SDL2/SDL.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace env{
    // paths
    const fs::path get_data_path();
    const fs::path get_data_path(std::string sub_name);
    const std::string get_uuid();
    const std::string get_name();
}
