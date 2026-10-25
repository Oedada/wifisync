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
        std::condition_variable &on_read;
    public:
        BytePipe(std::condition_variable &on_readable) : on_read(on_readable){}
        std::vector<char> read(uint64_t n);
        std::vector<char> eat(uint64_t n);
        void write(std::span<const char> data);
        bool empty();
};
