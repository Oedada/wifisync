#include <App.h>
#include <string>
#include <string_view>
#include "WebSocketProtocol.h"
#include "utils.hpp"

int main() {
    auto app = uWS::App();
    auto ws = app.ws<int>("/ws", {
        .open = [](auto *ws){
            logmsg("WebSocket is open");
        },
        .message = [](auto *ws, std::string_view msg, uWS::OpCode opCode){
            logmsg(std::string(msg));
            ws->send(msg);
        }
    });
    ws.listen(9001, [](auto *token) {if (token) {std::cout << "running\n";} else {std::cout << "failed\n";}}).run();
}
