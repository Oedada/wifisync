#include <iostream>
#include "change_applier.hpp"
#include "transport.hpp"
#include <filesystem>
namespace fs = std::filesystem;

bool ChangeApplier::safeRemove(const fs::path& path) {
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        std::cout << "Can't remove! Path does not exist\n";
        return false;
    }
    if (!(fs::is_directory(path, ec) || fs::is_regular_file(path, ec))) {
        std::cout << "Can't remove! Path isn't directory or file\n";
        return false;
    }
    if (path == "/" || path.empty()) {
        std::cout << "Refusing dangerous delete\n";
        return false;
    }
    auto count = fs::remove_all(path, ec);

    if (ec) {
        std::cout << "Delete failed: " << ec.message() << '\n';
        return false;
    }
    std::cout << "Removed " << count << " files\n";
    return true;
}


void ChangeApplier::apply_runit(RUnit runit){
    if(runit.mt == ModifyType::Deleted){
        // std::cout << "Delete: " << runit.path << "\n";
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
