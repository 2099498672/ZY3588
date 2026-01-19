#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include "common/Types.h"
#include "util/Mutex.h"
#include <queue>
#include <chrono>
#include <condition_variable>

/**
 * 线程安全的任务队列
 */
class TaskQueue {
public:
    TaskQueue() = default;
    ~TaskQueue() = default;
    
    // 禁止拷贝
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;
    
    /**
     * 添加任务到队列
     * @param task 要添加的任务
     */
    void push(const Task& task);
    
    /**
     * 从队列获取任务（阻塞直到有任务）
     * @return 获取的任务
     */
    Task pop();
    
    /**
     * 尝试从队列获取任务（非阻塞）
     * @param task 存储获取的任务
     * @return 是否成功获取任务
     */
    bool tryPop(Task& task);
    
    /**
     * 尝试在指定时间内从队列获取任务
     * @param task 存储获取的任务
     * @param timeout 超时时间（毫秒）
     * @return 是否成功获取任务
     */
    bool tryPopFor(Task& task, uint32_t timeout);
    
    /**
     * 清空队列
     */
    void clear();
    
    /**
     * 检查队列是否为空
     * @return 是否为空
     */
    bool empty() const;
    
    /**
     * 获取队列大小
     * @return 队列中的任务数量
     */
    size_t size() const;

private:
    mutable Mutex mutex_;                  // 互斥锁
    std::queue<Task> queue_;               // 任务队列
    std::condition_variable_any cond_;     // 条件变量
};

#endif // TASK_QUEUE_H
