#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <cassert>
#include <functional>
#include <atomic>
#include <future>
#include <cstdarg>

class ThreadPool
{
public:

    // 获取线程池的单例实例，传入线程数量
    static ThreadPool& GetInstance(int thread_count = std::thread::hardware_concurrency())
    {
        static ThreadPool instance(thread_count); // 使用静态变量保证单例
        return instance;
    }

    // 添加任务到线程池队列中
    template <typename F, typename... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<std::invoke_result_t<F, Args...>>
    {
        using return_type = std::invoke_result_t<F, Args...>; // 推导任务返回值类型

        // 将任务封装为 std::packaged_task
        auto task = std::make_shared<std::packaged_task<return_type>>([func = std::forward<F>(f), args...]() mutable
                                                                      { return std::invoke(func, args...); });

        std::future<return_type> res = task->get_future(); // 获取任务的 future 对象
        {
            std::unique_lock<std::mutex> lock(queue_mutex); // 加锁保护任务队列
            if (stop)
            {
                throw std::runtime_error("enqueue on stopped ThreadPool"); // 如果线程池已停止，抛出异常
            }
            // 将任务加入队列
            m_queue.emplace([task](){ (*task)(); });
        }
        condition.notify_one(); // 通知一个等待的线程
        return res;             // 返回任务的 future 对象
    }

    ~ThreadPool()
    {
        shutdown(); // 析构时关闭线程池
    }

    // 获取线程池中线程的数量
    size_t size() const
    {
        return workers.size();
    }

private:
    // 构造函数，初始化线程池并启动指定数量的线程
    explicit ThreadPool(int thread_count) : stop(false)
    {
        workers.reserve(thread_count); // 预留线程数量
        for (int i = 0; i < thread_count; ++i)
        {
            workers.emplace_back([this]{
            while(true){
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex); // 加锁保护任务队列
                    // 等待条件变量，直到有任务或线程池停止
                    this->condition.wait(lock,[this]{return this->stop || !this->m_queue.empty();});
                    if(this->stop && this->m_queue.empty()){
                        return; // 如果线程池停止且任务队列为空，退出线程
                    }
                    task = std::move(this->m_queue.front()); // 取出队列中的任务
                    this->m_queue.pop(); // 移除任务
                }
                task(); // 执行任务
            }
         });
        }
    }

    // 关闭线程池，等待所有线程完成
    void shutdown()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex); // 加锁保护任务队列
            stop = true;                                    // 设置停止标志
        }
        condition.notify_all(); // 通知所有等待的线程
        for (auto &worker : workers)
        {
            if (worker.joinable())
            {
                worker.join(); // 等待线程完成
            }
        }
    }

        // 禁止拷贝构造和拷贝赋值
        ThreadPool(const ThreadPool &obj) = delete;
        ThreadPool &operator=(const ThreadPool &obj) = delete;
        // 禁止移动构造和移动赋值
        ThreadPool(ThreadPool &&obj) = delete;
        ThreadPool &operator=(const ThreadPool &&obj) = delete;
    

private:
    std::vector<std::thread> workers;          // 工作线程集合
    std::queue<std::function<void()>> m_queue; // 任务队列
    std::mutex queue_mutex;                    // 保护任务队列的互斥锁
    std::condition_variable condition;         // 条件变量，用于线程间同步
    std::atomic<bool> stop;                    // 标志线程池是否停止
};