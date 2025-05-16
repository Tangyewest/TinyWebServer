#include "buffer.h"

// 构造函数，初始化缓冲区大小和读写位置
Buffer::Buffer(int size) : buffer_(size), readIndex_(0), writeIndex_(0) {}

// 返回可写字节数
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writeIndex_;
}

// 返回可读字节数
size_t Buffer::ReadableBytes() const {
    return writeIndex_ - readIndex_;
}

// 返回预留空间
size_t Buffer::prependableBytes() const {
    return readIndex_;
}

// 返回当前读指针位置
const char* Buffer::peek() const {
    return BeginPtr() + readIndex_;
}

// 确保有足够空间写入数据
void Buffer::ensureWritableBytes(size_t len) {
    if (WritableBytes() < len) {
        makeSpace(len);
    }
    assert(WritableBytes() >= len);
}

// 写入数据后移动写指针
void Buffer::HasWritten(size_t len) {
    writeIndex_ += len;
}

// 读取 len 字节后移动读指针
void Buffer::retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readIndex_ += len;
}

// 从当前读指针读取直到 end 指针
void Buffer::retrieveUntil(const char* end) {
    assert(peek() <= end);
    retrieve(end - peek());
}

// 清空所有内容并重置指针,读写下标归零,在别的函数中会用到
void Buffer::retrieveAll() {
    memset(buffer_.data(), 0, buffer_.size());
    readIndex_ = writeIndex_ = 0;
}

// 取出剩余可读的str
std::string Buffer::retrieveAllToString() {
    std::string str(peek(), ReadableBytes());
    retrieveAll();
    return str;
}

// 返回写指针位置（const 版本）
const char* Buffer::BeginWrite() const {
    return BeginPtr() + writeIndex_;
}

// 返回写指针位置
char* Buffer::BeginWrite() {
    return BeginPtr() + writeIndex_;
}

// 追加 std::string 数据
void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

// 追加 char* 数据
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    ensureWritableBytes(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

// 追加 void* 数据
void Buffer::Append(const void* data, size_t len) {
    Append(static_cast<const char*>(data), len);
}

// 追加另一个 Buffer 的内容
void Buffer::Append(const char* str) {
    Append(str, strlen(str));
}