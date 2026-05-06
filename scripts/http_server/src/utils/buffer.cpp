#include "utils/buffer.h"

// 构造函数,多分配一个字节的空间来区分满和空
Buffer::Buffer(size_t size) : _readIndex(0), _writeIndex(0), _size(size + 1), _buffer(size + 1) {}

// 获取当前写位置
uint64_t Buffer::GetWriteIndex() const { return this->_writeIndex; }

// 获取当前读位置
uint64_t Buffer::GetReadIndex() const { return this->_readIndex; }

// 清理缓冲区
void Buffer::Clear()
{
    this->_writeIndex = 0;
    this->_readIndex = 0;
}

// 获取当前可读数据大小
uint64_t Buffer::GetReadableSize() const { return (this->_writeIndex - this->_readIndex + this->_size) % this->_size; }

// 获取当前可写空间大小
uint64_t Buffer::GetWriteableSize() const
{
    return this->_size - this->GetReadableSize() - 1;  // 留一个字节区分满和空
}

// 读指针偏移len
bool Buffer::MoveReadIndex(uint64_t len)
{
    if (len > this->GetReadableSize())  // 超过可读数据大小,无法移动
        return false;
    this->_readIndex = (this->_readIndex + len) % this->_size;
    return true;
}

// 写指针偏移len
bool Buffer::MoveWriteIndex(uint64_t len)
{
    if (len > this->GetWriteableSize())  // 超过可写空间大小,无法移动
        return false;
    this->_writeIndex = (this->_writeIndex + len) % this->_size;
    return true;
}

// 获取新的缓冲区大小
uint64_t Buffer::_GetNewSize(uint64_t size)
{
    // 剩余空间
    uint64_t spaceSize = this->GetWriteableSize();
    uint64_t newSize = 0;
    if (spaceSize + this->_size < size)
    {
        // 剩余空间加上当前缓冲区大小都不足以满足size,则新缓冲区大小为当前缓冲区大小的2倍加上size
        newSize = this->_size * 2 + size;
    }
    else
    {
        // 否则新缓冲区大小为当前缓冲区大小的2倍
        newSize = this->_size * 2;
    }
    return newSize;
}

// 确保有size大小的可写空间
bool Buffer::_HaveSpace(uint64_t size)
{
    if (size > this->GetWriteableSize())
    {
        // 扩容
        uint64_t newSize = this->_GetNewSize(size);
        try
        {
            std::vector<char> newBuffer(newSize);
            // 复制数据到新缓冲区
            uint64_t readableSize = this->GetReadableSize();
            for (uint64_t i = 0; i < readableSize; ++i)
            {
                newBuffer[i] = this->_buffer[(this->_readIndex + i) % this->_size];
            }
            // 更新缓冲区和索引
            this->_buffer = std::move(newBuffer);
            this->_size = newSize;
            this->_readIndex = 0;
            this->_writeIndex = readableSize;
        }
        catch (const std::exception &e)
        {
            // 分配失败
            return false;
        }
    }
    return true;
}

// 写入数据
bool Buffer::Write(const void *data, uint64_t len)
{
    if (this->_HaveSpace(len) == false) return false;  // 没有足够的空间写入数据

    // 写入数据
    const char *charData = static_cast<const char *>(data);
    for (uint64_t i = 0; i < len; ++i)
    {
        this->_buffer[this->_writeIndex] = charData[i];
        this->_writeIndex = (this->_writeIndex + 1) % this->_size;
    }
    return true;
}
bool Buffer::Write(const std::string &data) { return this->Write(data.data(), data.size()); }
bool Buffer::Write(const Buffer &buffer)
{
    uint64_t readableSize = buffer.GetReadableSize();
    if (readableSize == 0) return true;  // 没有数据可写入

    return this->Write(buffer._buffer.data() + buffer._readIndex, readableSize);
}

// 读取数据
bool Buffer::Read(void *data, uint64_t len)
{
    if (len > this->GetReadableSize() || data == nullptr) return false;  // 没有足够的数据可读

    // 读取数据
    char *charData = static_cast<char *>(data);
    for (uint64_t i = 0; i < len; ++i)
    {
        charData[i] = this->_buffer[this->_readIndex];
        this->_readIndex = (this->_readIndex + 1) % this->_size;
    }
    return true;
}

std::string Buffer::Read(uint64_t len)
{
    if (len > this->GetReadableSize()) return "";  // 没有足够的数据可读

    std::string result;
    result.resize(len);
    this->Read(&result[0], len);  // 直接读取到字符串的内存中
    return result;
}

// 从当前读取位置读到\n(一行数据,不包括换行)
// include_newline参数控制当没有换行符的时候读取是否要全部读完,true代表全部读完(默认)
std::string Buffer::ReadLine(bool include_newline)
{
    uint64_t readableSize = this->GetReadableSize();
    for (uint64_t i = 0; i < readableSize; ++i)
    {
        char c = this->_buffer[(this->_readIndex + i) % this->_size];
        if (c == '\n')
        {
            std::string result(i, 0);   // 分配 i 个字节（不包括换行符）
            this->Read(&result[0], i);  // 读走 i 个字节
            this->MoveReadIndex(1);     // 跳过换行符
            return result;
        }
    }
    // 没有遇到换行符，返回剩余内容(最后一行)
    if (readableSize > 0 && include_newline == true)
    {
        std::string result(readableSize, 0);
        this->Read(&result[0], readableSize);
        return result;
    }
    return "";
}

// 获取缓冲区大小
uint64_t Buffer::GetSize() const { return this->_size - 1; }