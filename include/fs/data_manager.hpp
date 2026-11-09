#pragma once
#include <filesystem>
#include <map>
#include <mutex>
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
    std::string name = "";
    bool is_first = true;
    std::map<std::string, std::string> paths_pairs = {};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Device, name, paths_pairs,
                                                is_first)

class DeviceManager {
  private:
    inline static std::map<std::string, Device> devices;
    inline static std::mutex mtx;
    inline static bool is_load;
    static void save_devices();
    static void load_devices();


  public:
    static void add_device(const std::string &uuid, const std::string &name);
    static Device get_device(const std::string &uuid);
    static int set_path_pair_to_device(const std::string &uuid,
                                       const fs::path &own_path,
                                       const fs::path &other_path);
    static bool is_first(const std::string &uuid);
    static void set_is_first(const std::string &uuid, const bool is_first);

    static void rm_device(const std::string &uuid);
    static int rm_path_pair_from_device(const std::string &uuid,
                                        const fs::path &own_path);
    static std::vector<std::string> uuid_list();
};

class DataManager {
  private:
    inline static Config config;
    inline static std::mutex mtx;
    inline static bool is_load;
    static void load_config();
    static void save_config();

  public:
    inline static DeviceManager devices = DeviceManager();
    static void set_name(const std::string &name);
    static std::string get_name();
    static void set_uuid(const std::string &uuid);
    static std::string get_uuid();
    static void add_watch(const fs::path &path);
    static void rm_watch(const fs::path &path);
    static std::vector<std::string> list_watch();
    static void add_ignore(const fs::path &path);
    static void rm_ignore(const fs::path &path);
    static std::vector<std::string> list_ignore();
};
