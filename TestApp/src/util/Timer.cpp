#include "util/Timer.h"
#include <iostream>

Timer::Timer(uint32_t interval, std::function<void()> callback)
    : interval_(interval), callback_(std::move(callback)), running_(false) {}

Timer::~Timer() {
    stop();
}

void Timer::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) {
        running_ = true;
        thread_ = std::thread(&Timer::timerThread, this);
    }
}

void Timer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
        running_ = false;
        if (thread_.joinable()) {
            thread_.join();
        }
    }
}

void Timer::setInterval(uint32_t interval) {
    interval_ = interval;
}

bool Timer::isRunning() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

void Timer::timerThread() {
    while (running_) {
        // 等待指定时间
        auto start = std::chrono::steady_clock::now();
        
        // 执行回调
        try {
            if (callback_) {
                callback_();
            }
        } catch (const std::exception& e) {
            std::cerr << "定时器回调执行错误: " << e.what() << std::endl;
        }
        
        // 计算剩余等待时间
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        auto waitTime = interval_ > elapsed ? interval_ - elapsed : 0;
        
        // 等待剩余时间
        if (waitTime > 0 && running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
        }
    }
}
