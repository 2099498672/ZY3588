#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>

#include "thread_safe_queue.h"
#include "join_thread.h"

class interrupt_flag {
public:
    interrupt_flag() : interrupted(false) {}
    
    void request_stop() {
        interrupted.store(true, std::memory_order_release);
    }
    
    bool is_stop_requested() const {
        return interrupted.load(std::memory_order_acquire);
    }
    
    void reset() {
        interrupted.store(false, std::memory_order_release);
    }

private:
    std::atomic_bool interrupted;
};

class interruptible_task {
public:
    using task_type = std::function<bool(interrupt_flag&)>;
    
    interruptible_task(task_type task) : task_(std::move(task)) {}
    
    void operator()(interrupt_flag& flag) {
        if (task_) {
            task_(flag);
        }
    }
    
private:
    task_type task_;
};



class thread_pool {
public:
    thread_pool(unsigned const thread_count_) : thread_count(thread_count_), done(false), joiner(threads) {
        // unsigned const thread_count = std::thread::hardware_concurrency();
        // std::cout << "thread pool started with " << thread_count << " threads." << std::endl;
        try {
            for (unsigned i = 0; i < thread_count; ++i) {
                threads.push_back(
                    std::thread(&thread_pool::worker_thread, this)
                );
            }
        } catch (...) {
            done = true;
            throw;
        }
    }

    ~thread_pool() {
        done = true;
    }
    
    template<typename FunctionType>
    void submit(FunctionType f) {
        work_queue.push(std::function<void()>(f));
    }

    // template<typename FunctionType>
    // std::shared_ptr<interrupt_flag> submit_interruptible(FunctionType f) {
    //     std::shared_ptr<interrupt_flag> flag = std::make_shared<interrupt_flag>(); 
    //     auto task = [flag, f]() {                                  
    //         interruptible_task it([f](interrupt_flag& flag) {          
    //             return f(flag);
    //         });
    //         it(*flag);
    //     };
    //     work_queue.push(task);
    //     return flag;
    // }

    template<typename FunctionType>
        std::shared_ptr<interrupt_flag> submit_interruptible(FunctionType f) {
        auto flag = std::make_shared<interrupt_flag>();

        std::function<void(interrupt_flag&)> func = f;

        work_queue.push(std::function<void()>(
            [flag, func]() {
                func(*flag);
            }
    ));

    return flag;
}


    int get_queue_size() {
        return work_queue.size();
    }

private:
    unsigned const thread_count;
    std::atomic_bool done;
    thread_safe_queue<std::function<void()>> work_queue;
    std::vector<std::thread> threads;
    join_threads joiner;

    void worker_thread() {
        while (!done) {
            std::function<void()> task;
            if (work_queue.try_pop(task)) {
                task();
            } else {
                std::this_thread::yield();
            }
            // std::function<void()> task;
            // if (work_queue.wait_and_pop(task)) {
            //     task();
            // }
        }
    }
};

#endif // __THREAD_POOL_H__

/*
 * @Author: fjl
 * @description: v1 版本线程池
 * @Date: 2025-12-11
 *
 * @description: v2 添加可提交可中断任务的任务包装器
 * @Date: 2025-12-12
 */