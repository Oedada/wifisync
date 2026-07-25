#pragma once

#include <fstream>
#include <SDL2/SDL.h>
#include <string_view>

namespace FileAccess {
    std::ifstream get_fin(std::string_view path);
    std::ofstream get_fout(std::string_view path);
}
