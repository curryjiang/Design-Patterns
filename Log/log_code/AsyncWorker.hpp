#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include "AsyncBuffer.hpp"

namespace mylog
{
    // 异步类型枚举
    enum class AsyncType
    {
        ASYNC_SAFE,  // 线程安全的异步模式
        ASYNC_UNSAFE // 非线程安全的异步模式
    };

    using functor = std::function<void(Buffer &)>; // 回调函数类型

    class AsyncWorker
    {
    public:
        using ptr = std::shared_ptr<AsyncWorker>; // 智能指针类型定义

        // 构造函数，初始化异步工作器
        AsyncWorker(const functor &cb, AsyncType async_type = AsyncType::ASYNC_SAFE)
            : async_type_(async_type), stop_(false), callback_(cb), thread_(std::thread(&AsyncWorker::ThreadEntry, this))
        {
        }

        // 将数据推入生产者缓冲区
        void Push(const char *data, size_t len)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (AsyncType::ASYNC_SAFE == async_type_)
            {
                // 如果是线程安全模式，等待缓冲区有足够的可写空间
                cv_producer_.wait(lock, [this, len]()
                                  { return len <= this->buffer_producer_.WriteableSize(); });
            }
            buffer_producer_.Push(data, len); // 将数据写入生产者缓冲区
            cv_consumer_.notify_one();       // 通知消费者线程
        }

        // 停止异步工作器
        void Stop()
        {
            stop_ = true;               // 设置停止标志
            cv_consumer_.notify_all();  // 唤醒所有消费者线程
            thread_.join();             // 等待工作线程结束
        }

    private:
        // 消费者线程入口函数
        void ThreadEntry()
        {
            while (true)
            {
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    // 如果生产者缓冲区为空且停止标志为真，等待条件变量
                    if (buffer_producer_.IsEmpty() && stop_)
                    {
                        cv_consumer_.wait(lock, [&]()
                                          { return stop_ || !buffer_consumer_.IsEmpty(); });
                    }
                    buffer_producer_.Swap(buffer_consumer_); // 交换生产者和消费者缓冲区
                    if (async_type_ == AsyncType::ASYNC_SAFE)
                    {
                        cv_producer_.notify_one(); // 通知生产者线程
                    }
                }
                callback_(buffer_consumer_); // 调用回调函数处理消费者缓冲区数据
                buffer_consumer_.Reset();   // 重置消费者缓冲区
                if (stop_ && buffer_producer_.IsEmpty())
                    return; // 如果停止标志为真且生产者缓冲区为空，退出线程
            }
        }

    private:
        AsyncType async_type_;            // 异步类型
        std::atomic<bool> stop_;          // 停止标志
        std::mutex mutex_;                // 互斥锁
        mylog::Buffer buffer_producer_;   // 生产者缓冲区
        mylog::Buffer buffer_consumer_;   // 消费者缓冲区
        std::condition_variable cv_producer_; // 生产者条件变量
        std::condition_variable cv_consumer_; // 消费者条件变量
        std::thread thread_;              // 消费者线程
        functor callback_;                // 回调函数
    };
}