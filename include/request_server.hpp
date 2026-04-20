#include <condition_variable>
#include <map>
#include <string>
#include <unistd.h>
#include <variant>
#include <mutex>
#include <queue>
#include "httplib.h"
#include "nlohmann/json.hpp"

namespace httplib{
    struct Request;
    struct Response;
}

using Arg = std::variant<bool, int, std::string>;

namespace tstypes{
    enum class Tasks{
        Connect
    };

    struct Command{
        Tasks task;
        std::vector<Arg> args;
    };

    const std::map<std::string, Tasks> str_to_tasks{
        {"connect", Tasks::Connect}
    };
}
class SafeCmdQueue{
    private:
        std::queue<tstypes::Command> queue;
        std::mutex m;
        std::condition_variable cv;
    public:
        void add(tstypes::Command cmd);
        tstypes::Command get();
};

class TaskServer{
    public:
        SafeCmdQueue* cmd_q;
        TaskServer(SafeCmdQueue &queue);
        void start_server(int port);
        httplib::Server svr;

    private:
        int parse_task_json(nlohmann::json j, tstypes::Command& cmd);
        void get_tasks(const httplib::Request& req, httplib::Response& res);
};
