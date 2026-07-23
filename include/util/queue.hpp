#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class SafeQueue {
  private:
    std::queue<T> q;
    std::mutex mtx;
    std::condition_variable cv;
    bool closed = false;

  public:
    std::optional<T> get();
    void push(T val);
    void close();
};

template <typename T>
std::optional<T> SafeQueue<T>::get() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&] { return !q.empty() || closed; });
    if (closed) return std::nullopt;
    T val = std::move(q.front());
    q.pop();
    return val;
}

template <typename T>
void SafeQueue<T>::push(T val) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(std::move(val));
    }
    cv.notify_one();
}

template <typename T>
void SafeQueue<T>::close() {
    closed = true;
    cv.notify_one();
}