#pragma once
#include <filesystem>
#include <map>
#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Paths {
    std::vector<std::string> watch = {};
    std::vector<std::string> ignore = {};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Paths, watch, ignore)

struct Config {
    std::string uuid = "";
    std::string name = "";
    Paths paths = {};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config, uuid, name, paths)

struct Device {
    std::string name;
    std::map<std::string, std::string> paths_pairs = {};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Device, name, paths_pairs)

class DeviceManager {
  private:
    std::map<std::string, Device> devices;
    void save_devices();

  public:
    DeviceManager();
    void add_device(const std::string &uuid, const std::string &name);
    int set_path_pair_to_device(const std::string &uuid,
                                const fs::path &own_path,
                                const fs::path &other_path);
    void rm_device(const std::string &uuid);
    int rm_path_pair_from_device(const std::string &uuid,
                                 const fs::path &own_path);
    std::vector<std::string> uuid_list() const;
};

class DataManager {
  private:
    Config config;

    void save_config();

  public:
    DeviceManager devices = DeviceManager();
    DataManager();
    void set_name(const std::string &name);
    void set_uuid(const std::string &uuid);
    std::string get_uuid() const;
    void add_watch(const fs::path &path);
    void rm_watch(const fs::path &path);
    std::vector<std::string> list_watch() const;
    void add_ignore(const fs::path &path);
    void rm_ignore(const fs::path &path);
    std::vector<std::string> list_ignore() const;
};
