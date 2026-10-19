#include "fs/data_manager.hpp"
#include "network/TCPSocket.hpp"
#include <string>
#include <variant>

enum class State {
    ReceivedRequest,
    SendingRequest,
    WaitAnswer,
    Connected,
    Denied,
    Error,
    Closed
};

enum class Cmd {
    Idle,
    Accept,
    Deny,
    Disconnect
};

enum class Direction {
    Incoming,
    Outgoing
}

class Session{
    private:
        DataManager dm;
        TCPSocket sock;
        State state;
        Cmd command;
        Direction dir;
        std::string ConnectRequestHello;
        std::string ConnectResponseHello;
        void read_received_data();
    public:
        Session(TCPSocket s, Direction dir);
        bool set_request_cmd(RequestCmd cmd);
        bool set_response_cmd(ResponseCmd cmd);
        void cycle();

};
