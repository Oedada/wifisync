#include "transport.hpp"
#include "TCPSocket.hpp"
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>

Transport::Transport(TCPSocket s) : sock(std::move(s)){}

std::ostream& operator<<(std::ostream& stream, RUnit u){
    stream << "Modify Type: " << modify_to_string[u.mt] << "\n";
    stream << "Unit Type: " << unit_type_to_string[u.ut] << "\n";
    stream << "Unit Path: " << u.path << "\n";
    if(u.mt != ModifyType::Deleted){
        if(u.ut == UnitType::Directory){
            stream << "Count sub-units: " << std::get<DirData>(u.data).subunits_number << "\n";
        }
        else{
            stream << "Function for call to recv" << "\n";
        }
    }
    return stream;
}

std::ostream& operator<<(std::ostream& stream, SUnit u){
    stream << "Modify Type: " << modify_to_string[u.mt] << "\n";
    stream << "Unit Type: " << unit_type_to_string[u.ut] << "\n";
    stream << "Unit Name: " << u.name << "\n";
    if(u.ut == UnitType::Directory){
        stream << "Count sub-units: " << std::get<DirData>(u.data).subunits_number << "\n";
    }
    else{
        stream << "File path: " << std::get<FileData>(u.data).file_path << "\n";
    }
    return stream;
}

void Transport::send(SUnit u){
    sock.send(modify_to_string[u.mt]);
    sock.send(unit_type_to_string[u.ut]);
    sock.smart_send_msg(u.name);
    if(u.mt != ModifyType::Deleted){
        if(u.ut == UnitType::File){
            sock.send_file_with_name(std::get<FileData>(u.data).file_path);
        }
        else{
            sock.send(std::get<DirData>(u.data).subunits_number);
        }
    }
}

void Transport::set(RUnit &u){
    char temp[1];
    sock.receive(temp, 1);
    u.mt = string_to_modify[std::string(1, temp[0])];
    sock.receive(temp, 1);
    u.ut = string_to_unit_type[std::string(1, temp[0])];
    u.path = sock.smart_recv_msg();
    if(u.mt != ModifyType::Deleted){
        if(u.ut == UnitType::File){
            u.data = [&](fs::path p) {sock.recv_file_with_name(p);};
        }
        else{
            u.data = DirData{sock.recv_uint64()};
        }
    }
}

void Transport::walk_received(void (*emit)(RUnit&)){
    uint64_t count_roots = sock.recv_uint64();
    for(uint64_t i = 0; i < count_roots; i++){
        walk_unit(emit, "");
    }
}

void Transport::walk_unit(void (*emit)(RUnit&), fs::path parent_path){
    RUnit runit;
    set(runit);
    std::string name = runit.path;
    if(parent_path == ""){
        runit.path = name;
    } else {
        runit.path = parent_path / name;
    }
    emit(runit);
    if(runit.ut == UnitType::Directory){
        for(uint64_t j = 0; j < std::get<DirData>(runit.data).subunits_number; j++){
            walk_unit(emit, parent_path / name);
        }
    }
}
