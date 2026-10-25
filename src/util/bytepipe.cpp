#include "util/bytepipe.hpp"
#include <condition_variable>
#include <cstdint>
#include <mutex>


std::vector<char> BytePipe::read(uint64_t n) {
    std::unique_lock<std::mutex> lock(mtx);

    cv.wait(lock, [&] { return buf.size() >= n; });

    std::vector<char> ret(buf.begin(), buf.begin()+n);
    return ret;
}

std::vector<char> BytePipe::eat(uint64_t n) {
    std::unique_lock<std::mutex> lock(mtx);

    cv.wait(lock, [&] { return buf.size() >= n; });

    std::vector<char> ret(buf.begin(), buf.begin()+n);
    buf.erase(buf.begin(), buf.begin() + n);
    return ret;
}

void BytePipe::write(std::span<const char> data) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        buf.insert(buf.end(), data.begin(), data.end());
    }
    on_read.notify_all();
    cv.notify_one();
}

bool BytePipe::empty(){
    std::lock_guard<std::mutex> lock(mtx);
    return buf.size() == 0;
}
