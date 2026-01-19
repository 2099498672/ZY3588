#include "util/SingleTaskThread.h"

void SingleTaskThread::threadLoop() {
    while (running_) {  // 主循环
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return !running_ || current_task_  && !task_active_; });

    }
}