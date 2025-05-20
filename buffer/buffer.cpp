#include "buffer.h"
#include <sys/uio.h>

//都是读到可写区，写入fd是从读区readable开始写入
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
    return &buffer_[readIndex_];
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
const char* Buffer::BeginWriteConst() const {
    return &buffer_[writeIndex_];
}

// 返回写指针位置
char* Buffer::BeginWrite() {
    return &buffer_[writeIndex_];
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

// 追加 void* 数据，向缓冲区追加数据的重载函数
void Buffer::Append(const void* data, size_t len) {
    Append(static_cast<const char*>(data), len);
}

//主要功能时将另一个Buffer对象中的可读数据追加到当前Buffer对象中
void Buffer::Append(const Buffer& buffer){
    Append(buffer.peek(), buffer.ReadableBytes());
}

//将fd的内容读到缓冲区，既要读到writable的位置
ssize_t Buffer::ReadFd(int fd,int* Errno){
    char extraBuf[65536];//栈区
    struct iovec iov[2];
    size_t writeable = WritableBytes();//先记录能写多少
    // 分散读
    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writeable;
    iov[1].iov_base = extraBuf;
    iov[1].iov_len = sizeof(extraBuf);

    ssize_t len = readv(fd,iov,2);
    if(len < 0)
    {
        *Errno = errno;
    }
    else if(static_cast<size_t>(len)<writeable)//如果len小于writeable，说明写区能容纳len，就用不着栈上临时缓冲区
    {
       writeIndex_+= len;
    }
    else
    {
        writeIndex_ = buffer_.size();
        Append(extraBuf,static_cast<size_t>(len-writeable));//将栈上剩余的内容追加到buffer中
    }
    return len;
}


ssize_t Buffer::WriteFd(int fd, int* Errno){
    ssize_t len = write(fd,peek(),ReadableBytes());//从读区开始写入
    if(len < 0)
    {
        *Errno = errno;
        return len;
    }
    retrieve(len);
    return len;
}

char* Buffer::BeginPtr(){
    return &buffer_[0];
}

const char* Buffer::BeginPtr() const {
    return &buffer_[0];
}

void Buffer::makeSpace(size_t len){
    if(WritableBytes() + prependableBytes() < len)//第一种情况:需要扩容
    {
        buffer_.resize(writeIndex_ + len + 1);
    }
    else/*第二种情况：内部腾空间 当现有空间足够时，通过移动数据来腾出空间
将可读数据移动到缓冲区开始位置
重置读写指针*/
    {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr() + readIndex_, BeginPtr() + writeIndex_, BeginPtr());//将可读区的内容拷贝到缓冲区的开头
        readIndex_ = 0;
        writeIndex_ = readable;
        assert(readable == ReadableBytes());
    }
}