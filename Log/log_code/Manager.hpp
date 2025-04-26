#pragma once
#include <unordered_map>
#include "AsyncLogger.hpp"

namespace mylog
{
    //日志器管理类 - 单例模式(懒汉模式 - 静态局部变量)
    class LoggerManager
    {
    public:
        using ptr = std::shared_ptr<LoggerManager>;

        // 获取 LoggerManager 的单例实例
        static LoggerManager &GetInstance()
        {
            static LoggerManager instance;
            return instance;
        }

        // 检查指定名称的日志器是否存在
        bool LoggerExist(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(mutex);
            return loggers_.find(name) == loggers_.end();
        }

        // 添加一个日志器
        void AddLogger(const AsyncLogger::ptr &&AsyncLoggerPtr)
        {
            if (LoggerExist(AsyncLoggerPtr->Name()))
            {
                return;
            }
            std::unique_lock<std::mutex> lock(mutex);
            loggers_.insert(std::make_pair(AsyncLoggerPtr->Name(), AsyncLoggerPtr));
        }

        // 获取指定名称的日志器
        AsyncLogger::ptr GetLogger(const std::string& name)
        {
            std::unique_lock<std::mutex> lock(mutex);
            auto it = loggers_.find(name);
            if (it == loggers_.end())
            {
                return AsyncLogger::ptr();
            }
            return it->second;
        }

        // 获取默认日志器
        AsyncLogger::ptr DefaultLogger()
        {
            return default_logger_;
        }

    private:
        // 构造函数，初始化默认日志器
        LoggerManager()
        {
            std::unique_ptr<LoggerBuilder> builder(new LoggerBuilder());
            builder->BuildLoggerName("default");
            default_logger_ = builder->Build();
            loggers_.insert(std::make_pair("default", default_logger_));
        }

        ~LoggerManager() = default;
        LoggerManager(const LoggerManager &) = delete;            // 禁止拷贝构造
        LoggerManager &operator=(const LoggerManager &) = delete; // 禁止拷贝赋值
        LoggerManager(LoggerManager &&) = delete;                 // 禁止移动构造
        LoggerManager &operator=(LoggerManager &&) = delete;      // 禁止移动赋值

    private:
        std::mutex mutex; // 互斥锁，用于线程安全,互斥访问loggers_
        AsyncLogger::ptr default_logger_; // 默认日志器
        std::unordered_map<std::string, AsyncLogger::ptr> loggers_; // 存储所有日志器的映射表
    };
}