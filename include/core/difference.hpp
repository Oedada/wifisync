#pragma once

#include "nlohmann/json.hpp"
#include "util/constants.hpp"
#include <array>
#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

using json = nlohmann::json;

class Difference{
    public: 
        Difference(json last_tree, json current_tree);
        json calculate_difference();
    private:
        json ltree;
        json ctree;
        json dif;
        std::filesystem::path fname;
        std::array<unsigned char, constants::HASH_BYTE_LENGTH> get_hash_from_json(const json &j);
        bool equal_unit(const json& l,const json& c);
        std::vector<std::string> get_keys(const json& j);
        std::vector<std::string> exluding_dif(const std::vector<std::string> &m1, const std::vector<std::string>& m2);
        std::vector<std::string> exluding_dif(const json &m1, const json &m2);
        std::vector<std::string> shared_elements(const std::vector<std::string> &m1, const std::vector<std::string> &m2);
        std::vector<std::string> shared_elements(const json &m1, const json &m2);
        void calc_dif(const json &lj, const json &cj, json &res, const std::string &res_name);
    friend std::ostream& operator<<(std::ostream & os, const Difference obj);
};

inline std::ostream& operator<<(std::ostream &os, Difference obj){
    os << obj.dif;
    return os;
}
