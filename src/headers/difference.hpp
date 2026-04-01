#include <array>
#include <string>
#include <vector>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "constants.hpp"

using json = nlohmann::json;

class Difference{
    public: 
        Difference(std::filesystem::path file_name, json last_tree, json current_tree);
        void calculate_difference();
    private:
        json ltree;
        json ctree;
        json dif;
        std::filesystem::path fname;
        std::array<unsigned char, constants::HASH_BYTE_LENGTH> get_hash_from_json(const json &j);
        bool equal_unit(const json& l,const json& c);
        std::vector<std::string> get_keys(json j);
        std::vector<std::string> exluding_dif(std::vector<std::string> m1, std::vector<std::string> m2);
        std::vector<std::string> exluding_dif(json m1, json m2);
        std::vector<std::string> shared_elements(std::vector<std::string> m1, std::vector<std::string> m2);
        std::vector<std::string> shared_elements(json m1, json m2);
        void calc_dif(json lj, json cj, json &res, std::string res_name);
};
