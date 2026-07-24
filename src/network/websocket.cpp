#include "network/websocket.hpp"
#include "Loop.h"
#include "WebSocket.h"
#include "WebSocketProtocol.h"
#include "util/queue.hpp"
#include "util/utils.hpp"
#include <App.h>
#include <SDL_video.h>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

using json = nlohmann::json;

ActionMsg::ActionMsg(std::string type, std::string action, json params,
                     int64_t id)
    : type(type), action(action), id(id), parameters(params) {}

void ActionMsg::print() {
    std::cout << "Action Message" << "\n";
    std::cout << "Type: " << type << "\n";
    std::cout << "Action: " << action << "\n";
    std::cout << "Params: " << (parameters) << "\n";
}

    InfoMsg::InfoMsg(std::string event, json params)
        : event(event), parameters(params) {}
    json InfoMsg::to_json() const {
        json res = {};
        res["event"] = event;
        res["parameters"] = parameters;
        res["type"] = type;
        return res;
    }

    RespMsg::RespMsg(json params, bool status, uint64_t id)
        : parameters(params), status(status), id(id) {}
    json RespMsg::to_json() const {
        json j = {};
        j["type"] = type;
        j["ok"] = status;
        j["parameters"] = parameters;
        j["id"] = id;
        return j;
    }


    void Bridge::_on_open(auto *ws) {
        saved_ws = ws;
        ws->getUserData()->loop = uWS::Loop::get();
        logmsg("Websocket is open");
    }
    void Bridge::_on_message(auto *ws, std::string_view raw, uWS::OpCode code) {
        json j = json::parse(raw);
        auto msg = ActionMsg::from_json(j);
        action_messages.push(msg);
    }

    void Bridge::send_json(const json &j) {
        if (!saved_ws) {
            throw std::runtime_error("WebSocket isn't initialized");
        }
        auto *loop = saved_ws->getUserData()->loop;
        loop->defer([this, j = std::move(j)] {
            saved_ws->send(j.dump(), uWS::OpCode::TEXT);
        });
    }

    Bridge::Bridge(uint32_t port) : port(port) {
        app.ws<UserData>("/ws", {.open = [this](auto *ws) { _on_open(ws); },
                                 .message =
                                     [this](auto *ws, std::string_view raw,
                                            uWS::OpCode code) {
                                         _on_message(ws, raw, code);
                                     }});
    }

    std::optional<ActionMsg> Bridge::recieve_action_msg() { return action_messages.get(); }

    void Bridge::send_info(const InfoMsg &msg) {
        json j = msg.to_json();
        j["id"] = -static_cast<int64_t>(++id_counter);
        send_json(j);
    }

    void Bridge::send_resp(const RespMsg &msg) {
        json j = msg.to_json();
        send_json(j);
    }

    void Bridge::run() {
        app.listen(port,
                   [this](auto *token) {
                       if (!token) {
                           throw std::runtime_error(
                               std::string("Failed to listen on port ") +
                               std::to_string(port));
                       } else {
                           logmsg("WebSocket server started");
                       }
                   })
            .run();
    }

    Bridge::~Bridge() { action_messages.close(); }

int main() {
    auto bridge = Bridge(9001);
    std::thread t1 = std::thread([&bridge] {
        auto msg = bridge.recieve_action_msg();
        if (msg.has_value()) {
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
