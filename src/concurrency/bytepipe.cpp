#include "concurrency/bytepipe.hpp"
#include <condition_variable>
#include <cstdint>
#include <mutex>


std::vector<std::byte> BytePipe::read(uint64_t n) {
    std::unique_lock<std::mutex> lock(mtx);

    cv.wait(lock, [&] { return buf.size() >= n; });

    std::vector<std::byte> ret(buf.begin(), buf.begin()+n);
    return ret;
}

std::vector<std::byte> BytePipe::eat(uint64_t n) {
    std::unique_lock<std::mutex> lock(mtx);

    cv.wait(lock, [&] { return buf.size() >= n; });

    std::vector<std::byte> ret(buf.begin(), buf.begin()+n);
    buf.erase(buf.begin(), buf.begin() + n);
    return ret;
}

void BytePipe::write(std::span<const std::byte> data) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        buf.insert(buf.end(), data.begin(), data.end());
    }
    cv.notify_one();
}

bool BytePipe::empty(){
    std::lock_guard<std::mutex> lock(mtx);
    return buf.size() == 0;
}

uint64_t BytePipe::size() {
    std::lock_guard<std::mutex> lock(mtx);
    return buf.size();
}
