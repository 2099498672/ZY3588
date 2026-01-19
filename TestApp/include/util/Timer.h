#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>

/**
 * 定时器类，用于周期性执行任务
 */
class Timer {
public:
    /**
     * 构造函数
     * @param interval 定时周期（毫秒）
     * @param callback 定时回调函数
     */
    Timer(uint32_t interval, std::function<void()> callback);
    
    // 析构函数
    ~Timer();
    
    // 启动定时器
    void start();
    
    // 停止定时器
    void stop();
    
    // 设置定时周期
    void setInterval(uint32_t interval);
    
    // 检查定时器是否正在运行
    bool isRunning() const;

private:
    // 定时器线程函数
    void timerThread();
    
    std::thread thread_;               // 定时器线程
    std::atomic<bool> running_;        // 运行状态
    std::atomic<uint32_t> interval_;   // 定时周期（毫秒）
    std::function<void()> callback_;   // 回调函数
    mutable std::mutex mutex_;         // 互斥锁
};

#endif // TIMER_H
