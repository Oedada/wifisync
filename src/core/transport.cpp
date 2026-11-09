#include "core/transport.hpp"
#include "core/dif_walker.hpp"
#include "fs/environment.hpp"
#include "util/constants.hpp"
#include "util/utils.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <nlohmann/json_fwd.hpp>
#include <openssl/conf.h>
#include <stdexcept>
#include <sys/socket.h>

using json = nlohmann::json;

void ExtendedSend::send_string_with_size(std::string s) {
    sendf(s.size());
    sendf(s);
}

void ExtendedSend::send_file(fs::path file_path) {
    if (!fs::is_regular_file(file_path)) {
        throw std::runtime_error("Path isn't file");
    }
    std::ifstream fin(file_path, std::ios::binary);
    send_file(fin);
}

void ExtendedSend::send_file(std::ifstream &fin) {
    if (!fin) {
        throw std::runtime_error("Input stream doesn't correct");
    }
    fin.seekg(0, std::ios::end);
    uint64_t size = fin.tellg();
    fin.seekg(0, std::ios::beg);
    sendf(size);
    std::vector<std::byte> buf(constants::BUFFER_SIZE);
    while (fin.read(reinterpret_cast<char *>(buf.data()), buf.size()) ||
           fin.gcount() > 0) {
        sendf(std::span<std::byte>(buf.data(), buf.size()));
    }
}

uint64_t ExtendedRecv::recv_int() {
    return fromBytes64(
        reinterpret_cast<unsigned char *>(recvf(sizeof(uint64_t)).data()));
}

std::string ExtendedRecv::recv_string_with_size() {
    uint64_t data_size = recv_int();
    if (data_size > constants::MAX_ALLOWED_SIZE || data_size <= 0) {
        throw std::runtime_error("Invalide message size");
    }
    auto buf = recvf(data_size);
    return std::string(buf.begin(), buf.end());
}

void ExtendedRecv::recv_file(std::ofstream &fout) {
    std::vector<std::byte> buf(constants::BUFFER_SIZE);
    uint64_t file_size = recv_int();
    uint64_t remaining_bytes = file_size;
    int read_size;
    while (remaining_bytes > 0) {
        if (remaining_bytes >= constants::BUFFER_SIZE) {
            remaining_bytes -= constants::BUFFER_SIZE;
            read_size = constants::BUFFER_SIZE;
        } else {
            read_size = remaining_bytes;
            remaining_bytes = 0;
        }
        buf = recvf(read_size);
        fout.write(reinterpret_cast<char *>(buf.data()), read_size);
    }
}

Transport::Transport(std::function<bool(std::variant<SmallMsg, BigMsg>)> send,
                     std::function<std::vector<std::byte>(uint64_t)> recv)
    : sendf(send), recvf(recv), es(sendf), er(recvf) {}

std::ostream &operator<<(std::ostream &stream, RUnit u) {
    stream << "Modify Type: " << modify_to_string[u.mt] << "\n";
    stream << "Unit Type: " << unit_type_to_string[u.ut] << "\n";
    stream << "Unit Path: " << u.path << "\n";
    if (u.mt != ModifyType::Deleted) {
        if (u.ut == UnitType::Directory) {
            stream << "Count sub-units: "
                   << std::get<DirData>(u.data).subunits_number << "\n";
        } else {
            stream << "Function for call to recv" << "\n";
        }
    }
    return stream;
}

std::ostream &operator<<(std::ostream &stream, SUnit u) {
    stream << "Modify Type: " << modify_to_string[u.mt] << "\n";
    stream << "Unit Type: " << unit_type_to_string[u.ut] << "\n";
    stream << "Unit Name: " << u.name << "\n";
    if (u.ut == UnitType::Directory) {
        stream << "Count sub-units: "
               << std::get<DirData>(u.data).subunits_number << "\n";
    } else {
        stream << "File path: " << std::get<FileData>(u.data).file_path << "\n";
    }
    return stream;
}

void Transport::send(SUnit u) {
    sendf(SmallMsg(modify_to_string[u.mt]));
    sendf(unit_type_to_string[u.ut]);
    es.send_string_with_size(u.name);
    if (u.mt != ModifyType::Deleted) {
        if (u.ut == UnitType::File) {
            es.send_file(std::get<FileData>(u.data).file_path);
        } else {
            sendf(std::get<DirData>(u.data).subunits_number);
        }
    }
}

void ExtendedRecv::recv_file(fs::path file_path) {
    std::ofstream fout(file_path, std::ios::binary);
    recv_file(fout);
}

void Transport::set(RUnit &u) {
    auto temp = recvf(1);
    u.mt = string_to_modify[std::string(temp.begin(), temp.end())];
    temp = recvf(1);
    u.ut = string_to_unit_type[std::string(temp.begin(), temp.end())];
    u.path = er.recv_string_with_size();
    if (u.mt != ModifyType::Deleted) {
        if (u.ut == UnitType::File) {
            u.data = [&](fs::path p) { er.recv_file(p); };
        } else {
            u.data = DirData{er.recv_int()};
        }
    }
}

std::vector<fs::path> Transport::walk_received(void (*emit)(RUnit &)) {
    uint64_t count_roots = er.recv_int();
    logmsg(count_roots);
    json dif = FileAccess::read_json(constants::paths::data::difference);
    std::vector<fs::path> conflicts;
    for (uint64_t i = 0; i < count_roots; i++) {
        walk_unit(emit, "", dif, conflicts);
    }
    return conflicts;
}

void Transport::walk_unit(void (*emit)(RUnit &), fs::path parent_path,
                          json &dif_sector, std::vector<fs::path> &conflicts) {
    RUnit runit;
    set(runit);
    std::string name = runit.path;
    json dif_unit_sector;
    if (dif_sector.contains(name)) {
        dif_unit_sector = dif_sector.at(name);
    } else {
        dif_unit_sector = nullptr;
    }
    if (parent_path == "") {
        runit.path = name;
    } else {
        runit.path = parent_path / name;
    }
    if (!dif_unit_sector.is_null()) {
        SUnit own_dif_sunit =
            DifWalker::json_to_sunit(dif_unit_sector, parent_path / name, name);
        if (runit.ut == UnitType::File) {
            if (!(runit.mt == ModifyType::Deleted &&
                  own_dif_sunit.mt == ModifyType::Deleted)) {
                conflicts.push_back(runit.path);
                // TODO: понять и переделать явно
                // std::get<std::function<void(fs::path)>>(runit.data)(env::get_data_path(constants::DATA_DIR)
                // / "tmp.tmp");
            } else {
                emit(runit);
            }
        } else {
            if ((runit.mt == ModifyType::Deleted &&
                 own_dif_sunit.mt != ModifyType::Deleted) ||
                (runit.mt != ModifyType::Deleted &&
                 own_dif_sunit.mt == ModifyType::Deleted)) {
                conflicts.push_back(runit.path);
                return;
            } else {
                emit(runit);
            }
        }
    } else {
        emit(runit);
    }
    if (runit.ut == UnitType::Directory) {
        for (uint64_t j = 0; j < std::get<DirData>(runit.data).subunits_number;
             j++) {
            walk_unit(emit, parent_path / name, dif_unit_sector, conflicts);
        }
    }
}
