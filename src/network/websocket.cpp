#include "WebSocket.h"
#include "Loop.h"
#include "WebSocketProtocol.h"
#include "util/queue.hpp"
#include "util/utils.hpp"
#include <App.h>
#include <SDL_video.h>
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

using json = nlohmann::json;

class ActionMsg {
  private:
    ActionMsg(std::string type, std::string action, json params, int64_t id)
        : type(type), action(action), id(id), parameters(params) {}

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

    void print() {
        std::cout << "Action Message" << "\n";
        std::cout << "Type: " << type << "\n";
        std::cout << "Action: " << action << "\n";
        std::cout << "Params: " << (parameters) << "\n";
    }
};

class InfoMsg {
  public:
    std::string event;
    json parameters;
    constexpr static std::string_view type = "info";
    InfoMsg(std::string event, json params)
        : event(event), parameters(params) {}
    json to_json() const {
        json res = {};
        res["event"] = event;
        res["parameters"] = parameters;
        res["type"] = type;
        return res;
    }
};

class RespMsg {
  public:
    constexpr static std::string_view type = "resp";
    json parameters;
    bool status;
    uint64_t id;
    RespMsg(json params, bool status, uint64_t id)
        : parameters(params), status(status), id(id) {}
    json to_json() const {
        json j = {};
        j["type"] = type;
        j["ok"] = status;
        j["parameters"] = parameters;
        j["id"] = id;
        return j;
    }
};

using action_callback = std::function<RespMsg(ActionMsg)>;

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

    void _on_open(auto *ws) {
        saved_ws = ws;
        ws->getUserData()->loop = uWS::Loop::get();
        logmsg("Websocket is open");
    }
    void _on_message(auto *ws, std::string_view raw, uWS::OpCode code) {
        json j = json::parse(raw);
        auto msg = ActionMsg::from_json(j);
        action_messages.push(msg);
    }

    void send_json(const json &j) {
        if (!saved_ws) {
            throw std::runtime_error("WebSocket isn't initialized");
        }
        auto *loop = saved_ws->getUserData()->loop;
        loop->defer([this, j = std::move(j)] {
            saved_ws->send(j.dump(), uWS::OpCode::TEXT);
        });
    }

  public:
    uint32_t port;
    Bridge(uint32_t port) : port(port) {
        app.ws<UserData>("/ws", {.open = [this](auto *ws) { _on_open(ws); },
                                 .message =
                                     [this](auto *ws, std::string_view raw,
                                            uWS::OpCode code) {
                                         _on_message(ws, raw, code);
                                     }});
    }

    std::optional<ActionMsg> recieve_msg() { return action_messages.get(); }

    void send_info(const InfoMsg &msg) {
        json j = msg.to_json();
        j["id"] = -static_cast<int64_t>(++id_counter);
        send_json(j);
    }

    void send_resp(const RespMsg &msg) {
        json j = msg.to_json();
        send_json(j);
    }

    void run() {
        app.listen(port, [this](auto *token) {
            if (!token) {
                throw std::runtime_error(
                    std::string("Failed to listen on port ") +
                    std::to_string(port));
            } else {
                logmsg("WebSocket server started");
            }
        }).run();
    }

    ~Bridge(){
        action_messages.close();
    }
};

int main(){
    auto bridge = Bridge(9001);
    std::thread t1 = std::thread([&bridge]{
        auto msg = bridge.recieve_msg();
        if(msg.has_value()){
            ActionMsg rmsg = msg.value();
            rmsg.print();
            RespMsg resp({{"lol", "kek"}}, true, rmsg.id);
            bridge.send_resp(resp);
            bridge.send_info(InfoMsg("info.event", {{"lol", "kek"}}));
        }
    });
    bridge.run();
    t1.join();
}
