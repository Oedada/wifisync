#include "transport.hpp"
#include "TCPSocket.hpp"

Transport::Transport(TCPSocket s) : sock(std::move(s)){}

void Transport::send(SUnit u){
    sock.send(modify_to_string[u.mt]);
    sock.send(unit_type_to_string[u.ut]);
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
    u.mt = string_to_modify[temp];
    sock.receive(temp, 1);
    u.ut = string_to_unit_type[temp];
    if(u.mt != ModifyType::Deleted){
        if(u.ut == UnitType::File){
            u.data = [&](fs::path p) {sock.recv_file_with_name(p);};
        }
        else{
            u.data = DirData{sock.recv_uint64()};
        }
    }
}

int main(){
    TCPSocket
    Transport t();
}
