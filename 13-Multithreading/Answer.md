# Answer

## Thread

1. 见`Answer-code/Thread.cpp`。有几点要提醒一下：

   + `optional<T>`的move在对方`has_value`的情况下，不会把对方置为`std::nullopt`，而是对内部的`T`进行move，所以移动构造函数不能用`default`；

   + 为了传递参数为`void*`，我们统一地把所有的参数打包为`tuple` new出来；随后为了进行copy，我们的`tuple`参数类型都要进行decay。当然你也可以利用tuple的deduction guide自动decay：

     ```c++
     decltype(std::tuple{ func, args... })
     ```

   + 主线程里必须成功启动线程后才能`release`智能指针，防止内存泄漏。

   + `std::apply(..., std::move(*ptr))`是因为我们传入的参数是拷贝的副本，因此可以逐参数move给实际调用的函数；`apply`本质上调用`std::get`，后者在遇到右值引用的tuple时会返回move的成员。

   + 测试的`Object`使用的是`cout`，有可能多个`<<`之间穿插执行，输出会乱序；还是比较建议用`std::println`或者`std::osyncstream`。

2. 见`Answer-code/ThreadGroup.cpp`。`join`之类的没有加上shallow const，要想加也可以加上。

3. 对于前9次循环，`task`耗时9 * (9 + 4) = 117ms，中间的所有stop callback全部构造然后析构，即先register然后deregister，所以前9次输出只有：

   ```text
   call task(0)
   call task(1)
   ...
   call task(8)
   ```

   然后第10次时，在`sleep(9ms)`的过程中，main函数sleep结束，jthread析构时`request_stop`，于是第一个callback输出：

   ```text
   - STOP1 requested in task(9, false);
   ```

   然后第二个callback构造时，stop token已经`request`过了，因此在构造函数里立刻执行：

   ```text
   - STOP2 requested in task(9, true);
   ```

   > 在实际执行的时候，由于其他代码实际上还是占用时间的，所以不一定是这个结果。

## Synchronization

0. 不正确，因为`mutex`不是通过`std::unique_lock`的API来锁住的，因此`std::unique_lock`仍然处于A & !B的状态。我们课上的代码是正确的：

   ```c++
   std::lock(lock1, lock2); // 调用了std::unique_lock::lock()等，状态改变正确。
   ```

1. 见`Answer-code/ThreadSafeQueue1.hpp`。

