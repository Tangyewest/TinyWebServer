#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           
#include <assert.h>
#include <sys/stat.h>         
// #include "blockqueue.h"
#include "../buffer/blockqueue.h"
#include "../buffer/buffer.h"

class Log{
public: 
        //初始化日志实例（阻塞队列最大容量、日志保存路径、日志文件后缀）
        void init(int level, const char* path="./log",const char* suffix=".log",int maxQueueSize=1024);

        static Log* Instance();
        static void FlushLogThread();//异步写日志公有方法，调用私有方法asyncWrite

        void write(int level, const char* format, ...);//将输出内容按照标准格式整理 三个点是可变参数
        void flush();

        int getLevel();//获取日志等级
        void setLevel(int level);//设置日志等级
        bool isOpen(){return isOpen_;}//判断日志文件是否打开

private:
        Log();
        void AppendLogLevelTitle(int level);//添加日志等级标题
        virtual ~Log();
        void asyncWrite();//异步写日志方法


private:
        static const int LOG_PATH_LEN = 256;//日志路径长度
        static const int LOG_NAME_LEN = 256;//日志名称长度
        static const int MAX_LINES = 50000;//最大行数

        const char* path_; //日志路径
        const char* suffix_; //日志后缀

        int MAX_LINES_; //最大行数

        int lineCount_; //当前行数
        int today_; //按当天日期区分文件

        bool isOpen_; //日志文件是否打开

        Buffer buffer_; //输出的内容，缓冲区
        int level_; //日志等级
        bool isAsync_; //是否开启异步日志

        FILE* fp_; //打开log的文件指针
        std::unique_ptr<BlockQueue<std::string>> deque_; //阻塞队列
        std::unique_ptr<std::thread> writeThread_; //写线程的指针
        std::mutex mutex_; //互斥锁
};

#define LOG_BASE(level,format,...) \
        do {\
        Log* log = Log::Instance();\
        if (log->isOpen() && log->getLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);


// 四个宏定义，主要用于不同类型的日志输出，也是外部使用日志的接口
// ...表示可变参数，__VA_ARGS__就是将...的值复制到这里
// 前面加上##的作用是：当可变参数的个数为0时，这里的##可以把把前面多余的","去掉,否则会编译出错。
#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);    
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);
#endif