# Answer

## Part 1

1. 有问题，我们从上至下依次编号#1 ~ #5，只能得到HB(1, 2, 4, 5)，并不能得到和#3的HB关系，于是构成data races，UB。

   > 原问题询问为什么t2没有与t3同步，应该对t3不产生visible effects，但是`assert(b == 1)`却可能失败。原因就是产生了data race，是UB。

2. 有问题，虽然通过原子性可以保证未产生data races，但是由于HB(5, 3)不成立，因此允许`b`读到`2`。

3. ```c++
   std::atomic<int> turn{ 1 };
   
   void thread1()
   {
       while (true)
       {
           while (turn.load(std::memory_order_acquire) == 2); // #1
           CS1; // #2
           turn.store(2, std::memory_order_release); // #3
           program1;
       }
   }
   
   void thread2()
   {
       while (true)
       {
           while (turn.load(std::memory_order_acquire) == 1); // #4
           CS2; // #5
           turn.store(1, std::memory_order_release); // #6
           program2;
       }
   }
   ```

   最开始MO中只存在1，因此一定是`#1_1 -> #2_1 -> #3_1`执行。在这里存下2后，由于SB的关系，`#1_2`不能读到比这个2更老的值，因此只能等待`#6`进行修改；`#6`修改成功后会进行release操作，与`#1_2`的SW保证了`#5_1` happens before `#2_2`，没有data races。依次类推，因此在这个版本的Dekker's algorithm中是可以使用acquire-release model的。

   > 事实上存在多个版本的Dekker's algorithm，并不是每个都可以使用acquire-release model。

4. 与上一题不同，Peterson's algorithm必须要求全局一致的顺序（即一个total order）。

   ```c++
   std::atomic<int> c1{ 0 }, c2{ 0 }, willWait{ 1 };
   
   void thread1()
   {
       while (true)
       {
           c1.store(1);       // #1
           willWait.store(1); // #2
           while (c2.load() != 0            // #3
                  && willWait.load() == 1); // #4
           CS1;               // #5
           c1.store(0);       // #6
           program1;
       }
   }
   
   void thread2()
   {
       while (true)
       {
           c2.store(1);       // #7
           willWait.store(2); // #8
           while (c1.load() != 0            // #9
                  && willWait.load() == 2); // #10
           CS1;               // #11
           c2.store(0);       // #12
           program2;
       }
   }
   ```

   我们假设上面的load都是acquire，store都是release，那么存在这样的可能：

   + `thread1`执行`#1`和`#2`，`thread2`执行`#7, #8`；此时由于不存在任何的load，因此也就没有任何的SW关系；在c1/c2的MO都是`0 1`，而willWait的MO是`1 2`或`2 1`；
   + `thread1`执行`#3`，由于读的是c2，而此时还没有任何SW，因此可以读到`0`这个值，于是可以进入CS1（#5）；
   + `thread2`执行`#9`，由于读的是c1，而此时还没有任何SW，因此可以读到`0`这个值，于是可以进入CS1（#11）；

   因此两个线程可以同时进入临界区。