2. 见`Answer-code/ThreadSafeQueue2.hpp`。有几点提醒的：

   + 假设现在队列为空，线程A在等待队列可以`Pop`；此时线程B进行了`Push`，则在`sizeSema_.release()`时，操作系统有可能会直接调度出等待的线程A（当然这取决于操作系统的具体实现）；而此时A成功`acquire`后，又要等待`mutex_`，于是又要切换回B，整个过程要有两次对线程A的唤醒。因此，我们可以先释放锁，再释放信号量；至少对于单生产者单消费者很有道理。例如：

     ```c++
     void Push(int elem)
     {
         spaceSema_.acquire();
         {
             std::lock_guard _{ mutex_ };
             PushUnsafe_(elem);
         }
         sizeSema_.release();
     }
     
     int Pop(int elem)
     {
         sizeSema_.acquire();
         int result;
         {
             std::lock_guard _{ mutex_ };
             result = PopUnsafe_();
         }
         spaceSema_.release();
         return result;
     }
     ```

   + 你也可以把其中信号量换成`binary_semaphore`，例如：

     ```c++
     std::binary_semaphore pushableSema_{ 1 };
     
     int Pop()
     {
         sizeSema_.acquire();
         
         bool isPreviouslyFull = false;
         int elem;
         {
             std::lock_guard _{ mutex_ };
             isPreviouslyFull = IsFullUnsafe_();
             elem = PopUnsafe_();        
         }
     
         if (isPreviouslyFull)
             pushableSema_.release();
         return elem;
     }
     
     void Push(int elem)
     {
         pushableSema_.acquire();
         
         bool isNotFull = false;
         {
             std::lock_guard _{ mutex_ };
             PushUnsafe_(elem);
             isNotFull = !IsFullUnsafe_();        
         }
         
         if (isNotFull)
             pushableSema_.release();
         sizeSema_.release();
     }
     ```

     但是这种实现有一个缺点，就是对于三个`Push`的线程，有两个要等在`pushableSema_`这里；如果锁在唤醒之前被其他人（例如`Pop`线程）抢走，还要接着等锁，也就是要卡两个地方。用`counting_semaphore`在队列没满的时候就只需要等锁就可以了。

   + 如果要考虑异常和一般类型，设计要复杂的多，例如要防止抛出异常时信号量错误计数。我们可以引入一个Guard：

     ```c++
     template<typename T>
     class SemaphoreGuard
     {
         T* sema_;
     public:
         SemaphoreGuard(T& sema) : sema_{ &sema } { sema_->acquire(); }
         void Release() noexcept { sema_ = nullptr; }
         ~SemaphoreGuard()
         {
             if (sema_)
                 sema_->release();
         }
     };
     ```

     和`lock_guard`不同，由于正常的分支是不需要回退信号量的，因此需要手动`Release`。之后我们改一下`Push`：

     ```c++
     void Push(auto&& elem)
     {
         SemaphoreGuard guard{ spaceSema_ };
         {
             std::lock_guard _{ mutex_ };
             PushUnsafe_(std::forward<decltype(elem)>(elem));
         }
         guard.Release();
         sizeSema_.release();
     }
     ```

     如果`lock`锁上时抛出`system_error`，则`guard`正常恢复原样。最后就是要看一下`PushUnsafe_`有没有问题：

     ```c++
     void PushUnsafe_(auto&& elem)
     {
         auto idx = GetTailIndex_();
         vec_[idx] = std::forward<decltype(elem)>(elem);
         size_++;
     }
     ```

     假设`operator=`抛出异常，则`size_++`还没有进行；外面`lock`和`guard`都正常解锁，`spaceSema_`的值还对应于`size_`。对于`Pop`这个问题要更复杂一些，读者不妨自己思考。

3. 见`Answer-code/ThreadSafeUnorderedMap.hpp`，每个bucket用了一个`shared_mutex`，查找的时候用`shared_lock`锁上，更改用`unique_lock`锁上。有人可能是这么实现`Size`：

   ```c++
   // 引入一个整个容器的mutex
   std::mutex globalMutex_;
   void UpdateSize_(std::size_t num)
   {
       std::lock_guard _{ globalMutex_ };
       size_ += num;
   }
   
   auto Insert(const Key& key, const Value& value)
   {
       auto& bucket = GetBucketByKey_(key);
       auto result = bucket.Insert(key, value, compare_);
       if (result)
           UpdateSize_(1);
       return result;
   }
   // Erase类似
   auto Size() const { std::lock_guard _{ globalMutex_ }; return size_; }
   ```

   但是这种实现是有漏洞的。如果两个线程，一个用`Insert`，一个用`Erase`，那么可以出现下面的过程：

   + `Insert`中`bucket.Insert`结束；
   + `Erase`中`bucket.Erase`结束；
   + `Erase`中`UpdateSize_`结束；

   因为用了两个不同的锁进行保护。如果此时另一个线程调用`.Size`，由于`Insert`中`UpdateSize_`还没执行，就会得到`std::size_t(-1)`的诡异结果。对于`Size`和`GetSnapshot`，也可以全部上锁，再插进去，这样得到一个确定版本的`Map`，否则每个bucket的版本是不太一致的，可能会出现这样的情况：

   + A调用了GetSnapshot，现在处在读取bucket 1的时候（bucket 0读完了，bucket 2还没读）；
   + B插入了bucket 0；
   + C插入了bucket 2；

   同时通过某种同步让B插入bucket 0一定发生在C插入bucket 2之前，则从全局的顺序来看只存在以下两个版本：

   + 仅bucket 0被插入；
   + bucket 0和bucket 2都被插入了。

   但是A读出来的却是仅bucket 2被插入，凭空看到了一个新的版本。当然，我们的前提是所有线程看到的必须是全局一致的，但是我们下一节课讲到的memory order会发现并行里这种全局不一致还是允许存在的。A由于在`GetSnapshot`时并与B和C没有确定性的关系，它可以声称自己只看到了C的结果，没有看到B的结果。具体如何实现取决于调用者是否需要保证这种全局一致性。

   还有其他几个小点，和并行没关系：

   + `Hasher`和`EqualCompare`通常可以使用空基类优化，我们后面内存管理部分会讲到；
   + API的参数全部简写为`const&`，实际上为了性能还要加上右值引用的重载；

