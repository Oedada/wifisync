#include "fs/data_manager.hpp"
#include <string>
#include <variant>

enum class ResponseState {
    Pass,
    Received,
    Connecting,
    Connected,
    Error,
    Denied
};

enum class ResponseCmdType { Pass, Accept, Deny, Disconnect, Cancel };
struct ResponseCmd {
    ResponseCmdType type;
};

struct Response {
    ResponseCmd cmd;
    ResponseState state;
};

enum class RequestState { Pass, Connecting, WaitAns, Connected, Denied, Error };

enum class RequestCmdType { Pass, Connect, Disconnect, Cancel };

struct RequestCmd {
    RequestCmdType type;
    std::variant<std::pair<std::string, int>, std::monostate> data;
};

struct Request {
    RequestCmd cmd;
    RequestState state;
};

class Session{
    private:
        DataManager dm;
        Request req_conn;
        Response resp_conn;
        std::string ConnectRequestHello;
        std::string ConnectResponseHello;
        void read_received_data();
    public:
        Session();
        bool set_request_cmd(RequestCmd cmd);
        bool set_response_cmd(ResponseCmd cmd);
        void cycle();

};
