#ifndef UTIL_SINGLETASKTHREAD_H_
#define UTIL_SINGLETASKTHREAD_H_

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <iostream>
#include <chrono>
#include <memory>

class SingleTaskThread {
public: 
    using TaskFunction = std::function<void()>;

private:
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_;
    TaskFunction currentTask_;
    bool taskActive_;
    bool stopCurrentTask_;

    void threadLoop();
};

#endif  // UTIL_SINGLETASKTHREAD_H_