#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "log.h"

class SqlConnPool {
public:
    static SqlConnPool* Instance();

    MYSQL* GetConn();//获取一个连接
    void FreeConn(MYSQL* conn);//释放一个连接
    int GetFreeConnNum(); //获取空闲连接数

    void Init(const char* host, int port,const char* user, const char* passwd,const char* dbName,int connSize); //初始化连接池
    void ClosePool(); //关闭连接池

private:
    SqlConnPool();
    ~SqlConnPool(){ClosePool();};

    int max_CONN_SIZE; //最大连接数
    std::queue<MYSQL*> conn_queue; //连接队列
    std::mutex mtx; //互斥锁
    sem_t sem; //信号量
};

// RAII类，构造函数获取连接，析构函数释放连接
class SqlConnRAII {
public: SqlConnRAII(MYSQL** SQL, SqlConnPool* connpoll){
        assert(connpoll);
        *SQL = connpoll->GetConn();
        sql_ = *SQL;
        connpool_ = connpoll;
        sql_ptr_ = SQL;
    }
    
    ~SqlConnRAII(){
    if(sql_){
        connpool_->FreeConn(sql_);
        sql_ = nullptr;
        // 置空外部指针
        if (sql_ptr_) *sql_ptr_ = nullptr;
    }
}

private:
    MYSQL* sql_;
    SqlConnPool* connpool_;
    MYSQL** sql_ptr_;
};

#endif