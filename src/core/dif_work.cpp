#include "core/dif_work.hpp"
#include "core/difference.hpp"
#include "fs/data_manager.hpp"
#include "fs/environment.hpp"
#include "fs/snapshot.hpp"
#include "util/constants.hpp"
#include "util/utils.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace fs = std::filesystem;

DifWork::DifWork(std::string uuid) : other_uuid(uuid) {
    own_name = DataManager::get_name();
    own_uuid = DataManager::get_uuid();
    detecting_paths = DataManager::list_watch();
    ignoring_paths = DataManager::list_ignore();
}
/**

*/
void DifWork::shift_snapshots() {
    // путь последнего снапшота !относительно корня
    std::string_view last_shapshot_path = constants::paths::data::last_snapshot;
    std::string_view cur_snapshot_path =
        constants::paths::data::current_snapshot;
    // проверяем, что текущий снашот файл и он существует
    if (FileAccess::is_file(cur_snapshot_path)) {
        Snapshot cur_snapshot(
            json::parse(FileAccess::get_fin(cur_snapshot_path)));
        // удаляем игнорируемые пути из текущего снапшота
        for (const auto &path : ignoring_paths) {
            if (cur_snapshot.is_registred(path)) {
                cur_snapshot.rm_unit(path);
            }
        }
        // добавляем отслеживаемые пути, если они ещё не добавлены в текущем,
        // заполняем нулями hash, чтобы он точно отличался
        for (const auto &path : detecting_paths) {
            if (!cur_snapshot.is_registred(path)) {
                cur_snapshot.json_tree[path] = json::object();
                if (fs::is_regular_file(path)) {
                    cur_snapshot
                        .json_tree[path][constants::JSON_FIELD_NAME_TYPE] =
                        constants::UNIT_TYPE_FILE;
                } else if (fs::is_directory(path)) {
                    cur_snapshot
                        .json_tree[path][constants::JSON_FIELD_NAME_TYPE] =
                        constants::UNIT_TYPE_DIR;
                }
                for (int i = 0; i < constants::HASH_BYTE_LENGTH; i++) {
                    cur_snapshot
                        .json_tree[path][constants::JSON_FIELD_NAME_HASH][i] =
                        0;
                }
            }
        }
        // записываем текущий
        FileAccess::get_fout(constants::paths::data::current_snapshot)
            << cur_snapshot.json_tree;
        // копируем текущий в последний
        FileAccess::get_fout(constants::paths::data::last_snapshot, true)
            << FileAccess::get_fin(constants::paths::data::current_snapshot,
                                   true)
                   .rdbuf();
    } else if (FileAccess::exists(cur_snapshot_path)) {
        throw std::runtime_error(
            "Current snapshot path is directory, but should file");
    }
    // создаём новый снапшот
    Snapshot cur_snapshot(json::parse("{}"));
    // задаём отслеживаемые пути
    for (const auto &path : detecting_paths) {
        cur_snapshot.set_unit(path, true);
    }
    // удаляем игноры
    for (const auto &path : ignoring_paths) {
        if (fs::is_regular_file(path)) {
            cur_snapshot.rm_unit(path);
        }
    }
    // записываем текущий
    FileAccess::get_fout(constants::paths::data::current_snapshot)
        << cur_snapshot.json_tree;
    //если прошлого не было, записыываем текущий в него 
    if (!FileAccess::is_file(last_shapshot_path)) {
        logmsg("Прошлого не было");
        FileAccess::get_fout(last_shapshot_path)
            << json({});
    }
}

void DifWork::calculate_dif() {
    Snapshot last_snapshot(
        FileAccess::read_json(constants::paths::data::last_snapshot));
    Snapshot cur_snapshot(
        FileAccess::read_json(constants::paths::data::current_snapshot));
    //если устройство впервые подключаем, то скидываем ему всё, то есть сравниваем всё из current
    if (DataManager::devices.get_device(other_uuid).is_first) {
        logmsg("first");
        Difference dif({}, cur_snapshot.json_tree);
        FileAccess::write_json(constants::paths::data::difference,
                               dif.calculate_difference());
        DataManager::devices.set_is_first(other_uuid, false);
    } else {
        Difference dif(last_snapshot.json_tree, cur_snapshot.json_tree);
        FileAccess::write_json(constants::paths::data::difference,
                               dif.calculate_difference());
    }
    json difference = FileAccess::read_json(constants::paths::data::difference);
    for (const auto &[key, val] : difference.items()) {
        if (key == constants::JSON_FIELD_NAME_TYPE) {
            continue;
        }
        if (!DataManager::devices.get_device(other_uuid).paths_pairs.contains(key)) {
            logerr("There is no corresponding path to the path " + key);
            difference[key]["/other_path"] = "/None";
        } else {
            difference[key]["/other_path"] =
                DataManager::devices.get_device(other_uuid).paths_pairs[key];
        }
    }
    std::cout << difference << "\n";
    FileAccess::write_json(constants::paths::data::difference, difference);
}
