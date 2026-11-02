#pragma once
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <vector>
class BytePipe{
    private:
        std::mutex mtx;
        std::vector<std::byte> buf;
        std::condition_variable cv;
    public:
        BytePipe(const BytePipe &) = delete;
        BytePipe& operator=(const BytePipe &) = delete;

        BytePipe(BytePipe &&other) noexcept {
            buf = other.read(other.size());
        }

        BytePipe& operator=(BytePipe &&other) noexcept {
            std::lock_guard<std::mutex> lock(mtx);
            buf.clear();
            buf = other.read(other.size());
            return *this;
        }
        BytePipe(){};
        std::vector<std::byte> read(uint64_t n);
        std::vector<std::byte> eat(uint64_t n);
        void write(std::span<const std::byte> data);
        bool empty();
        uint64_t size();
};
