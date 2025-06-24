# Assignment

1. 实现`std::thread`，不需要实现`get_id`、`native_handle`和`hardware_concurrency`，`join`也不需要实现`resource_deadlock_would_occur`（因为我们不需要实现`id`）。为了方便统一各个系统的代码，我们使用C标准库`<threads.h>`中的API。我们给一个简单的示例：

   ```c
   #include <threads.h>
   #include <stdio.h>
   
   int foo(void* arg) // C标准库要求返回类型是int，代表错误码，我们可以忽略。
   {
       printf("Hello, %d", (int)arg);
       return 0;
   }
   
   int main()
   {
       thrd_t threadHandle;
       int arg = 1;
       int err = thrd_create(&threadHandle, &foo, (void*)arg);
       if (err == thrd_nomem) {
           // 资源不足，无法创建线程
       } else if (err == thrd_error) {
           // 其他错误。
       }
       
       err = thrd_join(threadHandle, NULL); // 不需要foo的错误码。
       // 或err = detach: thrd_detach(threadHandle);
       if (err == thrd_error) {
           // 不知道什么错误。
       }
       
       return 0;
   }
   ```

   > 如果你成功实现了上述版本的`thread`，就可以很容易地改写成platform-dependent API。

   Hint:

   1. 对于`system_error`的异常，可以使用如下的方式来构造：

      ```c++
      #include <system_error>
      
      throw std::system_error {
          std::make_error_code(std::errc::resource_unavailable_try_again)
      };
      ```

   2. 合理使用`std::unique_ptr`和`std::tuple`，达成包装一切参数和decay copy的效果。

2. 当多个`jthread`实际共享一个stop state时，直接使用`std::vector<std::jthread>`会出现两方面的浪费：

   + 每个`std::jthread`的大小会变大，因为要额外存储一个stop source；
   + 考虑到stop source的实现原理，基本等价于一个特殊的`std::shared_ptr`，因此每个stop source都要new一个stop state出来。

   请写一个新的类`ThreadGroup`，它对`std::vector<std::thread>`进行包装，达到与`jthread`相同的RAII效果，并可以在析构之前单独指定哪个线程进行join和detach。

   > C++26随着execution增加了`std::inplace_stop_source`，这个类不会进行额外分配，直接把stop state放在类里面；`std::inplace_stop_token`就是直接存储`stop_source`的指针。与之相对应的，`std::inplace_stop_source`不能进行拷贝和移动，也不能保证use-after-free的问题，是为了降低开销而出现的类。

3. 阅读下面程序，假设除`sleep`外其他的代码的耗时可以忽略，应该得到怎样的输出？

   ```c++
   void task(std::stop_token token, int num)
   {
       auto id = std::this_thread::get_id();
       std::println("call task({})"， num);
       std::stop_callback cb1{ token, [num, id]{
           std::println("- STOP1 requested in task({}, {})", num, 
                        id == std::this_thread::get_id());
       }};
       std::this_thread::sleep_for(9ms);
       std::stop_callback cb2{ token, [num, id]{
           std::println("- STOP2 requested in task({}, {})", num,
                        id == std::this_thread::get_id());
       }};
       std::this_thread::sleep_for(4ms);
   }
   
   int main()
   {
       std::jthread thread{ [](std::stop_token token){
           for (int i = 0; i < 10; i++)
               task(token, i);
       } }:
       std::this_thread:sleep_for(120ms);
   }
   ```

   