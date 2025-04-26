#pragma once
#include <cassert>
#include <fstream>
#include <memory>
#include <unistd.h>
#include "Util.hpp"

extern mylog::Util::JsonData *g_conf_data;

namespace mylog
{
    // 日志刷新基类
    class LogFlush
    {
    public:
        using ptr = std::shared_ptr<LogFlush>;
        virtual ~LogFlush() = default;
        // 纯虚函数，定义日志刷新接口
        virtual void Flush(const char *data, size_t len) = 0;
    };

    // 将日志输出到标准输出的实现类
    class StdoutFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<StdoutFlush>;
        void Flush(const char *data, size_t len) override
        {
            std::cout.write(data, len); // 将日志写入标准输出
        }
    };

    // 将日志输出到文件的实现类
    class FileFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<FileFlush>;
        FileFlush(const std::string &filename) : filename_(filename)
        {
            // 创建目录并打开文件
            Util::File::CreateDirectory(Util::File::Path(filename));
            fs_ = fopen(filename.c_str(), "ab");
            if (fs_ == NULL)
            {
                // 打开文件失败时输出错误信息
                std::cout << __FILE__ << __LINE__ << "open file " << filename << " failed" << std::endl;
                perror(NULL);
            }
        }

        void Flush(const char *data, size_t len) override
        {
            // 将日志写入文件
            fwrite(data, 1, len, fs_);
            if (ferror(fs_))
            {
                // 写入失败时输出错误信息
                std::cout << __FILE__ << __LINE__ << "write file " << filename_ << " failed" << std::endl;
                perror(NULL);
            }

            // 根据配置决定是否刷新文件缓冲区
            if (g_conf_data->flush_log == 1)
            {
                if (fflush(fs_) == EOF)
                {
                    std::cout << __FILE__ << __LINE__ << "flush file " << filename_ << " failed" << std::endl;
                    perror(NULL);
                }
            }
            else if (g_conf_data->flush_log == 2)
            {
                fflush(fs_);
                fsync(fileno(fs_));
            }
        }

    private:
        std::string filename_; // 文件名
        FILE *fs_ = NULL;      // 文件指针
    };

    // 支持日志文件滚动的实现类
    class RollFileFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<RollFileFlush>;
        RollFileFlush(const std::string &filename, size_t max_size) : basename_(filename), max_size_(max_size)
        {
            // 创建目录
            Util::File::CreateDirectory(Util::File::Path(filename));
        }

        void Flush(const char *data, size_t len) override
        {
            InitLogFile(); // 初始化日志文件
            fwrite(data, 1, len, fs_);
            if (ferror(fs_))
            {
                // 写入失败时输出错误信息
                std::cout << __FILE__ << __LINE__ << "write file " << basename_ << " failed" << std::endl;
                perror(NULL);
            }

            cur_size_ += len; // 更新当前文件大小
            // 根据配置决定是否刷新文件缓冲区
            if (g_conf_data->flush_log == 1)
            {
                if (fflush(fs_))
                {
                    std::cout << __FILE__ << __LINE__ << "flush file " << basename_ << " failed" << std::endl;
                    perror(NULL);
                }
            }
            else if (g_conf_data->flush_log == 2)
            {
                fflush(fs_);
                fsync(fileno(fs_));
            }
        }

    private:
        // 初始化日志文件
        void InitLogFile()
        {
            if (fs_ == NULL || cur_size_ >= max_size_)
            {
                if (fs_ != NULL)
                {
                    fclose(fs_); // 关闭当前文件
                    fs_ = NULL;
                }
                std::string filename = CreateFileName(); // 创建新的文件名
                fs_ = fopen(filename.c_str(), "ab");
                if (fs_ == NULL)
                {
                    // 打开文件失败时输出错误信息
                    std::cout << __FILE__ << __LINE__ << "open file " << filename << " failed" << std::endl;
                    perror(NULL);
                }
                cur_size_ = 0; // 重置当前文件大小
            }
        }

        // 创建新的日志文件名
        std::string CreateFileName()
        {
            time_t time_ = Util::Date::Now();
            struct tm t;
            localtime_r(&time_, &t);
            // 根据时间和计数器生成文件名
            std::string filename = basename_ + std::to_string(t.tm_year + 1900) + std::to_string(t.tm_mon + 1) + std::to_string(t.tm_mday) + std::to_string(t.tm_hour + 1) + std::to_string(t.tm_min + 1) + std::to_string(t.tm_sec + 1) + "-" + std::to_string(cnt_++) + ".log";
            return filename;
        }

        size_t cnt_ = 1;           // 文件计数器
        size_t max_size_;          // 文件最大大小
        size_t cur_size_ = 0;      // 当前文件大小
        std::string basename_;     // 基础文件名
        FILE *fs_ = NULL;          // 文件指针
    };


    // 日志刷新工厂类，用于创建不同类型的日志刷新对象
    class LogFlushFactory
    {
    public:
        using ptr = std::shared_ptr<LogFlushFactory>;

        // 模板函数，用于创建指定类型的日志刷新对象
        // FlushType: 日志刷新类型
        // Args: 构造函数参数
        template <typename FlushType, typename... Args>
        static std::shared_ptr<LogFlush> CreateLog(Args &&... args)
        {
            // 使用完美转发创建日志刷新对象
            return std::make_shared<FlushType>(std::forward<Args>(args)...);
        }
    };
}
