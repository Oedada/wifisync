#include "concurrency/bytepipe.hpp"
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

std::vector<std::byte> BytePipe::read(uint64_t n, long timeout_milliseconds) {
    if (buf.size() >= n) {
        return std::vector(buf.begin(), buf.end());
    }
    std::unique_lock<std::mutex> lock(mtx);
    if (timeout_milliseconds == -1) {
        cv.wait(lock, [&] { return buf.size() >= n; });
    } else {
        cv.wait_for(lock, std::chrono::milliseconds(timeout_milliseconds),
                    [&] { return buf.size() >= n; });
    }
    if (buf.size() < n) {
        return {};
    }

    std::vector<std::byte> ret(buf.begin(), buf.begin() + n);
    return ret;
}

std::vector<std::byte> BytePipe::eat(uint64_t n, long timeout_milliseconds) {
    std::unique_lock<std::mutex> lock(mtx);
    if (buf.size() >= n) {
        std::vector<std::byte> ret(buf.begin(), buf.begin() + n);
        buf.erase(buf.begin(), buf.begin() + n);
        return ret;
    }

    if (timeout_milliseconds == -1) {
        cv.wait(lock, [&] { return buf.size() >= n; });
    } else {
        cv.wait_for(lock, std::chrono::milliseconds(timeout_milliseconds),
                    [&] { return buf.size() >= n; });
    }
    if (buf.size() < n) {
        return {};
    }

    std::vector<std::byte> ret(buf.begin(), buf.begin() + n);
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

bool BytePipe::empty() {
    std::lock_guard<std::mutex> lock(mtx);
    return buf.size() == 0;
}

uint64_t BytePipe::size() {
    std::lock_guard<std::mutex> lock(mtx);
    return buf.size();
}
