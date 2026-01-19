#ifndef MUTEX_H
#define MUTEX_H

#include <mutex>

/**
 * 互斥锁封装类
 */
class Mutex {
public:
    Mutex() = default;
    ~Mutex() = default;
    
    // 禁止拷贝构造和赋值
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    
    // 锁定互斥锁
    void lock() { mutex_.lock(); }
    
    // 尝试锁定互斥锁
    bool tryLock() { return mutex_.try_lock(); }
    
    // 解锁互斥锁
    void unlock() { mutex_.unlock(); }
    
    // 获取原生互斥锁
    std::mutex& getNativeMutex() { return mutex_; }

private:
    std::mutex mutex_;
};

/**
 * RAII风格的锁管理类
 */
class LockGuard {
public:
    // 构造时锁定
    explicit LockGuard(Mutex& mutex) : mutex_(mutex) {
        mutex_.lock();
    }
    
    // 析构时解锁
    ~LockGuard() {
        mutex_.unlock();
    }
    
    // 禁止拷贝
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;

private:
    Mutex& mutex_;
};

#endif // MUTEX_H
