# Errata
1. 例子出现位置在19:30 (example 01)、26:40 (example 02)、40:15 (example 04) 46:50 (example 05)、51:25 (example 06)

2. 16:40口误，不是做引用，而是做“修改”；auto&和const auto&都是指一个东西（对于不存在的序列本质上也可以用const auto&，不过引用的对象是临时对象，因此不能使用auto&）；后面说的被引用也是“通过引用修改”的含义。一个例子就可以说明区别：
    ```c++
    // 这里也可以使用const auto&，但是不能使用auto&，因为返回的是临时值
    for(auto ele1: stdv::iota(1, 10)) {}
    
    const std::vector v{1,2,3};
    // 这里既可以使用auto&，也可以使用const auto&，不过二者都是相同的const int&，因为面对的是非临时的只读对象
    for(auto ele1: v){ }
    ```

3. 38:40-39:40，所有“lazy_split_view对iterator解引用的时候”，都是指“对iterator解引用的view进行迭代的时候”，请见github的example 03

4. 44:05不是对list，是对vector保留cache

5. 事实上`stdr::to`既可以接受一个模板作为模板参数（例如`std::vector`），也可以接受一个类型作为模板参数（例如`std::vector<int>`）。

6. `pop_heap`保证被pop出来的元素在原来的最后一个位置（即`[first, last - 1)`构成新的堆，而`last - 1`是原来的堆顶），并不是invalid。

# Assignment

## Ranges
学完ranges这一部分，你肯定有很多想写的代码。我们也只是抛砖引玉，写一些比较有意思的东西。

1. 对于可以用`stdr::to`构造的容器，用一条语句完成快速排序（即只有一个return）。

   ```c++
   template<typename Cont>
   Cont QuickSort(const Cont& cont)
   {
       // 如果是空，返回空容器；否则...
       return stdr::empty(cont) ? cont :
       // Your code...
   }
   ```

   > Hint：简单回顾一下快速排序的步骤：
   >
   > + 取一个轴点，不妨是第一个元素。注意不要用`operator[]`，因为容器不保证随机访问。不过可以考虑使用`stdr::b...`取得迭代器（差点就告诉你了）。
   > + 不考虑轴点的位置，把所有小于轴点的部分进行QuickSort，大于等于轴点的部分也进行QuickSort，在把三个部分拼在一起即可。
   >   + 对于轴点，可以使用`stdv::single(...)`来获得它的view，再用`stdr::to<Cont>`来转型为容器。
   >   + 把它们组合成一个`std::array<Cont, 3>`，随后有一个什么view可以把多个view拼成一个view来着？

   这种方式并不是很高效，但是有些语言（例如Python）以此宣传自己功能的强大，事实上C++利用ranges也可以做到。

2. 让我们来分割字符串吧！C++中的`std::string`不存在字符串分割函数一直饱受诟病，被Python等语言使用者嘲笑，把ranges甩给他们吧！

   > 我并不是对Python有什么意见，因为我自己也是Python使用者（

   ```c++
   // 函数参数使用std::string_view更好一点，不过我们还没讲到流与字符串，所以就略过了。
   // 可以简单说一下，他和span<const char>十分相似，存储了两个const char指针，代表了一段字符范围。
   std::vector<std::string> SplitString(const std::string& str, 
                                        const std::string& delim)
   {
       return str | // Your code.
   }
   ```

   注意，分割相关的view得到的view并不直接是`std::string`，需要先transform成`std::string`。

   > C++中不存在字符串分割函数的原因是不知道用什么存储它比较合适，比如我们选择了`std::vector<std::string>`事实上导致了大量的动态内存分配。有了ranges就没这个顾虑了，反正是惰性求值，具体存在哪由用户决定就行了，对标准库无负担。

3. 进行简单的加密：对一个小写英文字母的字符串，减去`'a'`，再加上它的下标并mod 26（不考虑溢出问题），再加上`'a'`来获得密文；例如`abc`会变为`ace`。

   ```c++
   for(auto [idx, elem] : /* Your code */ )
       elem = /* Your code */
   ```

4. 利用Generator生成Fibonacci数列，如下：

   ```c++
   // 目前只有gcc14对应的libstdc++支持std::generator；对于msvc用户，可以先用<experimental/generator>中的std::experimental::generator.
   std::generator<int> Fib(int num) // 生成[0, num)的斐波那契数列
   {
       
   }
   
   auto fib5 = Fib(5);
   for(auto num : fib5)
       std::print("{} ", num); // 1, 1, 2, 3, 5
   ```


## Function

1. 猜测一下为什么`std::bind_back/front`的绑定需要`std::ref`才能真正地把引用绑定上去；这和move-only的对象`bind`后不能再调用的原因密切相关。

   > Hint：move-only不能调用，说明在函数调用的过程中发生了拷贝；那么绑定的对象到函数的参数其实是一个...（答案呼之欲出啊）
   >
   > 所以，`std::reference_wrapper`由于可以...，因此这次拷贝相当于把原来的参数绑定在了引用上。

2. 测试一下transparent operator相关的优化：

   ```c++
   class A
   {
   public:
       A(int init_a) : a{ init_a } { std::println("Construction: a = {}", a); }
       // 这里写一下默认的三路比较运算符和==
   
       // 这里写一下与int比较的三路比较运算符和==
   
   private: // 我们用int代表一个昂贵的构造对象。
       int a;
   };
   
   int main()
   {
       /* 以A为key的std::set类型 */ normalSet{ A{ 1 }, A{ 2 }, A{ 3 } };
       /* 同上，但支持transparent operator的类型 */ goodSet{ A{ 1 }, A{ 2 }, A{ 3 } };
   
       std::println("Begin testing...");
       std::println("----For normal set.");
       normalSet.find(1);
   
       std::println("----For good set.");
       goodSet.find(1);
   
       return 0;
   }
   ```

   看看输出，是不是真的少了一次构造。

3. 既然`std::function`相关的类有性能损失，为什么不总是使用模板呢？构想一个简单的任务，它使用模板会非常不方便去存储各种函数体。

   > 提示：想想什么时候必须要统一类型？
