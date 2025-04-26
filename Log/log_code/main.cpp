#include <iostream>
#include <thread>
#include <vector>
#include "MyLog.hpp" // 假设你的日志系统头文件是mylog.cpp
#include "LogFlush.hpp"
#include "Util.hpp"

// 定义全局变量（必须在所有包含LogFlush.hpp的文件之前）
mylog::Util::JsonData* g_conf_data = mylog::Util::JsonData::GetJsonData();


using namespace mylog;

// 测试函数 - 模拟多线程日志记录
void thread_func(int thread_id) {
    for (int i = 0; i < 5; ++i) {
        // 使用默认日志器
        LOGINFODEFAULT("Thread %d - Default logger message %d", thread_id, i);
        
        // 获取特定日志器
        auto logger = GetLogger("test_logger");
        if (logger) {
            logger->Debug("Thread %d - Debug message %d", thread_id, i);
            logger->Info("Thread %d - Info message %d", thread_id, i);
            logger->Warn("Thread %d - Warn message %d", thread_id, i);
            
            if (i == 3) {
                logger->Error("Thread %d - Error message %d", thread_id, i);
            }
            
            if (i == 4) {
                logger->Fatal("Thread %d - Fatal message %d", thread_id, i);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    // 1. 测试默认日志器
    LOGINFODEFAULT("Starting async logger test...");
    LOGWARNDEFAULT("This is a warning from default logger");
    
    // 2. 创建一个新的日志器
    {
        std::unique_ptr<LoggerBuilder> builder(new LoggerBuilder());
        builder->BuildLoggerName("test_logger");
        builder->BuildLoggerFlush<StdoutFlush>(); // 输出到控制台
        builder->BuildLoggerFlush<FileFlush>("test.log"); // 同时输出到文件
        builder->BuildLoggerType(AsyncType::ASYNC_SAFE);
        
        auto logger = builder->Build();
        LoggerManager::GetInstance().AddLogger(std::move(logger));
    }
    
    // 3. 测试多线程日志记录
    const int num_threads = 5;
    std::vector<std::thread> threads;
    
    LOGINFODEFAULT("Creating %d worker threads...", num_threads);
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_func, i+1);
    }
    
    // 4. 主线程也记录一些日志
    auto main_logger = GetLogger("test_logger");
    for (int i = 0; i < 3; ++i) {
        main_logger->Info("Main thread message %d", i);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // 5. 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 6. 测试错误和致命日志
    main_logger->Error("This is an error message from main thread");
    main_logger->Fatal("This is a fatal message from main thread");
    
    LOGINFODEFAULT("All test threads completed");
    LOGWARNDEFAULT("Test finished, checking log files...");
    
    return 0;
}