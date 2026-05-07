#include "TCPSocket.hpp"
#include <functional>
#include <map>
#include <variant>

namespace fs = std::filesystem;


enum class ModifyType{
    Modified,
    Added,
    Deleted
};

enum class UnitType{
    File,
    Directory
};

inline std::map<ModifyType, std::string> modify_to_string{
    {ModifyType::Modified, "M"}, {ModifyType::Added, "A"}, {ModifyType::Deleted, "D"}
};

inline std::map<UnitType, std::string> unit_type_to_string{
    {UnitType::File, "F"}, {UnitType::Directory, "D"}
};

inline std::map<std::string, ModifyType> string_to_modify{
    {"M", ModifyType::Modified}, {"A", ModifyType::Added}, {"D", ModifyType::Deleted}
};

inline std::map<std::string, UnitType> string_to_unit_type{
    {"F", UnitType::File}, {"D", UnitType::Directory}
};

struct DirData{
    std::uint64_t subunits_number;
};

struct FileData{
    std::filesystem::path file_path;
};

struct SUnit{
    ModifyType mt;
    UnitType ut;
    std::string name;
    std::variant<DirData, FileData, std::monostate> data;
};

struct RUnit{
    ModifyType mt;
    UnitType ut;
    std::string path;
    std::variant<DirData, std::function<void(fs::path)>, std::monostate> data;
};

class Transport{
    private:
        TCPSocket sock;
        void walk_unit(void (*emit)(RUnit&), fs::path parent_path);
    public:
        Transport(TCPSocket s);
        void send(SUnit u);
        void set(RUnit &u);
        void walk_received(void (*emit)(RUnit&));
};

std::ostream& operator<<(std::ostream& stream, RUnit u);
std::ostream& operator<<(std::ostream& stream, SUnit u);
