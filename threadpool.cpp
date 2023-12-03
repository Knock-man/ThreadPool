#include "threadpool.h"
#include <thread>
#include <iostream>

const int TASK_MAX_THRESHHOLD = INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 100;
const int THREAD_MAX_IDLE_TIME = 6;//单位s

////////////////////////////////////线程池方法实现

ThreadPool::ThreadPool()
        :initThreadSize_(0),
        curThreadSize_(0),
        threadSizeThreadHold_(THREAD_MAX_THRESHHOLD),
        idleThreadSize_(0),
        taskSize_(0),
        taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD), 
        poolMode_(PoolMode::MODE_FIXED),
        isPoolRuning_(false)
        
 {

 }
 
ThreadPool::~ThreadPool()
{
    isPoolRuning_ = false;
    //等待线程池里面所有的线程返回 有两种状态：阻塞 & 正在执行任务中
    std::unique_lock<std::mutex> lock(taskQueMtx_);
    notEmpty_.notify_all();
    exitCond_.wait(lock,[&]()->bool{
        return threads_.size() == 0;
    });

}

//设置线程池工作模式
void ThreadPool::setMode(PoolMode mode)
{
    if(checkRuningState()) return;//线程池已启动，不允许设置
    poolMode_ = mode;
}

//设置线程池cached模式下线程阈值
void ThreadPool::setThreadSizeThreshHold(size_t threadhold)
{
    if(checkRuningState()) return;//线程池已启动，不允许设置
    if(poolMode_ == PoolMode::MODE_CACHED)
    {
        threadSizeThreadHold_ = threadhold;
    }
    
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

    //cached模式，任务处理比较紧急 场景：小而快的任务 需要根据任务数量和空闲线程的数量，判断是否需要创建新的线程出来
    if(poolMode_ == PoolMode::MODE_CACHED && taskSize_ > idleThreadSize_ && curThreadSize_ < (int)threadSizeThreadHold_)
    {
        std::cout<<">>>>>创建了新线程>>>>>>"<<std::endl;

        //创建新线程
        std::unique_ptr<Thread> ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc,this,std::placeholders::_1));
        //threads_.emplace_back(ptr->getId(),std::move(ptr));
        int threadId = ptr->getId();
        threads_.emplace(threadId,std::move(ptr));
        //启动线程
        threads_[threadId]->start();
        //相爱线程个数相关的变量
        curThreadSize_++;
        idleThreadSize_++;
    }

    //返回任务的Result对象
    return Result(sp);
}

//开启线程池
void ThreadPool::start(size_t initThreadSize)
{
    //设置线程池的运行状态
    isPoolRuning_ = true;
    //初始线程个数
    initThreadSize_ = initThreadSize;
    curThreadSize_ = initThreadSize;

    //创建线程对象
    for(int i=0; i < (int)initThreadSize_ ; i++)
    {
       
        std::unique_ptr<Thread> ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc,this,std::placeholders::_1));
        //std::unique_ptr<Thread> ptr(new Thread(std::bind(&ThreadPool::threadFunc,this)));
        //threads_.emplace_back(std::move(ptr));//unique_ptr不允许左值引用拷贝构造，可右值引用拷贝构造-
        threads_.emplace(ptr->getId(),std::move(ptr));

    }

    //启动所有线程
    for(int i=0; i < (int)initThreadSize_ ; i++)
    { 
        threads_[i]->start();//需要去执行一个线程函数
        idleThreadSize_++;//记录初始空闲线程数量
        
    }
}

bool ThreadPool::checkRuningState() const
{
    return isPoolRuning_;
}
  
//定义线程函数 线程池的所有线程从任务队列里面消费任务
void ThreadPool::threadFunc(int threadid)//线程函数执行完，线程结束
{
    auto lastTime = std::chrono::high_resolution_clock().now();

    while(isPoolRuning_)
    {
        std::shared_ptr<Task> task;
        {//保证取出任务立马释放锁，让别的线程去取任务，而不是等到任务执行结束再释放
        std::cout<<"threadid"<<std::this_thread::get_id()<<"尝试获取任务!"<<std::endl;
            //先获取锁
            std::unique_lock<std::mutex> lock(taskQueMtx_);
            
            //cached模式下，有可能已经创建了很多的线程，但是空闲时间超过60s,应该把多余的线程结束回收掉
            //(超过initThreadSize数量的线程要进行回收)
            //当前时间 - 上一次线程执行的时间 > 60s
                //每一秒中返回一次 怎么区分：超时返回？还是任务执行返回
            while(isPoolRuning_ && taskQue_.size() == 0)
            {
                if(poolMode_ == PoolMode::MODE_CACHED)
                {
                    //条件变量，超时返回了
                    if(std::cv_status::timeout == notEmpty_.wait_for(lock,std::chrono::seconds(1)))
                    {
                        auto now = std::chrono::high_resolution_clock().now();
                        auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                        if(dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > (int)initThreadSize_)//空闲队列超时
                        {
                            //回收当前线程
                            //记录线程数量相关变量的值修改
                            //把线程对象从线程列表容器中删除 threadid => thread对象 => 删除
                            threads_.erase(threadid);
                            curThreadSize_--;
                            idleThreadSize_--;
                            std::cout<<"threadid"<<std::this_thread::get_id()<<"exit!"<<std::endl;
                            return;
                        }
                    }

                }
                else if(poolMode_ == PoolMode::MODE_FIXED)
                {
                    //等待notEmpty条件
                    notEmpty_.wait(lock);
                }
                
                //线程池要结束，回收线程资源
                if(!isPoolRuning_)
                {
                   break;
                }
            }
            
            idleThreadSize_--;

            //从任务队列中取一个任务出来
            task = taskQue_.front();
            taskQue_.pop();
            taskSize_--;
            std::cout<<"threadid"<<std::this_thread::get_id()<<"获取任务成功!"<<std::endl;
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

        idleThreadSize_++;
        lastTime = std::chrono::high_resolution_clock().now();
        
    }

    threads_.erase(threadid);
    std::cout<<"===threadid"<<std::this_thread::get_id()<<"exit===!"<<std::endl;
    exitCond_.notify_all();
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

int Thread::generateId_ = 0;

Thread::Thread(ThreadFunc func)
        :func_(func),
        threadId_(generateId_++)
{
    

}

Thread::~Thread()
{
    
}


//获取线程id
int Thread::getId()const
{
    return threadId_;
}

//启动线程
void Thread::start()
{
    //创建一个线程来执行线程函数
    std::thread t(func_,threadId_);
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