#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <span>
#include <vector>
class BytePipe{
    private:
        std::mutex mtx;
        std::vector<char> buf;
        std::condition_variable cv;
    public:
        std::vector<char> read(uint64_t n);
        void write(std::span<const char> data);
        bool empty();
};
