# Assignment

补充：

1. 并不只有原子操作可以引入SW关系，像我们课程中说过，被启动的线程A在起始时与启动线程B有SW(B, A)的关系，在结束时与等待线程（join）B有SW(A, B)的关系。除此之外，像`promise`的`set_value/exception`与`future`的`.wait`之间，都有类似的性质。因为这些关系都比较符合直觉，所以我们略过了，需要时可以查下C++标准（或者C++ Concurrency in Action 2nd. ed.这本书 P170-P172有一个汇总，也可以看看）。只有两个提醒的地方：

   + 对同一个mutex的`lock/unlock`有一个total order；

   + C++20引入的新特性（semaphore / latch / barrier），引入的同步关系都是SHB，不是SW（虽然通常这种区别不影响什么东西）。例如，对semaphore，`release`和读到它的`acquire`之间是SHB关系。

2. 关于标准中memory order相关的叙述，主要可以参考[[intro.multithread\]](https://eel.is/c++draft/intro.multithread)和[[atomics.order\]](https://eel.is/c++draft/atomics.order)两节；cppreference中[Multi-threaded executions and data races](https://en.cppreference.com/w/cpp/language/multithread.html)和[std::memory_order](https://cppreference.com/w/cpp/atomic/memory_order.html)两部分基本与之对应。

## Part 1

以下两道题中，如果assert可能失败，我们也视作程序错误。

1. 以下程序是否有问题？

   > 本题来自于知乎问题https://www.zhihu.com/question/513849422。

   ```c++
   std::atomic<int> a{ 0 };
   int b = 0;
   
   std::jthread t1{ []() {
       b = 1;
       a.store(1, std::memory_order_release);
   } }, t2{ []() { b = 2; } }
      , t3{ []() {
   	while (!a.load(std::memory_order_acquire));
   	assert(b == 1);
   } };
   ```

2. 以下程序是否有问题？

   ```c++
   std::atomic<int> a{ 0 };
   int b = 0;
   
   std::jthread t1{ []() {
       b.store(1, std::memory_order_relaxed);
       a.store(1, std::memory_order_release);
   } }, t2{ []() { b.store(2, std::memory_order_relaxed); } }
      , t3{ []() {
   	while (!a.load(std::memory_order_acquire));
       assert(b.load(std::memory_order_relaxed) == 1);
   } };
   ```

3. Dekker's algorithm是一个让两线程交替进入临界区的无锁算法。其伪代码如下：

   ```pseudocode
   turn = 1;
   function thread_1():
   while (true)
   	while (turn==2);
   	CS1;
   	turn = 2;
   	program1;
   end
   
   function thread_2():
   while (true)
   	while (turn==1);
   	CS1;
   	turn = 1;
   	program2;
   end
   ```

   将上述代码翻译为C++代码。

4. 当两个线程执行的速度不同时，Dekker's algorithm迫使其中一方进行等待。有些情况下，我们允许一个线程接着执行，不必强迫交替性。Peterson’s Algorithm可以解决这个问题：

   ```pseudocode
   c1 = 0, c2 = 0, willWait = 1;
   
   function thread_1():
   while (true)
   	c1 = 1;
   	willWait = 1;
   	while (c2 && (willWait==1));
   	CS1;
   	c1 = 0;
   	program1;
   end
   
   function thread_2():
   while (true)
   	c2 = 1;
   	willWait = 2;
   	while (c1 && (willWait==2));
   	CS1;
   	c2 = 0;
   	program2;
   end
   ```

   将上述代码翻译为C++代码。

5. *（这道题比较困难，可以直接看答案）Double-checked locking是一种有名的降低锁开销的pattern（但对于C++并不常用）。例如对于lazy的单例模式，如果希望它线程安全：

   ```c++
   class Singleton
   {
       Singleton() { /* ... */ }
       
       static Singleton* s_instance_;
       static std::mutex s_mutex_;
       
   public:
       static Singleton& GetInstance();
   };
   
   Singleton* Singleton::s_instance_ = nullptr;
   std::mutex Singleton::s_mutex_{};
   
   Singleton& Singleton::GetInstance()
   {
       std::lock_guard _{ s_mutex_ };
       if (s_instance_ == nullptr) {
           s_instance_ = new Singleton();
       }
       return *s_instance_;
   }
   ```

   然而，这样就要每次`GetInstance`都需要加锁，但是实际上这个锁只有第一次初始化的时候才有用，后面全部都是读，没有必要mutal exclusion。而Double-checked locking则是引入了原子变量来表示instance是否已经创建，如果没有创建，再进行锁，即下面的代码：

   ```c++
   static std::atomic<Singleton*> s_instance_; // Singleton类内
   
   Singleton& Singleton::GetInstance()
   {
       Singleton* p = s_instance_.load();
       if (p == nullptr) { // 1st check
           std::lock_guard<std::mutex> lock(s_mutex);
           p = s_instance_.load();
           if (p == nullptr) { // 2nd (double) check
               p = new Singleton();
               s_instance_.store(p);
           }
       }
       return *p;
   }
   ```

   这样，当初始化成功后，之后都只需要一条atomic load就可以了，不需要再接着锁；反之，则再进行上锁。但是为了防止在初始化成功前，有多个线程同时进入了1st check通过后的区域，还要在锁内部重新检查一次。

   然而，上述原子指令使用的全部是`seq_cst`；思考：三条指令可以何种更宽松的order？

   > 注：之所以说这个pattern在C++不常用，是因为C++有block static和`std::call_once`已经可以接管其使用场景。在Java等语言用的可能更多一些。

### Part 3

1. Progress bar.