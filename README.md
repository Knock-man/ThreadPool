# ThreadPool
## 支持fixed和cached模式下允许返回值线程池
平台工具：vscode远程连接Linux服务器    
项目描述：
* 1、基于可变参模板编程和引用折叠原理，实现线程池submitTask接口，支持任意任务函数和任意参数的传递
* 2、使用future类型定制submitTask提交任务的返回值
* 3、使用map和queue管理线程对象和任务
* 4、基于条件变量condition_variable和互斥锁实现任务提交线程和任务执行线程间的通信机制
* 5、cached模式新创建线程空闲时间过长自动销毁
* 6、支持fixd和cached模式的线程定制

***

### 有两个版本  
   * 分支dev：纯手动实现复杂版
   * 分支main：采用future类型定制简洁版

### 快速运行  
   * 1、引入线程池头文件 #include"threadpool.h"
   * 2、编写任务函数
   * 3、使用线程池
     ``` C++
     ThreadPool pool;  //创建线程池对象  
     pool.setMode(PoolMode::MODE_CACHED); //设置工作模式 固定线程PoolMode::MODE_CACHED  动态线程PoolMode::MODE_FIXED  
     pool.start(4); //启动线程池  
     future<int> res1 = pool.submitTask(run,1,5);//提交任务函数 函数名 参数  
     cout<<res1.get()<<endl; //获取执行结果  
     ```
   * 4、编译 ```g++ main.cpp pthreadpool.h -lpthread```
   * 5、运行``` ./a.out```
    
        
        
