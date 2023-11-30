#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

//任务抽象基类
class Task
{
    public:
         //用户可以自定义任意任务类型，从Task继承，重写run方法，实现自定义任务处理
        virtual void run() = 0;
};

//线程池支持的模式
enum class PoolMode
{
    MODE_FIXED, //固定线程数量的线程池
    MODE_CACHED, //线程数量动态增长线程池
};

class Thread
{
public:
    //线程函数对象类型
    using ThreadFunc = std::function<void()>;

    Thread(ThreadFunc func);
    ~Thread();

    //启动线程
    void start();
private:
    ThreadFunc func_;

private:

};


/*

example:
ThreadPool pool;
pool.start(4);
class MyTask : public Task
{
public:
    void run(){ //线程代码.....}
}
pool.submitTask(std::make_shared<MyTask>());

*/
//线程池
class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool();

    //设置线程池工作模式
    void setMode(PoolMode mode);

    //设置任务队列上限阈值
    void setTaskQueMaxThreadHold(size_t threadhold);

    //给线程池提交任务
    void submitTask(std::shared_ptr<Task> sp);

    //开启线程池
    void start(size_t initThreadSize = 4);//初始线程数量

    //禁止拷贝构造和赋值构造
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    //定义线程函数
    void threadFunc();


private:
    std::vector<std::unique_ptr<Thread>> threads_; //线程列表
    std::size_t initThreadSize_; //初始的线程数量

    std::queue<std::shared_ptr<Task>> taskQue_; //任务队列
    std::atomic_uint taskSize_; //任务的数量
    size_t taskQueMaxThreshHold_;   //任务队列数量上限阈值

    //线程通信
    std::mutex taskQueMtx_; //保证任务队列线程安全
    std::condition_variable notFull_; //表示任务队列不满
    std::condition_variable notEmpty_; //表示任务队列不空

    PoolMode poolMode_; //当前线程池的工作模式
    
    
    

};

#endif