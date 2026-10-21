#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <utility>

template <typename T>
class SafeQueue {
private:
    std::queue<T> q;
    std::mutex mtx;
    std::condition_variable cv;
    bool closed = false;

public:
    std::optional<T> get();
    bool push(T val);
    void close();
    bool empty();
};

template <typename T>
std::optional<T> SafeQueue<T>::get() {
    std::unique_lock<std::mutex> lock(mtx);

    cv.wait(lock, [&] {
        return !q.empty() || closed;
    });

    if (q.empty())
        return std::nullopt;

    T val = std::move(q.front());
    q.pop();

    return std::optional<T>{std::move(val)};
}

template <typename T>
bool SafeQueue<T>::push(T val) {
    {
        std::lock_guard<std::mutex> lock(mtx);

        if (closed)
            return false;

        q.push(std::move(val));
    }

    cv.notify_one();
    return true;
}

template <typename T>
bool SafeQueue<T>::empty(){
    std::lock_guard<std::mutex> lock(mtx);
    return q.empty();
}

template <typename T>
void SafeQueue<T>::close() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        closed = true;
    }

    cv.notify_all();
}

