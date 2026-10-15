#include "core/dif_work.hpp"
#include "core/difference.hpp"
#include "fs/data_manager.hpp"
#include "fs/environment.hpp"
#include "fs/snapshot.hpp"
#include "util/constants.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace fs = std::filesystem;

DifWork::DifWork(std::string uuid) : other_uuid(uuid) {
    own_name = dm.get_name();
    own_uuid = dm.get_uuid();
    detecting_paths = dm.list_watch();
    ignoring_paths = dm.list_ignore();
}

void DifWork::shift_snapshots() {
    std::string_view last_shapshot_path = constants::paths::data::last_snapshot;
    std::string_view cur_snapshot_path =
        constants::paths::data::current_snapshot;
    if (FileAccess::is_file(cur_snapshot_path)) {
        Snapshot cur_snapshot(
            json::parse(FileAccess::get_fin(cur_snapshot_path)));
        for (const auto &path : ignoring_paths) {
            if (cur_snapshot.is_registred(path)) {
                cur_snapshot.rm_unit(path);
            }
        }
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
        FileAccess::get_fout(constants::paths::data::current_snapshot)
            << cur_snapshot.json_tree;
        FileAccess::get_fout(constants::paths::data::last_snapshot, true)
            << FileAccess::get_fin(constants::paths::data::current_snapshot,
                                   true)
                   .rdbuf();
    }
    Snapshot cur_snapshot(json::parse("{}"));
    for (const auto &path : detecting_paths) {
        cur_snapshot.set_unit(path, true);
    }
    for (const auto &path : ignoring_paths) {
        if (fs::is_regular_file(path)) {
            cur_snapshot.rm_unit(path);
        }
    }
    if (!fs::exists(last_shapshot_path)) {
        fs::copy_file(cur_snapshot_path, last_shapshot_path,
                      fs::copy_options::overwrite_existing);
    }
}

void DifWork::calculate_dif() {
    Snapshot last_snapshot(FileAccess::read_json(constants::paths::data::last_snapshot));
    Snapshot cur_snapshot(FileAccess::read_json(constants::paths::data::current_snapshot));
    DataManager dm;
    if (dm.devices.get_device(other_uuid).is_first) {
        Difference dif({}, cur_snapshot.json_tree);
        FileAccess::write_json(constants::paths::data::difference, dif.calculate_difference());
        dm.devices.set_is_first(other_uuid, false);
    } else {
        Difference dif(last_snapshot.json_tree, cur_snapshot.json_tree);
        FileAccess::write_json(constants::paths::data::difference,dif.calculate_difference());
    }
    json difference = FileAccess::read_json(constants::paths::data::difference);
    for (const auto &[key, val] : difference.items()) {
        if (key == constants::JSON_FIELD_NAME_TYPE) {
            continue;
        }
        if (!dm.devices.get_device(other_uuid).paths_pairs.contains(key)) {
            std::cerr << "There is no corresponding path to the path " << key;
            difference[key]["/other_path"] = "/None";
        } else {
            difference[key]["/other_path"] =
                dm.devices.get_device(other_uuid).paths_pairs[key];
        }
    }
    FileAccess::write_json(constants::paths::data::difference, difference);
}