4. 见`Answer-code/ThreadSafeQueue3.hpp`。看起来，我们使用condition variable规避了semaphore guard，对于异常处理更方便；但是其实这个问题只是隐藏起来了。例如，如果当前队列为空，有两个等待`Pop`的线程；之后有一个线程进行`Push`，则会唤醒其中一个来`Pop`。但如果这个`Pop`抛出了异常，则队列可以重新`Pop`，但是我们并没有`notify`，另一个`Pop`线程还是在等待。所以，如果想规避这个问题，要么写一个`ConditionVariableGuard`，要么使用`notify_all`。

最后，我们现在是对整个队列进行加锁，但是由于队列的操作位置只有头和尾，其实我们可以更细粒度地划分为头锁和尾锁，似乎就可以让`Pop`和`Push`可以同时进行；同时由于两个方法都要操作`head`和`size`，我们不妨改成`head`和`tail`两个index；为了防止`head == tail`时的歧义，可以再多分配一个位置，使得`tail == head - 1`为满。于是大概可以写这样的代码：

```c++
std::size_t GetTail() const { std::unique_lock<std::mutex> lock{ tailMutex_ }; return tail_; }

int Pop()
{
    std::unique_lock<std::mutex> lock{ headMutex_ };
    emptyWait_.wait(lock, [this](){ return head_ != GetTail(); });
    // 现在可以pop了。此时我们没有锁tail，所以如果有其他Push线程已经wait结束，则可以一同执行。
    auto elem = vec_[head_];
    head_++;
    if (head_ == vec_.size())
        head_ = 0;
    fullWait_.notify_one();
    return elem;
}
```

这时候如果你对称地写`Push`：

```c++
std::size_t GetHead() const { std::unique_lock<std::mutex> lock{ headMutex_ }; return head_; }

void Push(int elem)
{
    std::unique_lock<std::mutex> lock{ tailMutex_ };
    fullWait_.wait(lock, [this](){
        auto tailNext = tail_ + 1;
        if (tailNext == vec_.size()) tailNext = 0;
        return GetHead() != tailNext;
    });
    
    // 现在可以push了。
    vec_[tail_] = elem;
    tail_++;
    if (tail_ == vec_.size())
        tail_ = 0;
    
    emptyWait_.notify_one();
}
```

就会发现可能死锁，因为`Pop`先lock head再lock tail，`Push`则反之。因此要改成不对称地写：

```c++
std::unique_lock<std::mutex> headLock{ headMutex_ };
std::unique_lock<std::mutex> tailLock{ tailMutex_, std::defer_lock };
fullWait_.wait(headLock, [this, &tailLock](){
    auto head = head_;
    tailLock.lock();
    auto tailNext = tail_ + 1;
    if (tailNext == vec_.size()) tailNext = 0;
    if (head_ == tailNext)
    {
        tailLock.unlock();
        return false;
    }
    return true;
});
headLock.unlock(); // 我们只操作tail，所以解锁head，但是在lambda里要锁上tail。
```

不过这种细粒度锁可能提升有限，例如在操作`head`时，要求`tail`已经进入了不需要head lock的位置（也就是wait结束），只有当元素的拷贝或移动非常复杂时，可能线程才会调度走当前线程，让`tail`处于这种位置。不过对于一些无长度限制的队列，由于`Push`不需要head lock，这种优化应该就会更明显一些（但是为了防止vector扩容的同步问题，一般都是使用list，头尾完全不干扰）。

> 总之，可以看到并行同步问题非常非常复杂。