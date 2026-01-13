#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include "hash.hpp"
#include "files_opers.hpp"

std::vector<int> A = {1, 2, 3, 4, 5};
    std::vector<int> B = {10, 20, 30};

    // хотим взять из A кусок длины B.size()
    std::vector<int> slice(A.begin(), A.begin() + B.size());
    for (int x : slice) std::cout << x << " "; // выведет: 1 2 3