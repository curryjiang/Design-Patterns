#include<iostream>
#include<mutex>
#include<condition_variable>
#include<memory>
#include<vector>
#include<queue>
#include<thread>
using namespace std;

class ProducerConsumer{
public:
  ProducerConsumer(int cnt):m_maxSize(cnt){}
 
  //生产者函数
  void producer(int value){
    {
    unique_lock<mutex>lock(m_mutex);
    m_cv_full.wait(lock,[this](){return m_queue.size() < m_maxSize;});
    m_queue.push(value);
    }
    std::cout<<"生产商品："<<value<<endl;
    m_cv_empty.notify_one();
  }

  //消费者函数
  int consumer(){
    int value;
    {
        unique_lock<mutex>lock(m_mutex);
        m_cv_empty.wait(lock,[this]{return !m_queue.empty();});
        value = m_queue.front();
        m_queue.pop();
    }
    m_cv_full.notify_one();
    std::cout<<"消费者消费商品："<<value<<endl;
    return value;
  }
  
private:
  std::mutex m_mutex;
  std::queue<int>m_queue;
  int m_maxSize;
  condition_variable m_cv_full;
  condition_variable m_cv_empty;
};


int main(){
    ProducerConsumer p(10);

    std::thread t1([&p](){
        for(int i = 0; i < 10; ++i){
            p.producer(i);
            this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    std::thread t2([&p](){
        for(int i = 0; i < 10; ++i){
            p.consumer();
            this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    t1.join();
    t2.join();
    return 0;
}