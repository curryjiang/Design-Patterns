#pragma once
#include <memory>
#include <mutex>
#include <functional>
#include <string>
#include <vector>
#include <atomic>
#include <vector>
#include <memory>

#include "Util.hpp"
#include "LogFlush.hpp"
#include "Message.hpp"
#include "ThreadPool.hpp"
#include "AsyncBuffer.hpp"
#include "AsyncWorker.hpp"
#include "Level.hpp"

extern ThreadPool *thread_pool;

namespace mylog
{
  // 异步日志类，负责日志的异步记录和处理
  class AsyncLogger
  {
  public:
    using ptr = std::shared_ptr<AsyncLogger>;

    // 构造函数，初始化日志名称、日志输出方式和异步工作者
    AsyncLogger(const std::string &logger_name, std::vector<LogFlush::ptr> &flushs, AsyncType type)
        : logger_name_(logger_name), flushs_(flushs.begin(), flushs.end()),
          async_worker_(std::make_shared<AsyncWorker>(std::bind(&AsyncLogger::RealFlush, this, std::placeholders::_1), type)) {}

    virtual ~AsyncLogger() {};

    // 获取日志名称
    std::string Name() { return logger_name_; }

    // 调试级别日志记录
    void Debug(const std::string &file, size_t line, const std::string format, ...)
    {
      va_list va;
      va_start(va, format);
      char *ret;
      if (-1 == vasprintf(&ret, format.c_str(), va))
      {
        perror("vasprintf failed!!");
      }
      va_end(va);
      serialize(LogLevel::value::DEBUG, file, line, ret);
      free(ret);
      ret = nullptr;
    }

    // 信息级别日志记录
    void Info(const std::string &file, size_t line, const std::string format, ...)
    {
      va_list va;
      va_start(va, format);
      char *ret;
      if (-1 == vasprintf(&ret, format.c_str(), va))
      {
        perror("vasprintf failed!!");
      }
      va_end(va);
      serialize(LogLevel::value::INFO, file, line, ret);
      free(ret);
      ret = nullptr;
    }

    // 警告级别日志记录
    void Warn(const std::string &file, size_t line, const std::string format, ...)
    {
      va_list va;
      va_start(va, format);
      char *ret;
      if (-1 == vasprintf(&ret, format.c_str(), va))
      {
        perror("vasprintf failed!!");
      }
      va_end(va);
      serialize(LogLevel::value::WARN, file, line, ret);
      free(ret);
      ret = nullptr;
    }

    // 错误级别日志记录
    void Error(const std::string &file, size_t line, const std::string format, ...)
    {
      va_list va;
      va_start(va, format);
      char *ret;
      if (-1 == vasprintf(&ret, format.c_str(), va))
      {
        perror("vasprintf failed!!");
      }
      va_end(va);
      serialize(LogLevel::value::ERROR, file, line, ret);
      free(ret);
      ret = nullptr;
    }

    // 致命错误级别日志记录
    void Fatal(const std::string &file, size_t line, const std::string format, ...)
    {
      va_list va;
      va_start(va, format);
      char *ret;
      if (-1 == vasprintf(&ret, format.c_str(), va))
      {
        perror("vasprintf failed!!");
      }
      va_end(va);
      serialize(LogLevel::value::FATAL, file, line, ret);
      free(ret);
      ret = nullptr;
    }

  protected:
    // 序列化日志信息，将日志信息格式化并传递给异步工作者
    void serialize(LogLevel::value level, const std::string &file, size_t line, char *ret)
    {
      LogMessage msg(file, line, level, logger_name_, ret);
      std::string data = msg.format();

      // 对于紧急日志（FATAL或ERROR），进行备份
      if (level == LogLevel::value::FATAL || level == LogLevel::value::ERROR)
      {
        // try
        // {
        //   auto ret = thread_pool->enqueue(start_backup, data);
        //   ret.get();
        // }
        // catch (const std::runtime_error &err)
        // {
        //   std::cout << __FILE__ << __LINE__ << "thread pool closed" << std::endl;
        // }
      }

      // 将日志数据推送到异步工作者
      Flush(data.c_str(), data.size());
    }

    // 将日志数据推送到异步工作者
    void Flush(const char *data, size_t len)
    {
      async_worker_->Push(data, len);
    }

    // 实际的日志刷新操作，将日志数据写入到指定的输出方式
    void RealFlush(Buffer &buffer)
    {
      if (flushs_.empty())
      {
        return;
      }
      for (auto &e : flushs_)
      {
        e->Flush(buffer.Begin(), buffer.ReadableSize());
      }
    }

  private:
    std::mutex mutex_;                          // 互斥锁，保护共享资源
    std::string logger_name_;                   // 日志名称
    std::vector<LogFlush::ptr> flushs_;         // 日志输出方式集合
    mylog::AsyncWorker::ptr async_worker_;      // 异步工作者，用于异步处理日志
  };

  // 日志构建器类，用于构建异步日志对象
  class LoggerBuilder
  {
  public:
    using ptr = std::shared_ptr<LoggerBuilder>;

    // 设置日志名称
    void BuildLoggerName(const std::string &name)
    {
      logger_name_ = name;
    }

    // 设置日志类型（异步类型）
    void BuildLoggerType(AsyncType type)
    {
      async_type_ = type;
    }

    // 添加日志输出方式
    template <typename FlushType, typename... Args>
    void BuildLoggerFlush(Args &&...args)
    {
      flushs_.emplace_back(LogFlushFactory::CreateLog<FlushType>(std::forward<Args>(args)...));
    }

    // 构建异步日志对象
    AsyncLogger::ptr Build()
    {
      assert(logger_name_.empty() == false);

      // 如果没有指定日志输出方式，默认使用标准输出
      if (flushs_.empty())
      {
        flushs_.emplace_back(std::make_shared<StdoutFlush>());
      }
      return std::make_shared<AsyncLogger>(logger_name_, flushs_, async_type_);
    }

  private:
    std::string logger_name_ = "async_logger";  // 日志名称，默认为"async_logger"
    std::vector<mylog::LogFlush::ptr> flushs_; // 日志输出方式集合
    AsyncType async_type_ = AsyncType::ASYNC_SAFE; // 异步类型，默认为安全异步
  };
}
