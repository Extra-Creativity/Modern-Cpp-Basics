# Answer

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