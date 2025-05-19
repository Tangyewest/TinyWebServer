#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <assert.h>

class ThreadPool {
    public:
        ThreadPool() = default;// 默认构造函数
        ThreadPool(ThreadPool &&) = default;
        // 尽量用make_shared代替new，如果通过new再传递给shared_ptr，内存是不连续的，会造成内存碎片化
        explicit ThreadPool(int threadCount=10) : pool_(std::make_shared<Pool>()){   //创建了一个指向 Pool 结构体的智能指针，并将其赋值给 pool_ 成员变量。
                                                                                    //make_shared:传递右值，功能是在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
                 assert(threadCount > 0);
                 for(int i = 0; i < threadCount; i++) {
                        std::thread([this]() {
                            std::unique_lock<std::mutex> locker(pool_->mutex_);
                            while(true){
                             if(!pool_->taskQueue_.empty())
                             {
                                auto task =std::move(pool_->taskQueue_.front());
                                pool_->taskQueue_.pop();
                                locker.unlock();// 因为已经把任务取出来了，所以可以提前解锁了
                                task();
                                locker.lock();// 马上又要取任务了，上锁
                             }else if(pool_->isClosed_){
                                break;//如果线程池已关闭，则退出循环；
                             }else{
                                pool_->cond_.wait(locker);//否则，线程通过条件变量`cond_`等待新任务的到来。
                             }         //当任务队列为空时，线程通过cond_.wait(locker) 挂起，并自动释放锁。
                                       //当新任务被添加时（通过 AddTask），调用 cond_.notify_one() 唤醒一个线程，该线程重新获取锁并处理任务。
                            }
                        }).detach(); //线程创建后被分离（`detach()`），意味着主线程不会等待这些工作线程结束。
                 }

        }
        
        ~ThreadPool(){
            if(pool_)
            {
                std::unique_lock<std::mutex> locker(pool_->mutex_);//这个作用域内的代码被互斥锁保
                pool_->isClosed_=true;                                                   // ...操作共享资源...                                                       // 离开作用域时自动释放锁
            }
            pool_->cond_.notify_all();//使它们检查关闭标志并退出循环。
        }

        template<typename T>
        void AddTask(T&& task)
        {
            std::unique_lock<std::mutex> locker(pool_->mutex_);
            pool_->taskQueue_.emplace(std::forward<T>(task));
            pool_->cond_.notify_one();
        }



    private:
        struct Pool{
            std::mutex mutex_;
            std::condition_variable cond_;
            bool isClosed_ = false;
            std::queue<std::function<void()>> taskQueue_;// 任务队列
        };
        std::shared_ptr<Pool> pool_; // 共享指针，线程安全
};




#endif