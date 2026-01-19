#include "task/TaskQueue.h"

void TaskQueue::push(const Task& task) {
    std::lock_guard<Mutex> lock(mutex_);
    queue_.push(task);
    cond_.notify_one();
}

Task TaskQueue::pop() {
    std::unique_lock<Mutex> lock(mutex_);
    cond_.wait(lock, [this] { return !queue_.empty(); });
    
    Task task = queue_.front();
    queue_.pop();
    return task;
}

bool TaskQueue::tryPop(Task& task) {
    std::lock_guard<Mutex> lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    
    task = queue_.front();
    queue_.pop();
    return true;
}

bool TaskQueue::tryPopFor(Task& task, uint32_t timeout) {
    std::unique_lock<Mutex> lock(mutex_);
    if (!cond_.wait_for(lock, std::chrono::milliseconds(timeout), 
                       [this] { return !queue_.empty(); })) {
        return false;
    }
    
    task = queue_.front();
    queue_.pop();
    return true;
}

void TaskQueue::clear() {
    std::lock_guard<Mutex> lock(mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
}

bool TaskQueue::empty() const {
    std::lock_guard<Mutex> lock(mutex_);
    return queue_.empty();
}

size_t TaskQueue::size() const {
    std::lock_guard<Mutex> lock(mutex_);
    return queue_.size();
}
