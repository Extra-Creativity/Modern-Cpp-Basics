# Answer

### 前半部分

2. 有，在`SomeFunc`调用时构造了一个临时变量，传递给参数`vec`。**在函数调用结束后，临时变量析构，因此`a`变为悬垂引用**。

   > 你可能会问，`const&`不是会延长生命周期吗？一定要注意，只有返回临时变量才会延长，返回引用并不会。

   `functional`的代码类似，把成员的`&`去掉就行。

   如果你觉得还没明白，可以看看下面的一大堆例子：

   ```c++
   #include <vector>
   
   // UB, vec是临时变量，它的data()在离开作用域后直接悬垂
   const int *Func0(std::vector<int> vec) { return vec.data(); }
   // UB, vec是临时变量，悬垂引用
   const std::vector<int> &Func1(std::vector<int> vec) { return vec; }
   
   // 有可能UB，如果vec是临时变量，那么本语句过后它就会析构，悬垂。
   const int *Func2(const std::vector<int> &vec) { return vec.data(); }
   // 和上一个同理，悬垂指针变悬垂引用。
   const std::vector<int> &Func3(const std::vector<int> &vec) { return vec; }
   
   std::vector<int> Func4(const std::vector<int>& vec) { return vec; }
   
   int main()
   {
       // 后面我们说UB还是OK都是指继续使用结果进行其他操作，比如访问元素。
       std::vector<int> a;
       auto b1 = Func0(a); // ub
       auto c1 = Func1(a); // ub
       auto d1 = Func2(a); // ok
       auto e1 = Func3(a); // ok
   
       auto b2 = Func0({1, 2, 3}); // ub
       auto c2 = Func1({1, 2, 3}); // ok, because auto deduces std::vector
       // which copies and makes a new variable.
       auto d2 = Func2({1, 2, 3}); // ub
       auto e2 = Func3({1, 2, 3}); // ok
   
       const auto& c3 = Func1({1, 2, 3}); // ub, const auto& refer to 
       // the destructed local vec.
       const auto& c4 = Func1(a); // ub, same as before.
       const auto& e3 = Func3(a); // ok
       const auto& e4 = Func3({1, 2, 3}); // ub, const auto& refer to the 
       // destructed temporary passed to the function.
   
       auto f1 = Func4(a); // ok, copy to f1
       auto f2 = Func4({1, 2, 3}); // ok, copy to f1
       const auto& f3 = Func4(a); // ok, const auto& will extend lifetime
       // for returned value type (i.e. not reference)
       const auto& f4 = Func4({1, 2, 3}); // ok, same as above.
   
       return 0;
   }
   ```

3. ```c++
   #include <cstddef>
   #include <print>
   
   template<typename T, std::size_t N>
   class InplaceVector
   {
   public:
       InplaceVector() = default;
   
       T* Data() { return reinterpret_cast<T*>(buffer_); }
       const T* Data() const { return reinterpret_cast<const T*>(buffer_); }
   
       std::size_t Size() const { return size_; }
       void PushBack(const T& elem)
       {
           auto newPos = Data() + size_;
           new(newPos) T{ elem };
           size_++;
       }
   
       void PopBack()
       {
           auto currPos = Data() + (size_ - 1);
           currPos->~T();
           size_--;
       }
   
       T& operator[](std::size_t idx) { return *(Data() + idx); }
       const T& operator[](std::size_t idx) const { return *(Data() + idx); }
   
       ~InplaceVector()
       {
           for (auto ptr = Data(), end = ptr + size_; ptr < end; ptr++)
           {
               ptr->~T();
           }
       }
   private:
       alignas(T) std::byte buffer_[sizeof(T) * N];
       std::size_t size_ = 0;
   };
   
   int main()
   {
       InplaceVector<int, 5> a;
       a.PushBack(1);
       a.PushBack(2);
       a.PushBack(3);
       a.PopBack();
       a.PushBack(4);
       for (std::size_t i = 0; i < a.Size(); i++)
           std::println("{}", a[i]);
       return 0;
   }
   ```

   你可以试验一个在析构函数中进行打印的类作为模板参数，检查是否所有的元素都合适地析构了。

   > 从理论上说，原来的`buffer`是一个旧指针，在placement new之后要想获得正确的`T*`头元素指针需要如下代码：
   >
   > ```c++
   > T* Data() { return std::launder(reinterpret_cast<T*>(buffer)); }
   > ```
   >
   > 或者你额外存储一个当前的尾指针，这样用尾指针向前访问；**但是实际上来说编译器一般不会在这里做优化，因为一般只分析到alias的地步，所以不加`std::launder`也没事，也不用多存那么一个指针。**
   >
   > 如果你实在实在讨厌可能的UB，你可以考虑`reinterpret_cast`头指针但是不使用它进行访问，这样也能不用`std::launder`就规避上述问题：
   >
   > ```c++
   > template<typename T, std::size_t N>
   > class InplaceVector
   > {
   > public:
   >     InplaceVector() = default;
   > 
   >     T *Data() { return endPtr_ - Size(); }
   >     const T *Data() const { return endPtr_ - Size(); }
   > 
   >     std::size_t Size() const
   >     {
   >         return endPtr_ - reinterpret_cast<T *>(buffer_);
   >     }
   > 
   >     void PushBack(const T &elem)
   >     {
   >         new (endPtr_) T{ elem };
   >         endPtr_++;
   >     }
   > 
   >     void PopBack()
   >     {
   >         --endPtr_;
   >         endPtr_->~T();
   >     }
   > 
   >     T &operator[](std::size_t idx) { return *(Data() + idx); }
   >     const T &operator[](std::size_t idx) const { return *(Data() + idx); }
   > 
   >     ~InplaceVector()
   >     {
   >         for (auto ptr = Data(); ptr < endPtr_; ptr++)
   >         {
   >             ptr->~T();
   >         }
   >     }
   > 
   > private:
   >     alignas(T) std::byte buffer_[sizeof(T) * N];
   >     T *endPtr_;
   > };
   > ```
   
