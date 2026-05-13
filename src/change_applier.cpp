#include "change_applier.hpp"
#include "transport.hpp"
#include "utils.hpp"
#include <filesystem>
namespace fs = std::filesystem;

bool ChangeApplier::safeRemove(const fs::path& path) {
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        logwarn("Can't remove! Path does not exist");
        return false;
    }
    if (!(fs::is_directory(path, ec) || fs::is_regular_file(path, ec))) {
        logwarn("Can't remove! Path isn't directory or file");
        return false;
    }
    if (path == "/" || path.empty()) {
        logwarn("Refusing dangerous delete");
        return false;
    }
    fs::remove_all(path, ec);

    if (ec) {
        logwarn("Delete failed: " + ec.message());
        return false;
    }
    return true;
}


void ChangeApplier::apply_runit(RUnit &runit){
    if(runit.mt == ModifyType::Deleted){
        safeRemove(runit.path);
    }
    else{
        if(runit.ut == UnitType::Directory){
            fs::create_directories(runit.path);
        }
        else{
            std::get<std::function<void(fs::path)>>(runit.data)(runit.path);
        }
    }

}
