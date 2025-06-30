# Assignment

## Thread

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

## Synchronization

0. 下面的代码是否是正确的？

   ```c++
   void transfer(Box& from, Box& to, int num)
   {
       std::unique_lock lock1{from.m, std::defer_lock};
       std::unique_lock lock2{to.m, std::defer_lock};
       std::lock(*lock1.mutex(), *lock2.mutex());
   
       from.num_things -= num;
       to.num_things += num;
   }
   ```

接下来我们非常简单地写两个并发容器；在此之前，我们先简述一下并发容器的设计原则（摘自C++ Concurrency in Action 2nd ed.）。对于所有的容器来说，最简单的方式就是直接配一个锁，做什么操作都锁一下，但是这显然只是保证安全性，本质上所有线程还是在串行执行。根据不同数据结构的性质，我们可以尽可能最大化并发操作的数量，于是要考虑以下基本问题：

+ 是否锁保护了不必要保护的操作？
+ 数据结构的不同部分能否使用不同的锁保护？
+ 是否所有操作都需要相同粒度的锁保护？
+ 在不改变语义的情况下，是否存在简单的改动来改进并行性？

当然我们后面的题目有比较明显的实现方法提示，所以不用太担心这些问题。

1. 写一个定长队列类`ThreadSafeQueue`（元素为int，使用`std::vector<int>`存储即可），提供`TryPop() -> std::optional<int>`和`TryPush(int) -> bool`、`GetSnapshot() -> std::vector<int>`三个接口。请仅使用`std::mutex`进行同步。

   > 我们在`Answer-code/ThreadSafeQueue1.test.cpp`中给了一个简单的测试代码，你也可以自己测试一下正确性。不过我们的测试并不是很全面，实际上有很多可以改进的地方。后面的实现你也可以把其中的`TryPop`改成`Pop`等进行类似的测试。

2. 通常情况下，元素入队和出队的速度是不同的，如果只使用`Try`类型的接口，则只能写下面的代码进行忙等待：

   ```c++
   while (!queue.TryPush(elem));
   ```

   当然，也可以在循环里使用`sleep`，但是这样就会面临sleep多长时间合适的问题。因此，一种更好的方式是在元素为空和为满时使用信号量来进行等待。请使用信号量来增加`Pop`和`Push`两个接口。

   Hint：你可以引入两个counting semaphore `sizeSema`和`spaceSema`，分别表示当前队列的元素数量、以及当前队列的剩余空间数量

实际上以上实现中都不需要考虑入队和出队时元素移动可能抛出的异常（甚至是`mutex.lock()`的异常），你可以思考一下如果有这种可能，你的实现还对不对，应该进行怎样的设计。

3. 实现一个哈希表`ThreadSafeUnorderedMap`，提供`Find(Key)`、`Insert(Key, Value)`、`Erase(Key)`、`Size()`和`GetSnapshot() -> std::unordered_map<Key, Value>`五个接口。为了简化实现：

   + 不需要返回迭代器；`Find`返回`std::optional<Value>`（未查找到返回`nullopt`，否则返回数据指针），`Insert`返回`bool`（如果键已经存在，返回`false`；否则插入并返回`true`），`Erase`返回`bool`（如果键不存在，返回`false`；否则删除并返回`true`）。
   + 哈希表不需要支持扩容，也就不需要rehash；只需要在构造函数指定bucket的数量。

   希望可以利用查找表的性质，不必任何操作都使用一整个锁锁住容器。

   Hint：可以每个bucket设置一个锁，思考应该使用什么样的mutex。

4. 回到第2题；使用信号量在设计上需要考虑更多情况；在学习了`condition_variable`后，将这两个接口改为使用`condition_variable`进行同步。

