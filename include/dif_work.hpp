#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

class DifWork{
    private:
        std::string own_name;
        std::string own_uuid;
        std::vector<std::string> detecting_paths;
        std::vector<std::string> ignoring_paths;
        std::string other_uuid;
        std::vector<std::string> read_paths_from_file(fs::path path, bool create);

    public:
        DifWork(std::string uuid);
        void add_to_sync(fs::path path);
        void add_to_ignore(fs::path path);
        void shift_snapshots();
        void calculate_dif();

};

