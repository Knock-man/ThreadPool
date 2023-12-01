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
        std::cout<<"begin threadFunc  tid:"<<std::this_thread::get_id()<<std::endl;
        std::cout<<"end threadFunc   tid:"<<std::this_thread::get_id()<<std::endl;
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        for(int i=a_+1;i<=b_;i++)
        {
            a_+=i; 
        }
        return a_;
    }  
    private:
        int a_;  
        int b_;
};
int main()
{
    ThreadPool pool;
    //开启线程池
    pool.start();

    //将任务提交给线程池
    Result res1 = pool.submitTask(std::make_shared<MyTask>(1,5));
    int sum1 = res1.get().case_<int>();
    Result res2 = pool.submitTask(std::make_shared<MyTask>(6,10));
    int sum2 = res2.get().case_<int>();
    Result res3 = pool.submitTask(std::make_shared<MyTask>(11,15));
    int sum3 = res3.get().case_<int>();

    std::cout<<"sum="<<sum1+sum2+sum3<<std::endl;

    
    getchar();
 
}