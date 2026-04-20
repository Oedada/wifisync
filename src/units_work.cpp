// #include "headers/files_opers.hpp"
// #include <fstream>
// #include <iostream>
// #include <optional>

// namespace fs = std::filesystem;

// struct FileReceiver{
//     std::optional<fs::path> out_path;
// };

// bool dry_run = true;

// class Transfer

// class SystemWalker{
    

// };

// class ChangeApplier{

//     bool safe_remove_all(const std::filesystem::path& p) {
//         std::error_code ec;

//         if (std::filesystem::is_directory(p, ec)) {
//             for (const auto& entry : std::filesystem::directory_iterator(p, ec)) {
//                 if (ec) return false;

//                 if (!safe_remove_all(entry.path())) {
//                     return false; // ❗ стоп сразу
//                 }
//             }
//         }

//         // удаляем сам объект (файл или пустую папку)
//         if (!std::filesystem::remove(p, ec) || ec) {
//             return false; // ❗ ошибка — стоп
//         }

//         return true;
//     }

//     FileReceiver write_unit_change(UnitChange u, fs::path rp){
//         if(u.mt != ModifyType::Deleted){
//             if(u.ut == UnitType::Directory){
//                 fs::create_directory(rp / u.name);
//             }
//             else if(u.ut == UnitType::File){
//                 std::ofstream out(rp / u.name, std::ios::binary);
//                 return FileReceiver{rp / u.name};
//             }
//         }
//         else{
//             auto cannon = fs::weakly_canonical(rp / u.name);
//             if(cannon == "/" || cannon.empty()){
//                 throw std::runtime_error("Опаааасно!!!");
//             }
//             if(dry_run){
//                 std::cout << "Удалил бы: " << cannon;
//             } else {
//                 safe_remove_all(cannon);
//             }
//         }
//         return FileReceiver{};
//     }
// };