4. `a1`的成员重用合法，其他不合法。

5. 合法，因为创建`std::byte` array隐式地创建了对象，我们也在网络中写入了这个buffer使得object进行了suitable created并开启了生命周期，直接`reinterpret_cast`就可以正常访问。

### 后半部分

1. + `test`和`test4`必然为true，**这是因为中间发生了隐式的类型转换**。本质上这个比较等价于`aPtr == (A*)&c`和`bPtr == (B*)&c`，这显然和前面定义变量时的转型是一致的。

   + `test2`和`test5`也必然为true，这是因为中间发生了显式的类型转换，把指针转回原来的位置。

   + `test3`和`test6`的结果不确定，这是因为对`reinterpret_cast`，它等价于`static_cast<T*>(static_cast<void*>())`。我们首先考察`C`和`A`是否满足pointer-interconvertible的条件。

     + 显然`C`和`A`并不相等，也不满足一方是union，一方是其成员的条件。

     + 对于`C`来说，`A`是它的基类，但是`C`并不是standard layout的（因为继承链里有多个类含有数据成员），因此也不满足。

       > 从C++20开始，这个可以用`std::is_pointer_interconvertible_base_of_v<Base, Derived>`来检查。

     因此，转成`reinterpret_cast<C*>`的结果是地址不变。而`aPtr`和`&c`的地址值未必相同，因此未必返回`true`。

   如果把`A`和`B`的成员都去掉，那么`C`就满足standard layout，因此得到的结果就是`&c`，此时确定地得到true。

   > 从实际情况理解一下，以GCC和Clang使用的Itanium ABI（在这里决定了layout的排布）为例，layout的排布为`A -> B -> ddd`。因此，在`bPtr = &c`的过程中，指针的指向不是`c`的首地址，而是`B`开始的位置，因此，它们的地址值是不同的。此时`reinterpret_cast`转回去，得到了`false`。
   >
   > 由于C++对继承的排布顺序没有规定，因此`A`的首地址也未必是`C`的首地址相同（事实上通过合适地增加虚函数，很可能使`A`重排布到后面）。因此`test3`和`test6`结果不确定。
   >
   > 而在`A`和`B`变为空类后，会出现空基类优化（Empty Base Optimization, EBO），这是C++所规定的，即继承链中的空类不占据空间，此时自然地址值必然相同。我们在内存管理的部分还会额外地提及这一部分。

2. ```c++
   class A
   {
   public:
       A(int init_a) : a{ init_a } {}
       int GetBaseVal() const { return a; }
       virtual ~A() = default;
       // 编写一个Clone方法
       virtual A &Clone(const A &another) { return A::operator=(another); }
   
   protected:
       A &operator=(const A &) = default;
   
   private:
       int a;
   };
   
   class B : public A
   {
   public:
       B(int init_a, int init_b) : A{ init_a }, b{ init_b } {}
       int GetDerivedVal() const { return b; }
       // 这里发生了返回类型的协变，我们在第二章复习课说过，仍然构成重载。
       B &Clone(const A &another) override
       {
           const auto &bRef = dynamic_cast<const B &>(another);
           return B::operator=(bRef);
       }
   
   private:
       int b;
   };
   ```

