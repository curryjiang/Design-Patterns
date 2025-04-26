#pragma once
#include <sys/stat.h>
#include <sys/types.h>
#include <jsoncpp/json/json.h>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <system_error>
#include <memory>

namespace mylog
{
    namespace Util
    {
        class Date
        {
        public:
            // 获取当前时间
            static time_t Now() { return time(nullptr); }
        };

        class File
        {
        public:
            // 判断文件路径是否存在
            static bool Exists(const std::string &filename)
            {
                struct stat st;
                return (0 == stat(filename.c_str(), &st));
            }

            // 提取文件的目录路径
            static std::string Path(const std::string &filename)
            {
                if (filename.empty())
                {
                    return "";
                }
                int it = filename.find_last_of("/\\");
                if (it != std::string::npos)
                {
                    return filename.substr(0, it + 1);
                }
                else
                {
                    return "";
                }
            }

            // 创建当前文件路径
            static void CreateDirectory(const std::string &filename)
            {
                if (filename.empty())
                {
                    throw std::invalid_argument("Path cannot be empty");
                }
                std::error_code error;
                if (!std::filesystem::create_directories(filename, error))
                {
                    throw std::runtime_error("Failed to create directory: " + error.message());
                }
            }

            // 获取文件大小
            int64_t FileSize(std::string filename)
            {
                struct stat st;
                if (-1 == stat(filename.c_str(), &st))
                {
                    perror("Get file size failed");
                    return -1;
                }
                return st.st_size;
            }

            // 读取文件内容到content
            bool GetContent(std::string *content, std::string filename)
            {
                std::ifstream ifs;
                ifs.open(filename.c_str(), std::ios::binary);
                if (ifs.is_open() == false)
                {
                    std::cout << "file open error" << std::endl;
                    return false;
                }

                // 读入content
                ifs.seekg(0, std::ios::beg);
                size_t len = FileSize(filename);
                content->resize(len);
                ifs.read(&(*content)[0], len);
                if (!ifs.good())
                {
                    std::cout << __FILE__ << __LINE__ << "-" << "read file content error" << std::endl;
                    ifs.close();
                    return false;
                }
                ifs.close();
                return true;
            }
        };

        class JsonUtil
        {
        public:
            // 序列化json->string
            static bool Serialize(const Json::Value &val, std::string *str)
            {
                Json::StreamWriterBuilder swb;
                std::unique_ptr<Json::StreamWriter> usw(swb.newStreamWriter());
                std::stringstream ss;
                if (usw->write(val, &ss) != 0)
                {
                    std::cout << "serialize error" << std::endl;
                    return false;
                }
                *str = ss.str();
                return true;
            }

            // 反序列化string->json
            static bool UnSerialize(const std::string &str, Json::Value *val)
            {
                Json::CharReaderBuilder crb;
                std::unique_ptr<Json::CharReader> ucr(crb.newCharReader());
                std::string err;
                if (ucr->parse(str.c_str(), str.c_str() + str.size(), val, &err) == false)
                {
                    std::cout << __FILE__ << __LINE__ << "parse error" << err << std::endl;
                    return false;
                }
                return false;
            }
        };

        class JsonData
        {
        public:
            static JsonData *GetJsonData()
            {
                static JsonData *json_data = new JsonData;
                return json_data;
            }
           
            //禁用拷贝和赋值
            JsonData(const JsonData& obj) = delete;
            JsonData& operator=(const JsonData& obj) = delete;

        private:
            JsonData()
            {
                std::string content;
                mylog::Util::File file;
                if (file.GetContent(&content, "../../log_system/logs_code/config.conf") == false)
                {
                    std::cout << __FILE__ << __LINE__ << "open config.conf failed" << std::endl;
                    perror(NULL);
                }
                Json::Value root;
                mylog::Util::JsonUtil::UnSerialize(content, &root); // 反序列化，把内容转成jaon value格式
                buffer_size = root["buffer_size"].asInt64();
                threshold = root["threshold"].asInt64();
                linear_growth = root["linear_growth"].asInt64();
                flush_log = root["flush_log"].asInt64();
                backup_addr = root["backup_addr"].asString();
                backup_port = root["backup_port"].asInt();
                thread_count = root["thread_count"].asInt();
            }

        public:
            size_t buffer_size;   // 缓冲区基础容量
            size_t threshold;     // 倍数扩容阈值
            size_t linear_growth; // 线性增长容量
            size_t flush_log;     // 控制日志同步到磁盘的时机，默认为0,1调用fflush，2调用fsync
            std::string backup_addr;
            uint16_t backup_port;
            size_t thread_count;
        };
    }
}
