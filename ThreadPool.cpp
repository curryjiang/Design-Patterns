#include <vector>
#include <queue>
#include <atomic>
#include <future>
#include <condition_variable>
#include<functional>
#include<mutex>
#include<atomic>
#include<memory>
#include<iostream>
using namespace std;

class ThreadPool{
public:
   static ThreadPool& getInstance(size_t numThreads){
       static ThreadPool instance(numThreads);
       return instance;
   }
   
   //禁止拷贝和移动
   ThreadPool(const ThreadPool& obj) = delete;
   ThreadPool& operator=(const ThreadPool& obj) = delete;
   ThreadPool(ThreadPool&& obj) = delete;
   ThreadPool& operator=(const ThreadPool&& obj) = delete;
   
   //插入任务到任务队列中
   template<typename F, typename...Args>
   void enqueue(F&& f, Args&&... args){
       {
        std::unique_lock<std::mutex>lock(queueMutex);
        if(stop){
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks.emplace([=]{f(args...);});
       }
       condition.notify_one();
   }
   
   void shutdown(){
    {
        std::unique_lock<std::mutex>lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for(auto& worker : workers){
       if(worker.joinable()){
         worker.join();
       }
    }
   }

   size_t size() const{
    return workers.size();
   }
 
private:
  ThreadPool(size_t numThreads):stop(false){
       for(size_t i = 0; i < numThreads; ++i){
        workers.emplace_back([this]{
           while(true){
            std::function<void()>task;
            {
                std::unique_lock<std::mutex>lock(this->queueMutex);
                this->condition.wait(lock,[this]{return this->stop || !this->tasks.empty();});
                if(this->stop && this->tasks.empty()){
                    return;
                }
                task = std::move(this->tasks.front());
                this->tasks.pop();
            }
            task();
           }
        });
       }
  }

  ~ThreadPool(){
    shutdown();
  }

  vector<thread>workers;
  queue<std::function<void()>>tasks;
  std::mutex queueMutex;
  std::condition_variable condition;
  std::atomic<bool>stop;

};

void work(int i, string s){
    std::cout<<"Task"<<i<<"is running on thread"<<std::this_thread::get_id()<<std::endl;
    std::cout<<s<<std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}


  int main(){
    ThreadPool& pool = ThreadPool::getInstance(4);
    for(int i = 0; i < 10; ++i){
        pool.enqueue(work,i,"hello");
        // pool.enqueue([i]{
        //     std::cout<<"Task"<<i<<"is running on thread"<<std::this_thread::get_id()<<std::endl;
        //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // });
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}

