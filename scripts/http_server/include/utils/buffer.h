#include "utils/public.h"
#pragma once
#define BUFFER_DEFAULT_SIZE 1024
// Buffer类定义了一个简单的缓冲区,使用std::vector<char>来存储数据,并维护读写索引.
class Buffer
{
   private:
    std::vector<char> _buffer;
    uint64_t _readIndex;
    uint64_t _writeIndex;
    uint64_t _size;

    // 确保有size大小的可写空间
    bool _HaveSpace(uint64_t size);

    // 获取新的缓冲区大小
    uint64_t _GetNewSize(uint64_t size);

   public:
    Buffer(size_t size = BUFFER_DEFAULT_SIZE);
    // 获取当前写位置
    uint64_t GetWriteIndex() const;
    // 获取当前读位置
    uint64_t GetReadIndex() const;
    // 清空缓冲区
    void Clear();

    // 获取当前可读数据大小
    uint64_t GetReadableSize() const;

    // 获取当前可写空间大小
    uint64_t GetWriteableSize() const;

    // 读指针偏移len
    bool MoveReadIndex(uint64_t len);

    // 写指针偏移len
    bool MoveWriteIndex(uint64_t len);

    // 写入数据
    bool Write(const void *data, uint64_t len);
    bool Write(const std::string &data);
    bool Write(const Buffer &buffer);
    // 读取数据
    bool Read(void *data, uint64_t len);
    std::string Read(uint64_t len);

    // 从当前读取位置读到\n(一行数据,不包括换行)
    std::string ReadLine(bool include_newline = true);

    // 获取当前缓冲区大小
    uint64_t GetSize() const;
};