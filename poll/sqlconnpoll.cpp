#include "sqlconnpool.h"
#include "log.h"

SqlConnPool::SqlConnPool() {
    // 可以初始化成员变量，也可以留空
}

SqlConnPool* SqlConnPool::Instance(){
    static SqlConnPool poll;
    return &poll;
}

void SqlConnPool::Init(const char*host,int port,const char* user,const char* passwd,const char* dbName,int connSize=20){
    assert(connSize>0);
    for(int i=0;i<connSize;i++)
    {
        MYSQL* conn = nullptr;
        conn = mysql_init(conn);
        if(!conn)
        {
            LOG_ERROR("Mysql init error");
            assert(conn);
        }
        conn = mysql_real_connect(conn,host,user,passwd,dbName,port,nullptr,0);
        if(!conn)
        {
            LOG_ERROR("Mysql connect error");
            assert(conn);
        }
        conn_queue.emplace(conn);//这句代码的作用是将连接对象conn放入连接池的队列中
    }                                  
    max_CONN_SIZE = connSize;
    sem_init(&sem,0,max_CONN_SIZE);//&sem：信号量变量的地址，表示该信号量是线程间共享（而不是进程间共享）。
                                       //0：表示该信号量是线程间共享（而不是进程间共享）。
                                       //max_CONN_SIZE：信号量的初始值，通常等于连接池的最大连接数。
    LOG_INFO("Mysql connect pool init success");
}

MYSQL* SqlConnPool::GetConn(){
    MYSQL* conn = nullptr;
    if(conn_queue.empty())
    {
        LOG_ERROR("conn queue is empty");
        return nullptr;
    }
    sem_wait(&sem);//如果 sem 的值大于 0，则 sem 的值减 1，函数立即返回，表示成功获取到一个资源（比如数据库连接）。
    std::lock_guard<std::mutex> locker(mutex);
    conn = conn_queue.front();//获取队列的第一个连接
    conn_queue.pop();//将队列的第一个连接弹出
    return conn;//返回获取到的连接给需要的线程使用
}

void SqlConnPool::FreeConn(MYSQL* conn){
    assert(conn);
    std::lock_guard<std::mutex> locker(mutex);
    conn_queue.push(conn);//把归还的连接放回队列
    sem_post(&sem);//信号量加1，表示有一个连接可用
}

void SqlConnPool::ClosePool(){
    std::lock_guard<std::mutex> locker(mutex);
    while(!conn_queue.empty())
    {
        MYSQL* conn = conn_queue.front();
        conn_queue.pop();
        mysql_close(conn);//关闭连接
    }
    mysql_library_end();//结束mysql库
    sem_destroy(&sem);//销毁信号量
    LOG_INFO("Mysql connect pool close success");
}

int SqlConnPool::GetFreeConnNum(){
    std::lock_guard<std::mutex> locker(mutex);
    return conn_queue.size();
}