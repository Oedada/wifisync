#pragma once

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

uint64_t count_subelements(nlohmann::json node);
void catch_error(int return_code, std::string error_message);
void toBytes(uint64_t x, unsigned char* out_b);
uint64_t fromBytes64(unsigned char* in_b);
uint64_t fromBytes16(unsigned char* in_b);
nlohmann::json read_json(std::filesystem::path path, bool not_exists_create);
void write_json(std::filesystem::path path, nlohmann::json j);
void logmsg(std::string msg);
void logerr(std::string error);
void logwarn(std::string warning);
template <typename T>
void print_vector(const std::vector<T>& v) {
    std::cout << "[ ";
    for (const auto& x : v) {
        std::cout << x << " ";
    }
    std::cout << "]\n";
}
