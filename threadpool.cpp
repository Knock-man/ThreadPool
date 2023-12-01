#include "threadpool.h"
#include <thread>
#include <iostream>


const int TASK_MAX_THRESHHOLD = 5;


////////////////////////////////////线程池方法实现

ThreadPool::ThreadPool()
        :initThreadSize_(0),
        taskSize_(0),
        taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD), 
        poolMode_(PoolMode::MODE_FIXED)
 {

 }
 
 ThreadPool::~ThreadPool()
{

}

//设置线程池工作模式
void ThreadPool::setMode(PoolMode mode)
{
    poolMode_ = mode;
}

//设置任务队列上限阈值
void ThreadPool::setTaskQueMaxThreadHold(size_t threadhold)
{
    taskQueMaxThreshHold_ = threadhold;
}

//给线程池提交任务 用户调用该接口，传入任务对象
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
    //获取锁
    std::unique_lock<std::mutex> lock(taskQueMtx_);
    //线程通信 等待等待任务队列有空余
    // while(taskQue_.size() == taskQueMaxThreshHold_)
    // {
    //     notFull_.wait(lock);
    // }
    //用户提交任务，最长不能阻塞超过1s，否则判断任务提交失败
    
    bool flag = notFull_.wait_for( lock,
                                std::chrono::seconds(1),
                                [&]()->bool{ return taskQue_.size() < taskQueMaxThreshHold_; } 
                                );
    if(!flag)
    {
        //表示notFull_等待1s钟，条件依然没有满足
        std::cerr<<"task queue is full, submit task fail."<<std::endl;
        return Result(sp,false);
    }

    //如果有空余，把任务队列放入任务队列中 
    taskQue_.emplace(sp);
    taskSize_++;

    //因为新放了任务，任务队列肯定不空了,notEmpty_上进行通知,赶快分配线程执行任务
    notEmpty_.notify_all();

    //返回任务的Result对象
    return Result(sp);
}

//开启线程池
void ThreadPool::start(size_t initThreadSize)
{
    //初始线程个数
    initThreadSize_ = initThreadSize;

    //创建线程对象
    for(int i=0; i < (int)initThreadSize_ ; i++)
    {
       
        std::unique_ptr<Thread> ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc,this));
        //std::unique_ptr<Thread> ptr(new Thread(std::bind(&ThreadPool::threadFunc,this)));
        threads_.emplace_back(std::move(ptr));//unique_ptr不允许左值引用拷贝构造，可右值引用拷贝构造-

    }

    //启动所有线程
    for(int i=0; i < (int)initThreadSize_ ; i++)
    { 
        threads_[i]->start();
    }
}

//定义线程函数 线程池的所有线程从任务队列里面消费任务
void ThreadPool::threadFunc()
{
    // std::cout<<"begin threadFunc  tid:"<<std::this_thread::get_id()<<std::endl;
    // std::cout<<"end threadFunc   tid:"<<std::this_thread::get_id()<<std::endl;
    for(;;)
    {
        std::shared_ptr<Task> task;
        {//保证取出任务立马释放锁，让别的线程去取任务，而不是等到任务执行结束再释放
            //先获取锁
            std::unique_lock<std::mutex> lock(taskQueMtx_);

            //等待notEmpty条件
            notEmpty_.wait(lock,[&]()->bool{
                return taskQue_.size() > 0;
            });

            //从任务队列中取一个任务出来
            task = taskQue_.front();
            taskQue_.pop();
            taskSize_--;
        }

        //如果依然有剩余任务，继续通知其它的线程执行任务
        if(taskQue_.size() > 0)
        {
            notEmpty_.notify_all();
        }
        //取出一个任务，进行通知，可以继续提交生产任务
        notFull_.notify_all();
        
        //当前线程负责执行这个任务
        if(task != nullptr)
        {
            task->exec();
            
        }
        
    }
}

//////////////////////////////Task方法实现
Task::Task()
    :result_(nullptr)
{}


void Task::setResult(Result* res)
{
    result_ = res;
}
void Task::exec()
{
    if(result_ != nullptr)
    {
        result_->setVal(run());//这里发生多态调用
    }
}


////////////////////////////////////线程方法实现
Thread::Thread(ThreadFunc func):func_(func)
{
    

}
Thread::~Thread()
{
    
}


//启动线程
void Thread::start()
{
    //创建一个线程来执行线程函数
    std::thread t(func_);
    t.detach();

}

////////////////////Result方法实现
Result::Result(std::shared_ptr<Task> task, bool isValid)
        :task_(task),
        isValid_(isValid)
{
    task_->setResult(this);
}

Any Result::get()//用户调用的
{
    if(!isValid_)
    {
        return "";
    }
    sem_.wait();//task任务如果没有执行完，这里会阻塞用户的线程
    return std::move(any_);
};

void Result::setVal(Any any)
{
    //存储task的返回值
    this->any_ = std::move(any);
    sem_.post(); //已经获取的任务的返回值，增加信号量资源

}