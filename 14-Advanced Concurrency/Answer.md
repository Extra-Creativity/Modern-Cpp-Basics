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

   

