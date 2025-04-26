#pragma once
#include <cassert>
#include <iostream>
#include <string>
#include <cstddef>
#include <vector>
#include "Util.hpp"

extern mylog::Util::JsonData *g_conf_data;

namespace mylog
{
    class Buffer
    {
    public:
        Buffer() : write_pos_(0), read_pos_(0)
        {
            // 初始化缓冲区大小，使用全局配置数据中的缓冲区大小
            buffer_.resize(g_conf_data->buffer_size);
        }

        // 将数据写入缓冲区
        void Push(const char *data, size_t len)
        {
            ToBeEnough(len); // 确保缓冲区有足够的空间
            std::copy(data, data + len, buffer_.begin() + write_pos_); // 将数据复制到缓冲区
            write_pos_ += len; // 更新写指针位置
        }

        // 获取可读数据的起始位置
        char *ReadBegin(int len)
        {
            assert(len <= ReadableSize()); // 确保读取长度不超过可读大小
            return &buffer_[read_pos_]; // 返回可读数据的起始地址
        }

        // 判断缓冲区是否为空
        bool IsEmpty()
        {
            return write_pos_ == read_pos_; // 写指针和读指针相等时，缓冲区为空
        }

        // 交换两个缓冲区的内容
        void Swap(Buffer &buf)
        {
            buffer_.swap(buf.buffer_); // 交换缓冲区内容
            std::swap(write_pos_, buf.write_pos_); // 交换写指针位置
            std::swap(read_pos_, buf.read_pos_); // 交换读指针位置
        }

        // 获取缓冲区的可写大小
        size_t WriteableSize()
        {
            return buffer_.size() - write_pos_; // 缓冲区总大小减去写指针位置
        }

        // 获取缓冲区的可读大小
        size_t ReadableSize()
        {
            return write_pos_ - read_pos_; // 写指针位置减去读指针位置
        }

        // 获取缓冲区的起始位置
        const char *Begin()
        {
            return &buffer_[read_pos_]; // 返回读指针位置的地址
        }

        // 移动写指针位置
        void MoveWritePos(int len)
        {
            assert(len <= ReadableSize()); // 确保移动长度不超过可读大小
            write_pos_ += len; // 更新写指针位置
        }

        // 移动读指针位置
        void MoveReadPos(int len)
        {
            assert(len <= ReadableSize()); // 确保移动长度不超过可读大小
            read_pos_ += len; // 更新读指针位置
        }

        // 重置缓冲区
        void Reset()
        {
            read_pos_ = 0; // 重置读指针位置
            write_pos_ = 0; // 重置写指针位置
        }

    protected:
        // 确保缓冲区有足够的空间
        void ToBeEnough(size_t len)
        {
            int buffer_size = buffer_.size();
            if (len >= WriteableSize()) // 如果需要写入的数据长度超过可写空间
            {
                if (buffer_.size() < g_conf_data->threshold) // 如果缓冲区大小小于阈值
                {
                    // 按倍数扩展缓冲区大小
                    buffer_.resize(2 * buffer_.size() + buffer_size);
                }
                else
                {
                    // 如果缓冲区大小超过阈值，则线性增长
                    buffer_.resize(g_conf_data->linear_growth + buffer_size);
                }
            }
        }

    protected:
        std::vector<char> buffer_; // 缓冲区
        size_t write_pos_;         // 写指针位置
        size_t read_pos_;          // 读指针位置
    };
}