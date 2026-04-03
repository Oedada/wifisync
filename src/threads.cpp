#include <openssl/rand.h>
#include "headers/x25519.hpp"
#include "headers/ed25519.hpp"
#include <unistd.h>
#include <variant>
#include <thread>
#include <nlohmann/json.hpp>
#include "headers/request_server.hpp"
#include "headers/sync.hpp"


using json = nlohmann::json;
using Arg = std::variant<bool, int, std::string>;

// Глобальная очередь сообщений
std::vector<httplib::Response*> clients;
Sync session;

SafeCmdQueue cmd_q;

//-----------//
//Thread work//
//-----------//

void thread_worker(){
    while(!session.is_connecting_process){
        session.broadcast.send(Message::Broadcast, session.broadcast.get_broadcast_addr());
        session.broadcast.read_received_data();
        auto [succes, uuid] = session.broadcast.recv(Message::ConnectRequest);
        if(succes){
            for(auto client : clients){
                std::ostringstream s;
                s << session.broadcast.get_found_devices()[uuid].second << " try to connect with you" << "\n\n";
                client->write(s.str());
            }
            session.broadcast.send(Message::ConnectResponse, session.broadcast.get_found_devices()[uuid].first);
        }
        usleep(constants::SLEEP_TIME);
    }
}

int main() {
    TaskServer ts(cmd_q);
    ts.svr.Get("/devices", [&](const httplib::Request&, httplib::Response& res) {
        json j = session.find_devices();
        res.set_content(j.dump(), "application/json");
    });

    ts.svr.Post("/connect", [&](const httplib::Request& req, httplib::Response& res) {
        auto j = json::parse(req.body);
        std::string uuid = j["uuid"];
        // 🔧 Тут подключаемся к устройству (ядро)
        bool succes = session.connect_to_device(uuid); // твоя функция

        json response;
        if(succes){
            response["message"] = "Успешно подключено к " + uuid;
        }
        else{
            response["message"] = "Не получилось, не фортануло";
        }
        res.set_content(response.dump(), "application/json");
    });

    ts.svr.Get("/sse", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Content-Type", "text/event-stream");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");

        // Цикл держит соединение открытым
        for(int i = 0; i < 5; ++i) {
            std::ostringstream msg;
            msg << "data: Событие " << i << "\n\n";

            // res.body нельзя менять многократно, поэтому используем flush
            res.set_content(msg.str(), "text/event-stream");

            // даём время браузеру получить данные
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // После выхода из цикла соединение закроется автоматически
    });

    std::thread api_thread([&ts](){return ts.start_server(5000);});
    std::thread thread_work_thread(thread_worker);
    
    api_thread.join();
    thread_work_thread.join();
}
