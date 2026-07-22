#include <App.h>
#include <string>
#include <string_view>
#include "WebSocketProtocol.h"
#include "utils.hpp"
#include <nlohmann/json.hpp>

int main() {
    auto app = uWS::App();
    app.ws<int>("/ws", {
        .open = [](auto *ws) {
            logmsg("WebSocket is open");
        },
        .message = [&](auto *ws, std::string_view raw, uWS::OpCode) {
            auto j = nlohmann::json::parse(raw);
            nlohmann::json resp = {};
            resp["id"] = j["id"];
            resp["ok"] = true;
            resp["parameters"] = nlohmann::json::object();
            ws->send(resp.dump(), uWS::OpCode::TEXT);
        }
    });
    app.listen(9001, [](auto *token) {
        if (token) std::cout << "running\n";
        else std::cout << "failed\n";
    }).run();
}

