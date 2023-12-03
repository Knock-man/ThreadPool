#include "threadpool.h"
#include <iostream>
#include <chrono>
#include <thread>
class MyTask : public Task
{
public:
    MyTask(int a,int b):a_(a),b_(b){}
    Any run()//在线程池分配的线程中做事情
    {
       // std::cout<<"begin threadFunc  tid:"<<std::this_thread::get_id()<<std::endl;
        
        //std::this_thread::sleep_for(std::chrono::seconds(3));
        for(int i=a_+1;i<=b_;i++)
        {
            a_+=i; 
        }
        //std::cout<<std::this_thread::get_id()<<"执行任务结束"<<std::endl;
        return a_;
    }  
    private:
        int a_;  
        int b_;
};
int main()
{
    ThreadPool pool;
    //设置线程池工作模式
    pool.setMode(PoolMode::MODE_FIXED);
    //开启线程池
    pool.start(1);

    //将任务提交给线程池
    Result res1 = pool.submitTask(std::make_shared<MyTask>(1,1000));
     Result res2 =pool.submitTask(std::make_shared<MyTask>(1000,10000));
    Result res3 = pool.submitTask(std::make_shared<MyTask>(1100000,150000000));

    
   
  
 

    int sum1 = res1.get().case_<int>();//获取结果会等线程执行结束，必须放在最后防止阻塞
    int sum2 = res2.get().case_<int>();
    int sum3 = res3.get().case_<int>();


    std::cout<<"sum="<<sum1+sum2+sum3<<std::endl;

    
    //getchar();
 
}