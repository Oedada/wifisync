#include "core/difference.hpp"
#include "nlohmann/json.hpp"
#include "util/constants.hpp"
#include "util/utils.hpp"
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

Difference::Difference(json last_tree, json current_tree)
    : ltree(last_tree), ctree(current_tree) {}
json Difference::calculate_difference() {
    // Перебираем корневые папки в текущем снапшоте
    for (auto &[dir_name, dir] : ctree.items()) {
        // Если есть в прошлом, то вычисляем разницу
        if (ltree.contains(dir_name)) {
            if (ltree.at(dir_name).at(constants::JSON_FIELD_NAME_HASH) !=
                ctree.at(dir_name).at(constants::JSON_FIELD_NAME_HASH)) {
                calc_dif(ltree.at(dir_name), ctree.at(dir_name), dif, dir_name);
            }
            // иначе считаем разницу между текущим и пустым(но вроде уже
            // офикшено в difwork)
        } else {
            json empty_json;
            empty_json[constants::JSON_FIELD_NAME_TYPE] =
                constants::UNIT_TYPE_EMPTY_DIR;
            empty_json[constants::JSON_FIELD_NAME_HASH] = json::array();
            calc_dif(empty_json, ctree.at(dir_name), dif, dir_name);
        }
    }
    return dif;
}

std::array<unsigned char, constants::HASH_BYTE_LENGTH>
Difference::get_hash_from_json(const json &j) {
    std::array<unsigned char, constants::HASH_BYTE_LENGTH> hash;
    for (int i = 0; i < constants::HASH_BYTE_LENGTH; i++) {
        try {
            hash[i] = j.at(constants::JSON_FIELD_NAME_HASH)
                          .at(i)
                          .get<unsigned char>();
        } catch (json::out_of_range &e) {
            throw std::runtime_error("Can't find /hash json data or index");
        }
    }
    return hash;
}

bool Difference::equal_unit(const json &l, const json &c) {
    return l.at(constants::JSON_FIELD_NAME_HASH) == l.at(constants::JSON_FIELD_NAME_HASH);
    //старый способ
    return get_hash_from_json(l) == get_hash_from_json(c);
}

std::vector<std::string> Difference::get_keys(const json &j) {
    std::vector<std::string> keys;
    for (auto &[key, val] : j.items()) {
        keys.push_back(key);
    }
    return keys;
}

std::vector<std::string>
Difference::exluding_dif(const std::vector<std::string> &m1,
                         const std::vector<std::string> &m2) {
    std::vector<std::string> dif;
    for (auto s : m1) {
        if (std::find(m2.begin(), m2.end(), s.c_str()) == m2.end()) {
            dif.push_back(s);
        }
    }
    return dif;
}

// возращает те элементы, что есть в первом, но нет во втром
std::vector<std::string> Difference::exluding_dif(const json &m1,
                                                  const json &m2) {
    return exluding_dif(get_keys(m1), get_keys(m2));
}

std::vector<std::string>
Difference::shared_elements(const std::vector<std::string> &m1,
                            const std::vector<std::string> &m2) {
    std::vector<std::string> elts;
    for (auto s : m1) {
        if (std::find(m2.begin(), m2.end(), s.c_str()) != m2.end()) {
            elts.push_back(s);
        }
    }
    return elts;
}

std::vector<std::string> Difference::shared_elements(const json &m1,
                                                     const json &m2) {
    return shared_elements(get_keys(m1), get_keys(m2));
}

void Difference::calc_dif(const json &lj, const json &cj, json &res,
                          const std::string &res_name) {
    // Если юнит имел тип emty_dir заменяем на dir, кажется костыль
    if (lj.at(constants::JSON_FIELD_NAME_TYPE) ==
        constants::UNIT_TYPE_EMPTY_DIR) {
        res[res_name][constants::JSON_FIELD_NAME_TYPE] =
            constants::UNIT_TYPE_DIR;
        // Присваиваем тот же тип, что и в снапшоте
    } else {
        res[res_name][constants::JSON_FIELD_NAME_TYPE] =
            lj.at(constants::JSON_FIELD_NAME_TYPE);
    }
    // Если array или строка(то есть hash или служебные) ничего не делаем
    if (lj.is_array() || lj.is_string()) {
        return;
    }
    // ставим modified в dif
    res[res_name][constants::JSON_FIELD_NAME_MOD_TYPE] =
        constants::MODIFIED_UNIT;
    if (lj.at(constants::JSON_FIELD_NAME_TYPE) != constants::UNIT_TYPE_FILE) {
        // то, что быо в прошлом, сейчас нет
        std::vector<std::string> deleted_files = exluding_dif(lj, cj);
        // то, что сейчас есть, но не было
        std::vector<std::string> added_files = exluding_dif(cj, lj);
        // добавляем
        for (std::string &name : deleted_files) {
            res[res_name][name][constants::JSON_FIELD_NAME_MOD_TYPE] =
                constants::DELETED_UNIT;
            res[res_name][name][constants::JSON_FIELD_NAME_TYPE] =
                lj.at(name).at(constants::JSON_FIELD_NAME_TYPE);
        }
        // убавляем
        for (std::string &name : added_files) {
            res[res_name][name][constants::JSON_FIELD_NAME_MOD_TYPE] =
                constants::ADDED_UNIT;
            res[res_name][name][constants::JSON_FIELD_NAME_TYPE] =
                cj.at(name).at(constants::JSON_FIELD_NAME_TYPE);
        }
        std::vector<std::string> shared_files = shared_elements(lj, cj);
        // Модификация файлов
        for (auto &key : shared_files) {
            // Пропускаем служебные
            if (lj.at(key).is_array() || lj.at(key).is_string()) {
                continue;
            }
            // Если не равны создаём объект и передаём его для записи
            if (!equal_unit(lj.at(key), cj.at(key))) {
                logmsg("Modified: " + key);
                res[res_name][key] = json::object();
                calc_dif(lj.at(key), cj.at(key), res[res_name], key);
            }
        }
    }
}