4. 特别注意，`dynamic_cast`在不涉及多态的类中使用会编译错误，但是`typeid`并不会。在没有多态时，`typeid`看到的类型就是当前具有的类型；在具有多态后，才可以看到底层实际具有的类型。

5. 如下：

   ```c++
   #include <print>
   #include <string>
   #include <variant>
   #include <vector>
   
   using VarType = std::variant<int, float, std::string>;
   
   std::string Process(const std::string &value) { return value; }
   std::string Process(int value) { return std::to_string(value); }
   std::string Process(float value) { return std::to_string(value); }
   
   int main()
   {
       std::vector<VarType> v{ 1, "test", 2.0f };
       std::string result{};
       for (auto &currVar : v)
       {
           result += std::visit([](const auto &value) { return Process(value); },
                                currVar);
       }
       std::println("{}", result);
       return 0;
   }
   ```

   当然，你也可以不写`Process`，利用我们提了一句的`if constexpr`；但是注意到我们这里使用了`const auto&`，而我们实际上需要内部那个"auto"，应该怎么办呢？聪明的你应该想到了decay！于是可以这么写：

   ```c++
   result += std::visit(
                   [](const auto &value) {
                       using InnerType = std::decay_t<decltype(value)>;
                       if constexpr (std::is_same_v<InnerType, std::string>)
                           return value;
                       else
                           return std::to_string(value);
                   }, currVar);
   ```

   > 当然，你也可以`std::is_same_v<const std::string&, decltype(value)>`。

   最后事实上上述过程可以用我们之前讲的`transform_reduce`解决，如下：

   ```c++
   auto result = std::transform_reduce(
       v.begin(), v.end(), std::string{}, std::plus<>{},
       [](const auto &currVar) {
           return std::visit(
               [](const auto &value) { return Process(value); }, currVar);
       });
   ```

6. ```c++
   class AnimalInterface
   {
   public:
       // 在学完移动语义后，这里可以进行小的优化。
       template<typename T>
       AnimalInterface(const T &object) : hiddenObject_{ object }
       {
           walkProxy_ = [](std::any &object) {
               std::any_cast<T &>(object).Walk();
           };
           talkProxy_ = [](std::any &object, int times) {
               return std::any_cast<T &>(object).Talk(times);
           };
       }
   
       void Walk() { walkProxy_(hiddenObject_); }
       std::string Talk(int times) { return talkProxy_(hiddenObject_, times); }
   
   private:
       std::any hiddenObject_;
       void (*walkProxy_)(std::any &);
       std::string (*talkProxy_)(std::any &, int);
   };
   ```

   这是C++标准库中的惯用策略，用以实现恢复原来类型部分功能。微软的[proxy库](https://github.com/microsoft/proxy)的基本原理也就是这个，还提了一个proposal [P3086](https://wg21.link/p3086)希望进入标准。当然这个库写的相当复杂，利用了很多模板上的知识，也有更多的功能，即使知道了原理，大家现在也肯定读不懂。

   其实可以注意到我们当前的实现有一个小问题，即每个`AnimalInterface`实例都单独存储了一堆函数的代理，这样如果函数相当多，那么整个对象会非常大。实际的实现里，我们可以把它们扔到一个全局的表里，然后`AnimalInterface`只存储一个指向表的指针。你会发现，这其实就和虚函数表别无二致了，我们用模板直接搞出了编译器才能生成的结构。

   此外，`std::any`是只能存储可拷贝对象的，因为`std::any`本身是可拷贝的；因此实际的库实现类似于自己写了一个容许更多可能的`std::any`。

   > 有人认为这种实现比虚函数调用起来更快，但是我个人感觉这种比较意义不大，因为首先虚函数的调用开销在大型项目里基本可以忽略不计，不如优化其他的部分；其次复杂的模板会让生成的二进制文件巨大无比；最后有可能编译器的优化程度有限，不能让调用的函数进行inline，在一些小代码上可能有明显性能损失。不过作为一种非侵入式设计，这种方式还是有便利性的。我期待有大型项目为它的性能和实用性背书。
   >
   > Reference: [Optimizing Away Virtual Functions May Be Pointless.【CppCon 2023 -哔哩哔哩】](https://b23.tv/KnhNP8e).