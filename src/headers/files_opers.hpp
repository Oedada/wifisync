#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>
#include <variant>
#include <vector>

enum class ModifyType{
    Modified,
    Added,
    Deleted
};

enum class UnitType{
    File,
    Directory
};

struct DirData{
    std::uint64_t subunits_number;
};

struct FileData{
    std::filesystem::path file_path;
};

struct UnitChange{
    ModifyType mt;
    UnitType ut;
    std::string name;
    std::variant<DirData, FileData, std::monostate> data;
};

class Units{
    public:
        std::filesystem::path json_path;
        nlohmann::json json_tree;
        Units(const std::filesystem::path& jp, bool clear_create);
        void set_unit(const std::filesystem::path &dp, bool ignore_registred);
        void get_unit(const std::filesystem::path p, nlohmann::json &unit);
        void rm_unit(const std::filesystem::path &dp);
        bool is_registred(const std::filesystem::path p);
        void save();
    private:
        void write_hash(std::filesystem::path path, nlohmann::json& json_field, std::vector<unsigned char> json_field_key, bool is_first_call);
        std::vector<std::string> vector_from_path(const std::filesystem::path &p);
        void find_root(const std::filesystem::path &p, nlohmann::json* &unit_path_part_pointer, std::vector<std::string> &remaining_path_link);
        void create_json_file_list(const std::filesystem::path path, nlohmann::json& json_field, bool is_first_call);
        nlohmann::json* get_unit_from_path(const std::filesystem::path &p);
};
