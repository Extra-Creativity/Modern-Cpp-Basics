# Assignment

补充：

1. 并不只有原子操作可以引入SW关系，像我们课程中说过，被启动的线程A在起始时与启动线程B有SW(B, A)的关系，在结束时与等待线程（join）B有SW(A, B)的关系。除此之外，像`promise`的`set_value/exception`与`future`的`.wait`之间，都有类似的性质。因为这些关系都比较符合直觉，所以我们略过了，需要时可以查下C++标准（或者C++ Concurrency in Action 2nd. ed.这本书 P170-P172有一个汇总，也可以看看）。只有两个提醒的地方：

   + 对同一个mutex的`lock/unlock`有一个total order；

   + C++20引入的新特性（semaphore / latch / barrier），引入的同步关系都是SHB，不是SW（虽然通常这种区别不影响什么东西）。例如，对semaphore，`release`和读到它的`acquire`之间是SHB关系。

2. 关于标准中memory order相关的叙述，主要可以参考[[intro.multithread\]](https://eel.is/c++draft/intro.multithread)和[[atomics.order\]](https://eel.is/c++draft/atomics.order)两节；cppreference中[Multi-threaded executions and data races](https://en.cppreference.com/w/cpp/language/multithread.html)和[std::memory_order](https://cppreference.com/w/cpp/atomic/memory_order.html)两部分基本与之对应。

3. 存在一些形式化验证memory order正确性的程序，例如[GenMC](https://github.com/MPI-SWS/genmc)。

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

## Part 2

补充一下无锁数据结构相关的知识，因为我们不会在课件中涉及这一部分。更一般地，无阻塞数据结构可以分为两种：

+ Lock-free：当多个线程同时操作同一数据结构时，至少有一个可以在有限步内完成全部操作；最差的情况就是仿佛进行了上锁，只有自己是可以进行数据处理的，其他全部在进行自旋。

  > 特别地，当其他线程全部阻塞，只有当前线程运行时，这个线程必须能在有限步内完成操作，这样才能称为无阻塞的。对于有锁的情况，只有hold lock的线程满足这个属性，其他线程就算被调度上来也不能进行任何操作；我们说的lock-free通常不希望有这种阻塞的情况。

+ Wait-free：就算有多个线程同时操作同一数据结构，每个线程仍然都可以在有限步内完成操全部操作。

显然，实现一个wait-free的数据结构是极度困难的，需要发明全新的算法才能实现。那么是否性能上wait-free > lock-free > lock呢？

其实未必，因为影响性能的因素有很多；

+ 锁可以一次性保护很多的操作，而原子变量每个操作都需要是原子操作，每一步的开销是增大的；
+ CAS的循环对cache可能不友好；
+ 为了达到“有限步”的约束，wait-free要引入大量额外结构；等等。

所以实际的性能还是要进行实测，例如测一测cache利用率、平均等待时间、最长等待时间、数据吞吐量等。

最后，无锁数据结构仍然有可能存在有锁数据结构的类似问题：

+ live lock（活锁）：虽然每个线程都在运行，然而它们并没有实际进行任何进一步的操作。与死锁的区别是，死锁是双方都在阻塞，而活锁是双方都一直在运行，但是实质上在阻塞（因为不能再向下进行了）。我们把死锁的例子改一改就可以得到活锁的例子：

  ```c++
  void func(std::atomic<bool>& a, std::atomic<bool>& b)
  {
      while (a.compare_exchange_weak(false, true)); // 对a进行上锁
      while (b.compare_exchange_weak(false, true)); // 对b进行上锁
  }
  ```

  然后两个线程传入的原子变量顺序相反，则完全有可能双方都执行完第一个上锁，而在循环中等待第二个上锁，这是一种实质上的阻塞。更弱地，虽然可能有些代码不会产生永久性的活锁，但是可能会导致暂时性的阻塞，造成性能的下降。

+ starvation：先到达一个位置的线程长时间不能被调度通过该位置，进行其他线程都正常地顺利执行。

---

1. 对于下述代码：

   ```c++
   void Func(std::mdspan<float, std::dims<2>> arr)
   {
       for (std::size_t i = 0; i < arr.extent(0); i++)
           for (std::size_t j = 0; j < arr.extent(1); j++)
               arr[i, j]++;
   }
   
   Func(std::mdspan{ ptr, dimx, dimy });
   ```

   当两个`Func`在并行执行时仍然会有data races的问题。除了使用锁，是否有一种简单的方式消除这种问题？

2. 写一个基本的无锁数据结构：`lock_free_stack`，提供`push`和`pop`；我们假设存储的数据类型在拷贝时不会抛出异常，并使用单向链表作为底层的数据结构。由于我们还没有讲下一部分，这里先不需要考虑使用何种memory order，全部使用默认的`seq_cst`。

   > Hint：把二者进行拆解，则push的步骤为：
   >
   > + 产生一个新的链表节点；
   > + 把该节点的`next`设置为`head`；
   > + 把当前的节点设置为`head`。
   >
   > 而pop的步骤为：
   >
   > + 读出`head`，以及读出`head->next`；
   > + 把`head->next`设置为`head`；
   > + 返回原`head`的数据；
   > + 删除原`head`。
   >
   > 为了能够一致地完成以上步骤，注意利用`compare_exchange`来完成数据的更新。



## Part 3

1. Progress bar.