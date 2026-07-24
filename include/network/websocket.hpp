#include "WebSocket.h"
#include "Loop.h"
#include "WebSocketProtocol.h"
#include "util/queue.hpp"
#include <App.h>
#include <SDL_video.h>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

using json = nlohmann::json;

class ActionMsg {
  private:
    ActionMsg(std::string type, std::string action, json params, int64_t id);

  public:
    constexpr static std::string_view REQUEST_TYPE = "req";
    constexpr static std::string_view COMMAND_TYPE = "cmd";
    std::string type;
    std::string action;
    uint64_t id;
    json parameters;

    static ActionMsg from_json(json j) {
        if (j.contains("type") && j.contains("action") &&
            j.contains("parameters") && j.contains("id")) {
            auto type = j.at("type").get<std::string>();
            if (type == REQUEST_TYPE || type == COMMAND_TYPE) {
                return ActionMsg(j.at("type").get<std::string>(),
                                 j.at("action").get<std::string>(),
                                 j.at("parameters"),
                                 j.at("id").get<uint64_t>());
            } else {
                throw std::runtime_error("Type must be one of req, cmd");
            }
        } else {
            throw std::runtime_error(
                std::string("One or more fields are missing in this json: ") +
                std::string(j));
        }
    }
    void print();
};

class InfoMsg {
  public:
    std::string event;
    json parameters;
    constexpr static std::string_view type = "info";
    InfoMsg(std::string event, json params);
    json to_json() const;
};

class RespMsg {
  public:
    constexpr static std::string_view type = "resp";
    json parameters;
    bool status;
    uint64_t id;
    RespMsg(json params, bool status, uint64_t id);
    json to_json() const;
};

class Bridge {
  public:
    struct UserData {
        uWS::Loop *loop;
    };

  private:
    SafeQueue<ActionMsg> action_messages;
    uWS::App app = uWS::App();
    uWS::WebSocket<false, true, UserData> *saved_ws;
    uint64_t id_counter = 1;

    void _on_open(auto *ws);
    void _on_message(auto *ws, std::string_view raw, uWS::OpCode code);
    void send_json(const json &j);

  public:
    uint32_t port;
    Bridge(uint32_t port);

    std::optional<ActionMsg> recieve_action_msg(); 
    void send_info(const InfoMsg &msg);
    void send_resp(const RespMsg &msg);
    void run();
    ~Bridge();
};
