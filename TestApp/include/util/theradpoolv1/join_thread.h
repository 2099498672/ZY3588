#ifndef __JION_THREADS_H__
#define __JION_THREADS_H__

#include <vector>
#include <thread>

class join_threads {
public:
    explicit join_threads(std::vector<std::thread>& threads_) : threads(threads_) {

    }

    ~join_threads() {
        for (unsigned long i = 0; i < threads.size(); ++i) {
            if (threads[i].joinable()) {
                threads[i].join();
            }
        }
    }

private:
    std::vector<std::thread>& threads;

};

#endif // __JION_THREADS_H__

/*
 * @Author: fjl
 * @description: v1 线程池辅助类，用于在线程池析构时，自动join所有线程
 * @Date: 2025-12-12
 */