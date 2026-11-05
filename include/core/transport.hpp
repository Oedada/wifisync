#pragma once

#include "network/session.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <nlohmann/json_fwd.hpp>
#include <variant>

using json = nlohmann::json;

namespace fs = std::filesystem;

enum class ModifyType { Modified, Added, Deleted };

enum class UnitType { File, Directory };

inline std::map<ModifyType, std::string> modify_to_string{
    {ModifyType::Modified, "M"},
    {ModifyType::Added, "A"},
    {ModifyType::Deleted, "D"}};

inline std::map<UnitType, std::string> unit_type_to_string{
    {UnitType::File, "F"}, {UnitType::Directory, "D"}};

inline std::map<std::string, ModifyType> string_to_modify{
    {"M", ModifyType::Modified},
    {"A", ModifyType::Added},
    {"D", ModifyType::Deleted}};

inline std::map<std::string, UnitType> string_to_unit_type{
    {"F", UnitType::File}, {"D", UnitType::Directory}};

struct DirData {
    std::uint64_t subunits_number;
};

struct FileData {
    std::filesystem::path file_path;
};

struct SUnit {
    ModifyType mt;
    UnitType ut;
    std::string name;
    std::variant<DirData, FileData, std::monostate> data;
};

struct RUnit {
    ModifyType mt;
    UnitType ut;
    std::string path;
    std::variant<DirData, std::function<void(fs::path)>, std::monostate> data;
};

class ExtendedSend {
  private:
    const std::function<bool(std::variant<SmallMsg, BigMsg>)> &sendf;

  public:
    ExtendedSend(
        const std::function<bool(std::variant<SmallMsg, BigMsg>)> &send)
        : sendf(send) {}
    void send_string_with_size(std::string s);
    void send_file(std::ifstream &fin);
    void send_file(fs::path file_path);
};

class ExtendedRecv {
  private:
    const std::function<std::vector<std::byte>(uint64_t)> &recvf;

  public:
    ExtendedRecv(const std::function<std::vector<std::byte>(uint64_t)> &recv)
        : recvf(recv) {}
    uint64_t recv_int();
    std::string recv_string_with_size();
    void recv_file(std::ofstream &fout);
    void recv_file(fs::path file_path);
};

class Transport {
  private:
    void walk_unit(void (*emit)(RUnit &), fs::path parent_path,
                   json &dif_sector, std::vector<fs::path> &conflicts);
    std::function<bool(std::variant<SmallMsg, BigMsg>)> sendf;
    std::function<std::vector<std::byte>(uint64_t)> recvf;
    ExtendedSend es;
    ExtendedRecv er;

  public:
    Transport(const std::function<bool(std::variant<SmallMsg, BigMsg>)> &send,
              const std::function<std::vector<std::byte>(uint64_t)> &recv);
    void send(SUnit u);
    void set(RUnit &u);
    std::vector<fs::path> walk_received(void (*emit)(RUnit &));
};

std::ostream &operator<<(std::ostream &stream, RUnit u);
std::ostream &operator<<(std::ostream &stream, SUnit u);
