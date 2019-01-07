#ifndef UTILS_H
#define UTILS_H

#include <ctime>
#include <iostream>
#include <iomanip>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#define MAX_PAYLOAD (80)

void logText(std::string const &text);


template<typename T>
class Queue {
public:

    T pop() {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty()) {
            cond_.wait(mlock);
        }
        auto val = queue_.front();
        queue_.pop();
        return val;
    }

    void pop(T &item) {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty()) {
            cond_.wait(mlock);
        }
        item = queue_.front();
        queue_.pop();
    }

    void push(const T &item) {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push(item);
        mlock.unlock();
        cond_.notify_one();
    }

    bool empty() {
        return queue_.empty();
    }

    Queue() = default;

    Queue(const Queue &) = delete;            // disable copying
    Queue &operator=(const Queue &) = delete; // disable assignment

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

#endif
