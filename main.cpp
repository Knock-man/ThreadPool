#include "threadpool.h"
#include <iostream>
#include <chrono>
#include <thread>
class MyTask : public Task
{
public:
    void run()
    {
        std::cout<<"begin threadFunc  tid:"<<std::this_thread::get_id()<<std::endl;
        std::cout<<"end threadFunc   tid:"<<std::this_thread::get_id()<<std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
};
int main()
{
    ThreadPool pool;
    pool.start();
    for(int i=0; i<100;i++)
    {
        pool.submitTask(std::make_shared<MyTask>());
    }
    
    getchar();
 
}