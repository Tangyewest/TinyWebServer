#include "log.h"

//构造函数
Log::Log(){
    fp_ = nullptr;
    deque_ = nullptr;
    writeThread_ = nullptr;
    lineCount_ = 0;
    today_ = 0;
    isAsync_ = false;
}

//析构函数
Log::~Log(){
    while(!deque_->empty()){
        deque_->flush();//唤醒消费者,处理掉剩下的任务
    }
    deque_->close();//关闭队列
    writeThread_->join();//等待当前线程完成手中的任务
    if(fp_){
        flush();//清空缓冲区的数据
        fclose(fp_);//关闭文件
    }
}

//唤醒阻塞队列消费者，开始写日志
void Log::flush(){
    if(isAsync_){ // 只有异步日志才会用到deque
        deque_->flush();
    }
    fflush(fp_);//强制将缓冲区中的数据写入磁盘文件
}


//懒汉模式 局部静态变量法 不用加锁
Log* Log::Instance(){
    static Log log;
    return &log;
}

//异步日志的写线程函数
void Log::FlushLogThread(){
    Log::Instance()->asyncWrite();
}

//写线程真正的执行函数
void Log::asyncWrite(){
    string str = "";
    while(deque_->pop(str)){
        lock_guard<mutex> locker(mutex_);//加锁
        fputs(str.c_str(), fp_);//将字符串写入文件
    }
}

//初始化日志实例
void Log::init(int level, const char* path, const char* suffix, int maxQueueSize){
    isOpen_ = true;
    level_ = level;
    path_ = path;
    suffix_ = suffix;
    if(maxQueueSize){  //如果队列大小不为0，说明是异步日志
        isAsync_ = true;
        if(!deque_){ //如果队列为空，说明是第一次调用
            unique_ptr<BlockQueue<string>> newDeque(new BlockQueue<string>(maxQueueSize));
            // 因为unique_ptr不支持普通的拷贝或赋值操作,所以采用move
            // 将动态申请的内存权给deque，newDeque被释放
            deque_ = move(newDeque);//将新的阻塞队列赋值给deque_
            unique_ptr<thread> newThread(new thread(FlushLogThread));
            writeThread_ = move(newThread);
        }

    }
    else{
        isAsync_ = false;
    }

    lineCount_ = 0;
    time_t timer=time(nullptr);
    struct tm* systime = localtime(&timer);
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            path_, systime->tm_year + 1900, systime->tm_mon + 1, systime->tm_mday, suffix_);
    today_ = systime->tm_mday;

    {
        lock_guard<mutex> locker(mutex_);
        buffer_.retrieveAll(); // 清空缓冲区
        if(fp_) {   // 重新打开
            flush();
            fclose(fp_);
        }
        fp_ = fopen(fileName, "a"); // 打开文件读取并附加写入
        if(fp_ == nullptr) {
            mkdir(path,777);
            fp_ = fopen(fileName, "a"); // 生成目录文件（最大权限）
        }
        assert(fp_ != nullptr);
    }
}

void Log::write(int level, const char* format, ...){
    struct timeval now = {0,0};
    gettimeofday(&now, nullptr);
    time_t timer = now.tv_sec;
    struct tm* systime = localtime(&timer);
    struct tm t = *systime;
    va_list vaList;

    // 日志日期 日志行数  如果不是今天或行数超了
    if (today_ != t.tm_mday || (lineCount_ && (lineCount_  %  MAX_LINES == 0)))
    {
        unique_lock<mutex> locker(mutex_);
        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (today_ != t.tm_mday)    // 时间不匹配，则替换为最新的日志文件名
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            today_ = t.tm_mday;
            lineCount_ = 0;
        }
        else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_  / MAX_LINES), suffix_);
        }
        
        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }

    // 在buffer内生成一条对应的日志信息
    {
        unique_lock<mutex> locker(mutex);
        lineCount_++;//记录当前日志文件的总行数，用于触发按行数分割文件的逻辑
        int n = snprintf(buffer_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);//将当前时间（精确到微秒）格式化为字符串（如 2023-10-05 14:30:45.123456），并写入缓冲区。
                    
        buffer_.HasWritten(n);//更新缓冲区的写指针，表示已写入 n 字节。
        AppendLogLevelTitle(level);//根据日志级别（level）追加前缀标签    

        va_start(vaList, format);
        int m = vsnprintf(buffer_.BeginWrite(), buffer_.WritableBytes(), format, vaList);
        va_end(vaList);

        buffer_.HasWritten(m);//将用户通过可变参数（...）传递的日志内容（如 "Connection from IP: %s"）格式化到缓冲区。
        //vsnprintf 支持类似 printf 的格式化功能。
        //HasWritten(m) 再次更新写指针。

        buffer_.Append("\n\0", 2);//在日志末尾添加换行符 \n 和空终止符 \0，确保每条日志独立成行，且字符串正确终止。

        if(isAsync_ && deque_ && !deque_->full()) { // 异步方式（加入阻塞队列中，等待写线程读取日志信息）
            deque_->push_back(buffer_.retrieveAllToString());//将缓冲区内容转为字符串，推送到阻塞队列 deque_，由后台线程异步写入文件。
        } else {    // 同步方式（直接向文件中写入日志信息）
            fputs(buffer_.peek(), fp_);   // 同步就直接写入文件
        }
        buffer_.retrieveAllToString();    // 重置缓冲区的读写指针（readIndex_ 和 writeIndex_ 归零），清空当前内容，为下一条日志做准备。
    }

}


// 添加日志等级
void Log::AppendLogLevelTitle(int level) {
    switch(level) {
    case 0:
        buffer_.Append("[debug]: ", 9);
        break;
    case 1:
        buffer_.Append("[info] : ", 9);
        break;
    case 2:
        buffer_.Append("[warn] : ", 9);
        break;
    case 3:
        buffer_.Append("[error]: ", 9);
        break;
    default:
        buffer_.Append("[info] : ", 9);
        break;
    }
}

int Log::getLevel() {
    lock_guard<mutex> locker(mutex_);
    return level_;
}

void Log::setLevel(int level) {
    lock_guard<mutex> locker(mutex_);
    level_ = level;
}

