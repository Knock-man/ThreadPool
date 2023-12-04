#include "threadpool.h"
#include <iostream>
#include <chrono>
#include <thread>
using namespace std;
//自行创建一个继承类 根据实际需要确定参数和任务函数
class MyTask : public Task
{
public:
    MyTask(int a,int b):a_(a),b_(b){}
    Any run()//在线程池分配的线程中做事情
    {
       
        return a_ + b_;
    }  

private:
        int a_;  
        int b_;
};


int main()
{
    ThreadPool pool;
    //设置线程池工作模式
    pool.setMode(PoolMode::MODE_CACHED);//PoolMode::MODE_CACHED:可变线程  PoolMode::MODE_FIXED:固定线程

    //pool.setTaskQueMaxThreadHold(1024);//设置任务队列上限阈值(可省略)
    //pool.setThreadSizeThreshHold(10);//设置线程上限阈值(可省略)

    //开启线程池
    pool.start(4);//开启线程池，初始线程数量(可设空)

    //将任务提交给线程池
    Result res = pool.submitTask(std::make_shared<MyTask>(100,200));//传入参数即可
    cout<<res.get().case_<int>()<<endl;//获取执行结果，会阻塞等待任务执行结束

}