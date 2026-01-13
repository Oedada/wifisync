#include <filesystem>
#include <nlohmann/json.hpp>

class Units{
    public:
        std::filesystem::path json_path;
        nlohmann::json json_tree;
        Units(const std::filesystem::path& jp);
        void set_unit(const std::filesystem::path &dp);
        void get_unit(const std::filesystem::path p, nlohmann::json &unit);
        void rm_unit(const std::filesystem::path &dp);
        bool is_registred(const std::filesystem::path p);
        ~Units();
    private:
        void find_root(const std::filesystem::path &p, nlohmann::json* &unit_path_part_pointer, std::vector<std::string> &remaining_path_link);
        void create_json_file_list(const std::filesystem::path path, nlohmann::json& json_field);
        nlohmann::json* get_unit_from_path(const std::filesystem::path &p);
};