5. ```c++
   Singleton& Singleton::GetInstance()
   {
       Singleton* p = s_instance_.load(std::memory_order_acquire); // #1
       if (p == nullptr) { // #2
           std::lock_guard<std::mutex> lock(s_mutex); // #3
           p = s_instance_.load(std::memory_order_relaxed); // #4
           if (p == nullptr) { // #5
               p = new Singleton(); // #6
               s_instance_.store(p, std::memory_order_release); // #7
           }
       }
       return *p;
   }
   ```

   对于acquire-release比较好理解，这样就能保证一旦`#1`读到的是`nullptr`以外的值，也即`#7`的值，SW的关系使得HB(#6, #1)，因此使用这个`*p`是安全的，无data races的；而`#4`之所以可以简化为`relaxed`，是因为`mutex`的`unlock`和下一次`lock`存在一个SW的关系，且lock之间存在total order，因此对于第一个进入`#3`者`unlock`后，之后所有进入`#3`者都发生在`#7`后面（即SB(#7, unlock_1), SW(unlock_1, lock_m), SB(lock_m, #4_k)，于是可以推出HB(#7, #4_k)），从而就算是relaxed load也可以正确地读到非`nullptr`的值。

## Part 2

1. 可以通过定义`atomic_ref`：

   ```c++
   std::atomic_ref val{ arr[i, j] };
   val++;
   ```

   除此之外，也可以更一般地定义一个Access Policy，这样就不用总是写这么一个额外的`atomic_ref`了：

   ```c++
   template<class ElementType>
   struct atomic_accessor {
       using offset_policy = std::default_accessor<ElementType>;
       using element_type = atomic_ref<ElementType>;
       using reference = atomic_ref<ElementType>;
       using data_handle_type = ElementType*;
   
       constexpr atomic_accessor() noexcept = default;
   
       constexpr reference access(data_handle_type p, size_t i) const noexcept {
           return std::atomic_ref{ p[i] };
       }
       constexpr typename offset_policy::data_handle_type
           offset(data_handle_type p, size_t i) const noexcept {
           return p + i;
       }
   };
   ```

   这样可以定义`std::mdspan<float, std::dims<2>, std::layout_right, atomic_accessor<float>>`，`operator[]`的访问会自动返回`atomic_ref`。

2. ```c++
   template<typename T>
   class LockFreeStack
   {
       struct Node
       {
           T data;
           Node* next;
       };
       std::atomic<Node*> head_;
   public:
       void Push(const T& data)
       {
           Node* newNode = new Node{ .data = data };
           newNode->next = head_.load();
           while (!head_.compare_exchange_weak(newNode->next, newNode));
       }
       
       std::optional<T> Pop()
       {
           Node* oldHead = head_.load();
           while (oldHead && !head_.compare_exchange_weak(oldHead, oldHead->next));
           std::optional<T> result = oldHead ? std::nullopt : oldHead.data;
           delete oldHead;
           return result;
       }
   };
   ```

   我们来分析一下：

   + 如果有多个线程进行`Push`，则可能其中一个线程的成功`Push`造成head的更新，使得其他线程之前读到的`head_.load()`是不正确的，应该更换为新的head。因此，我们可以通过CAS来及时进行更新：

     + 当`newNode->next`不再是当前的`head_`时，则比较失败，同时`newNode->next`会被赋当前的新head；
     + 当`newNode->next`是当前的`head_`时，则将`head_`更新为当前节点`newNode`，原子地进行数据结构的更新。

     > 如果`new`失败抛出异常，则由于还没有实际进入到数据结构的操作，因此没有什么影响。以及由于只有开头一处可能抛异常，因此没使用`unique_ptr`也可以。

   + 如果有多个线程进行`Pop`或同时进行`Pop`和`Push`时情况类似。

   最后我们强调，这里是假设了数据的拷贝不会抛出异常，否则情况会复杂的多。一种比较通用的解决方式是使用`std::shared_ptr`来避免拷贝，我们在下一章再讨论这个问题。


## Part 3

1. ```c++
   void Push(const T& data)
   {
       Node* newNode = new Node{ .data = data };
       newNode->next = head_.load(std::memory_order_relaxed);
       while (!head_.compare_exchange_weak(newNode->next, newNode,
                                           std::memory_order_release,
                                           std::memory_order_relaxed));
   }
   
   std::optional<T> Pop()
   {
       Node* oldHead = head_.load(std::memory_order_relaxed);
       while (oldHead && !head_.compare_exchange_weak(oldHead, oldHead->next,
                                                      std::memory_order_acquire,
                                                      std::memory_order_relaxed));
       std::optional<T> result = oldHead ? std::nullopt : oldHead.data;
       delete oldHead;
       return result;
   }
   ```

   relaxed load是因为它们不实际参与同步，就算读到比较老的值也没关系，在CAS中还是会变为正确的值；`Push`的CAS在成功时，需要让其之前的语句（即`new Node`）对于`Pop`中的成功CAS后的语句可见，也即需要建立happens-before的关系，因此`Push`需要用release，而`Pop`需要用acquire。

2. 见`Answer-code/ProgressBar.cpp`，我们分析一下这个过程的正确性：

   + `barGuard_`类似于一个锁，保证了只有一个人可以进入实际的更新代码；其他人则直接略过输出，仅仅更新内部进度。

     这里不能用relaxed，否则内部代码之间推导不出HB的关系，产生data races；我们这里使用acquire-release，从而对`barGuard_`的所有更新构成了以某个store开头的release sequence，下次成功的CAS就可以保证happen after上一次成功的更新了。

   + 注意到进度条不能回退，因此需要引入一个`lastUpdatedCnt_`，否则可能出现之前进度的人因为调度等原因，后拿到`barGuard_`，这样出现了倒退的百分比；为了能最终输出100%，最后一个updater需要保证输出。
   + 最后的`wait`可以使用relaxed order，是因为它并不参与同步相关的步骤。然而仍然有可能wait结束之后CAS失败（例如另一个线程抢到了guard），因此需要循环。之所以不直接循环CAS是考虑性能原因。

   当然还有一些比较简单的可能的优化空间，以及可能需要额外的防溢出等，我们就不详述了。

   > 注：你也可以让用户定值`emptyChar/fillChar/barLength`，这样需要把`outputBuffer_`写为`std::string`等。
   >
   > 你也可以假想一下没有`barGuard`可不可以正确实现，把其他很多因素改为local或atomic的行不行；由于并行的`print`输出是无序的，虽然没有data races，但是不可避免地会导致进度条回退。

3. 见我在知乎上的分析文章：[shared_ptr中原子计数器如何选择memory order?](https://zhuanlan.zhihu.com/p/1931663535085183386)。

## Part 4

1. 不正确，因为`generator`进行resume（即iterator++）后，`co_await`会结束，于是`val`引用的对象时效（即悬垂引用），造成UB。
2. 见`Answer-code/TaskCont.cpp`。
