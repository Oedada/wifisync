#include "network/broadcast.hpp"
#include "util/init.hpp"
#include "util/utils.hpp"
#include <chrono>
#include <thread>


int main() {
    init();
    UdpBroadcast br(12345);
    std::thread t(&UdpBroadcast::process, &br);
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto devices = br.get_devices();
        for(auto [uuid, device] : devices){
            logmsg(std::string(uuid) + ":" + std::string(device.name));
        }
    }
}
