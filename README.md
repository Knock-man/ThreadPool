# ThreadPool
## 支持fixed和cached模式下支持返回值的纯手写线程池
平台工具：vscode远程连接Linux服务器
项目描述：
* 1、支持固定线程和可变线程两种模式
* 2、多态实现定制方法提交任务
* 3、手动实现信号量
* 4、支持设置任务提交上限，维护内存空间
* 5、cached模式新创建线程空闲时间过长自动销毁
* 6、手动实现Reuslt缓存保存线程执行结果
***

### 快速运行：
  * 方式一：
    * 直接使用头文件和源文件->[src文件]
      * 1.编写MyTask派生类(确定参数和任务)
      * 2.ThreadPool pool;  
       pool.setMode(PoolMode::MODE_CACHED);  
       pool.start(4);  
       Result res = pool.submitTask(std::make_shared<MyTask>(参数));  
       cout<<res.get().case_<int>()<<endl;  
    
  * 方式二：
    * 使用链接->[link文件]
        * 1.将threadpool.h 文件复制到 /usr/local/include 目录中
          将libthpool.so 文件复制到 /usr/local/lib 目录中
        * 2.包含头文件 #include<threadpool.h>    
        * 3.ThreadPool pool;  
          pool.setMode(PoolMode::MODE_CACHED);  
          pool.start(4);  
          Result res = pool.submitTask(std::make_shared<MyTask>(参数));  
          cout<<res.get().case_<int>()<<endl;  
        * 4、编译 ```g++ main.cpp pthreadpool.h -lpthread```
        * 5、运行``` ./a.out```

        
        
