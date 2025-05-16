#ifndef BUFFER_H
#define BUFFER_H
#include <cstring>   //perror
#include <iostream>
#include <unistd.h>  // write
//#include <sys/uio.h> //readv
#include <vector> //readv
#include <atomic>
#include <assert.h>

class Buffer {
public:
       Buffer(int size = 1024);
       ~Buffer()= default;

       size_t WritableBytes() const;
       size_t ReadableBytes() const;
       size_t prependableBytes() const;
       
       const char* peek() const;
       void ensureWritableBytes(size_t len);
       void HasWritten(size_t len);
       
       void retrieve(size_t len);// 读取len字节
       void retrieveUntil(const char* end);// 读取到end

       void retrieveAll();// 读取所有
       std::string retrieveAllToString();// 读取所有并返回字符串
       
       const char* BeginWrite() const;
       char* BeginWrite();

       //根据不同的数据类型，返回不同的值，提升了类的易用性和兼容性。
       void Append(const std::string& str);
       void Append(const char* str, size_t len);
       void Append(const void* data, size_t len);
       void Append(const char* str);

       ssize_t ReadFd(int fd,int* Errno); // 读取fd的内容到buffer
       ssize_t WriteFd(int fd, int* Errno);// 写入buffer的内容到fd

private:
       char* BeginPtr();
       const char* BeginPtr( ) const;
       void makeSpace(size_t len);// 扩展空间

       std::vector<char> buffer_; // 缓冲区
       std::atomic<size_t> readIndex_; // 读下标
       std::atomic<size_t> writeIndex_; // 写下标
       /*对 readIndex_ 和 writeIndex_ 的读写操作是不可分割的，
       不会被线程切换中断，保证数据一致性,线程安全*/
};

#endif