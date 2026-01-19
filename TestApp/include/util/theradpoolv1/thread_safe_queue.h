#ifndef __THREAD_SAFE_QUEUE_H__
#define __THREAD_SAFE_QUEUE_H__

#include <mutex>
#include <memory>
#include <condition_variable>

template<typename T>
class thread_safe_queue {
public:
    thread_safe_queue() : head(new node), tail(head.get()) {
        
    }

    thread_safe_queue(const thread_safe_queue& other) = delete;
    thread_safe_queue& operator=(const thread_safe_queue& other) = delete;

    int size() {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        int count = 0;
        node* current = head.get();
        while (current != tail) {
            ++count;
            current = current->next.get();
        }
        return count;
    }

    void push(T new_value) {
        std::shared_ptr<T> new_data = std::make_shared<T>(std::move(new_value));
        std::unique_ptr<node> p(new node);
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            tail->data = new_data;
            node* new_tail = p.get();
            tail->next = std::move(p);
            tail = new_tail;
        }
        data_cond.notify_one();
    }

    std::shared_ptr<T> try_pop() {
        std::unique_ptr<node> old_head = try_pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    bool try_pop(T& value) {
        std::unique_ptr<node> old_head = try_pop_head(value);
        return old_head != nullptr;
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_ptr<node> old_head = wait_pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    bool wait_and_pop(T& value) {
        std::unique_ptr<node> old_head = wait_pop_head(value);
        return old_head != nullptr;
    }

    bool empty() {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        return head.get() == get_tail();
    }

private:
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::mutex head_mutex;
    std::unique_ptr<node> head;

    std::mutex tail_mutex;
    node* tail;
    std::condition_variable data_cond;

    node* get_tail() {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }

    std::unique_ptr<node> pop_head() {
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

    std::unique_ptr<node> try_pop_head() {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail()) {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head(T& value) {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail()) {
            return std::unique_ptr<node>();
        }
        value = std::move(*head->data);
        return pop_head();
    }

    std::unique_lock<std::mutex> wait_for_data() {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        data_cond.wait(head_lock, [this]() { return head.get() != get_tail(); });
        /* add by fjl
        1. std::unique_lock 的特性
            std::unique_lock<std::mutex> 是一个可移动但不可复制的类型
            它只能通过移动语义来转移所有权，不能拷贝
        2. 返回值优化（RVO）
            当直接返回局部对象 head_lock 时，编译器会尝试应用返回值优化（RVO）
            RVO 可以避免任何拷贝或移动操作，直接在调用处构造对象
            如果 RVO 不可行，编译器会自动使用移动构造函数
        3. 使用 std::move 的影响
            return std::move(head_lock); 显式要求移动，但可能抑制 RVO
            根据 C++ 核心指南（C++ Core Guidelines），对于可移动类型，应该直接返回局部对象而不使用 std::move
        */
        return head_lock;
    }

    std::unique_ptr<node> wait_pop_head() {
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        return pop_head();
    }

    std::unique_ptr<node> wait_pop_head(T& value) {
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        value = std::move(*head->data);
        return pop_head();
    }
};

#endif // __THREAD_SAFE_QUEUE_H__

/*
 * @Author: fjl
 * @description: v1 版本线程安全队列
 * @Date: 2025-12-12
 */