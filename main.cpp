#include "threadpool.h"
#include <iostream>
#include <future>
using namespace std;

//任务函数
int run(int a,int b)//在线程池分配的线程中做事情
    {
        return a + b;
    }  

int main()
{
    ThreadPool pool;
    //设置线程池工作模式
    pool.setMode(PoolMode::MODE_CACHED);//PoolMode::MODE_CACHED:可变线程  PoolMode::MODE_FIXED:固定线程

    //pool.setTaskQueMaxThreadHold(1024);//设置任务队列上限阈值(可省略)
    //pool.setThreadSizeThreshHold(10);//设置线程上限阈值(可省略)


    pool.start(4);//开启线程池，初始线程数量(可设空)

    //将任务提交给线程池
    future<int> res1 = pool.submitTask(run,1,5);//传入任务函数和参数

    cout<<res1.get()<<endl;//获取返回值
 
}