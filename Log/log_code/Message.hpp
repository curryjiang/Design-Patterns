#pragma once
#include <thread>
#include <memory>
#include <sstream>
#include "Level.hpp"
#include "Util.hpp"

namespace mylog
{
    class LogMessage
    {
    public:
        using ptr = std::shared_ptr<LogMessage>;

        // 默认构造函数
        LogMessage() = default;

        // 带参数的构造函数，用于初始化日志消息
        LogMessage(const std::string file, size_t line, LogLevel::value level, std::string name, std::string payload)
            : line_(line), ctime_(Util::Date::Now()), file_name_(file), name_(name), payload_(payload), tid_(std::this_thread::get_id()), level_(level)
        {
        }

        // 格式化日志消息为字符串
        std::string format()
        {
            std::stringstream ret;
            struct tm t;
            localtime_r(&ctime_, &t); // 将时间转换为本地时间
            char buf[128];
            strftime(buf, sizeof(buf), "%H:%M:%S", &t); // 格式化时间为时:分:秒
            std::string tmp1 = '[' + std::string(buf) + "][";
            std::string tmp2 = '[' + std::string(LogLevel::ToString(level_)) + "][" + name_ + "][" + file_name_ + ":" + std::to_string(line_) + "]\t" + payload_ + "\n";
            ret << tmp1 << tid_ << tmp2; // 拼接线程ID和日志信息
            return ret.str();
        }

    public:
        size_t line_;           // 行号
        time_t ctime_;          // 时间戳
        std::string file_name_; // 文件名
        std::string name_;      // 日志器名称
        std::string payload_;   // 日志内容
        std::thread::id tid_;   // 线程ID
        LogLevel::value level_; // 日志级别
    };
}
