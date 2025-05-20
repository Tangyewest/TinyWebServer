#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <queue>
#include <condition_variable>
#include <mutex>
#include <sys/time.h>
using namespace std;

template<typename T>
class BlockQueue{
      public:
              explicit BlockQueue(size_t maxSize = 1000);
              ~BlockQueue();
              bool empty();
              bool full();
              void push_back(const T& item);
              void push_front(const T& item);
              bool pop(T& item);//弹出的任务放入item
              bool pop(T& item, int timeout);//等待时间
              void clear();
              T front();
              T back();
              size_t capacity();
              size_t size();

              void flush();
              void close();//关闭队列

private:
              deque<T> deq_;//底层数据结构
              mutex mutex_;//互斥锁
              bool isClose_;//是否关闭
              size_t capacity_;//队列最大容量
              condition_variable condConsumer_;//消费者条件变量
              condition_variable condProducer_;//生产者条件变量
};

// 队列的构造函数
template<typename T>
BlockQueue<T>::BlockQueue(size_t maxSize) : capacity_(maxSize){
      assert(maxSize > 0);
      isClose_ = false;
}

template<typename T>
BlockQueue<T>::~BlockQueue(){
      close();
}

//判断队列是否为空
template<typename T>
bool BlockQueue<T>::empty(){
      lock_guard<mutex> locker(mutex_);//使用lock_guard实现了raii方式的加锁，线程安全
      return deq_.empty();//返回底层双端队列是否为空
}

//判断队列是否满了
template<typename T>
bool BlockQueue<T>::full(){
      lock_guard<mutex> locker(mutex_);//使用lock_guard实现了raii方式的加锁，线程安全
      return deq_.size() >= capacity_;//返回底层双端队列是否满了
}

//关闭队列
template<typename T>
void BlockQueue<T>::close(){
      // lock_guard<mutex> locker(mtx_); // 操控队列之前，都需要上锁
      // deq_.clear();                   // 清空队列
      clear();
      isClose_ = true;               // 设置关闭标志
      condConsumer_.notify_all();    // 唤醒所有消费者
      condProducer_.notify_all();    // 唤醒所有生产者
}

template<typename T>
void BlockQueue<T>::clear(){
      lock_guard<mutex> locker(mutex_);
      deq_.clear();
}

template<typename T>
void BlockQueue<T>::push_back(const T& item){
      // 注意，条件变量需要搭配unique_lock使用
      unique_lock<mutex> locker(mutex_);
      while(deq_.size() >= capacity_){ // 如果队列满了，生产者就等待
            condProducer_.wait(locker);//暂停生产，等待消费者唤醒生产条件变量
      }
      deq_.push_back(item);//将item放入队列
      condConsumer_.notify_one();//唤醒一个消费者
}

template<typename T>
void BlockQueue<T>::push_front(const T& item){
      unique_lock<mutex> locker(mutex_);
      while(deq_.size() >= capacity_){ // 如果队列满了，生产者就等待
            condProducer_.wait(locker);//暂停生产，等待消费者唤醒生产条件变量
      }
      deq_.push_front(item);
      condConsumer_.notify_one();//唤醒一个消费者
}


template<typename T>
bool BlockQueue<T>::pop(T& item){
      unique_lock<mutex> locker(mutex_);
      while(deq_.empty() && !isClose_){ // 如果队列空了，消费者就等待
            condConsumer_.wait(locker);//暂停消费，等待生产者唤醒消费条件变量
      }
      if(isClose_){
            return false;
      }
      item = deq_.front();//获取队首元素
      deq_.pop_front();//移除队首元素
      condProducer_.notify_one();//唤醒一个生产者
      return true;
}

template<typename T>
bool BlockQueue<T>::pop(T& item, int timeout){
      unique_lock<mutex> locker(mutex_);
      while(deq_.empty() && !isClose_){ // 如果队列空了，消费者就等待
            if(condConsumer_.wait_for(locker, std::chrono::milliseconds(timeout)) == std::cv_status::timeout){
                  return false;
            }
      }
      if(isClose_){
            return false;
      }
      item = deq_.front();//获取队首元素
      deq_.pop_front();//移除队首元素
      condProducer_.notify_one();//唤醒一个生产者
      return true;
}

template<typename T>
T BlockQueue<T>::front(){
      lock_guard<mutex> locker(mutex_);
      return deq_.front();
}

template<typename T>
T BlockQueue<T>::back(){
      lock_guard<mutex> locker(mutex_);
      return deq_.back();
}

template<typename T>
size_t BlockQueue<T>::capacity(){
      return capacity_;
}

template<typename T>
size_t BlockQueue<T>::size(){
      lock_guard<mutex> locker(mutex_);
      return deq_.size();
}

//唤醒消费者
template<typename T>
void BlockQueue<T>::flush(){
      condConsumer_.notify_one();
}
#endif