# Assignment

## Templates

### Further Reading: Core constant expressions

由于constexpr函数中允许非编译期可求值的表达式出现（因为允许运行时求值），因此尽管其条件非常放松，但不代表可出现在其中的表达式都可以编译期求值。

不过，我们没有仔细地探讨哪些表达式是可以编译期求值的，因为大部分都比较符合直觉（对于比较完整的规则，可见[cppreference](https://en.cppreference.com/w/cpp/language/constant_expression)或者[C++标准](https://eel.is/c++draft/expr.const#10)）；我们下面讨论一些比较特殊的情况。

1. 虚函数：从C++20开始，虚函数才被允许是constexpr的，只要可以编译期确定实际类型，就能够正常地调用相应函数。

   ```c++
   class Base
   {
   public:
       // 从C++20开始才允许virtual constexpr。
       virtual constexpr int func() const { return 1; }
   };
   
   class Derived : public Base
   {
   public:
       constexpr int func() const override { return 2; }
   };
   ```

   例如：

   ```c++
   constexpr int Test(const Base& obj)
   {
       return obj.func();
   }
   
   int main()
   {
       constexpr Base obj;
       static_assert(Test(obj) == 1);
   
       constexpr Derived obj2;
       static_assert(Test(obj2) == 2);
       
       return 0;
   }
   ```

2. RTTI：从C++20开始，`dynamic_cast`和`typeid`变为编译期可求值的表达式（和虚函数要求相同，需要编译期知道对应的object是什么类型）；`typeinfo`的`operator==`从C++20开始也变成constexpr函数了。

   ```c++
   constexpr int Test2(const Base& obj)
   {
       if (dynamic_cast<const Derived*>(&obj) == nullptr) {
           return 3;
       }
       return 4;
   }
   
   constexpr int Test3(const Base& obj)
   {
       if (typeid(obj) == typeid(Derived)) {
           return 3;
       }
       return 4;
   }
   ```

3. 堆内存的分配：new expression被视作不可编译期求值的，但C++20进行了一定的放松；简单来说，new出来的变量必须在相同的编译期context进行delete（常称为"transient allocation"）。相应地，C++20也进行了如下改进：

   + 使`vector`和`string`的构造函数及成员函数变为`constexpr`的（C++23又加入了`unique_ptr`）；
   + 允许析构函数是`constexpr`的（之前不能手动标注，只有trivial destructor默认`constexpr`）。
   + 使`<algorithm>`、`<numeric>`和`<utility>`中一大批函数都变为`constexpr`。

   可能比较抽象，我们举一个最简单的例子：

   ```c++
   constexpr int Test() {
       std::vector<int> v{ 1,2,3 };
       return v.size();
   }
   constexpr int size = Test(); // Okay
   
   constexpr int Test2() {
       constexpr std::vector<int> v{ 1,2,3 }; // Error
       return v.size();
   }
   constexpr int size = Test2();
   ```

   可以这么理解：每遇到一个必须要求编译期求值的位置（例如`constexpr`变量、数组大小等），就进入了新的context，此时内部所有语句都必须满足编译期可求值的要求。

   + 对于`size = Test()`进入的环境，由于`v`析构时会delete，所以满足了前面说的new的放松条件；
   + 对于`size = Test2()`，注意到`Test2`中`v`是一个constexpr变量，因此开启了新的context，需要探查其内部本身是否满足条件。而构造函数内部不会进行delete，因此不再满足放松条件。

   再举一个例子：

   ```c++
   template<typename T>
   constexpr auto MyAverage(T& rng) // 先插入一个默认值，再求平均
   {
       using ElemType = std::ranges::range_value_t<T>;
       std::vector<ElemType> v{ std::ranges::begin(rng), std::ranges::end(rng) };
       v.push_back(ElemType{});
       std::ranges::sort(v);
       auto newEnd = std::unique(v.begin(), v.end());
       auto sum = std::accumulate(v.begin(), newEnd, ElemType{});
       return sum / static_cast<double>(v.size());
   }
   ```

   思考，下面的代码是正确的吗？

   ```c++
   template<typename T>
   constexpr auto ToVector(T& rng)
   {
       using ElemType = std::ranges::range_value_t<T>;
       std::vector<ElemType> v{ std::ranges::begin(rng), std::ranges::end(rng) };
       return v;
   }
   
   constexpr std::array<int, 3> arr{ 1, 2, 3 };
   constexpr std::vector<int> v{ ToVector(arr) }; // 正确吗？
   constexpr std::vector<int> v2{0, 8, 15, 132, 4, 77}; // 正确吗？
   
   constexpr int MySize(T& rng) {
       auto v = ToVector(rng);
       return v.size();
   }
   constexpr int v2 = MySize(arr); // 正确吗？
   ```

   思考有没有什么方式绕过去，间接返回一个容器？

   > 关于编译期的allocation，Barry Revzin写了一篇文章非常详尽地描述其困境：[What's so hard about constexpr allocation? | Barry's C++ Blog](https://brevzin.github.io/c++/2024/07/24/constexpr-alloc/)，预计很长时间内这一方面的进展都会比较缓慢。

4. 异常：throw表达式是不可编译期求值的；C++20开始允许进行try-catch，但是仍然不允许执行流遇到异常。从C++26开始又进行了放松，即只要在context内异常被catch，就不会导致编译错误，仿佛正常抛出了异常。

   ```c++
   constexpr std::optional<int> Access(const std::array<int, 3>& arr, int idx) {
       try {
           return arr.at(3);
       } catch(const std::exception& ex) {
           return std::nullopt;
       }
   }
   // C++26前由于遇到throw而直接导致无法编译期求值，C++26之后则正常返回。
   static_assert(Access({ 1,2,3 }, 4) == std::nullopt);
   ```

   思考一下下面的`foo`和`foo2`是否可以编译通过。

   ```c++
   constexpr int just_error() { 
       throw my_exception{};
       return 1;
   }
   
   constexpr void foo() {
       try {
           auto v = just_error();
       } catch (my_exception) { }
   }
   
   constexpr void foo2() {
       try {
           constexpr auto v = just_error();
       } catch (my_exception) { }
   }
   ```

5. UB：例如整数除以0，编译期eval会编译报错。

6. `reinterpret_cast`：不允许。

7. 对context外变量的修改：不允许。例如：

   ```c++
   constexpr int Inc(int& n) { return ++n; }
   constexpr int Test() {
       int n = 1;
       constexpr int m = Inc(n); // compile error.
       int l = Inc(n); // ok，因为没有开启一个新的context，修改操作不在context外。
   }
   ```

除了这些之外，还有一些比较小的改进，比如C++23引入了constexpr bitset，C++26引入了constexpr structured binding以及constexpr placement new（限制分配位置只能是变量地址）。

### Part 1

1. 小明写下如下程序：

   ```c++
   int main()
   {
       static constinit int size = 3;
       int arr[size];
   }
   ```

   他发现在gcc上编译可以通过，于是开始质疑讲的有问题。思考一下为什么能编译通过。

2. 利用特化实现`std::is_same_v`。

3. 用两种方法，编译期完成如下任务：当一个自然数$n$是奇数，返回$3n+1$；否则返回$n/2$。

### Part 2

1. 判断以下代码是否会导致编译错误（`integral`和`same_as`都是`<concepts>`中定义的标准库concept）：

   ```c++
   #include <concepts>
   
   template<typename T>
   requires (!std::integral<T>) && (!std::same_as<T, float>)
   void Func(T x) {}
   
   template<typename T>
   requires !(std::integral<T> || std::same_as<T, float>)
   void Func(T x) {}
   
   int main()
   {
       Func(1.0);
   }
   ```

2. 写一个`Insert`函数，接受参数`x`和`val`，如果`x`可以调用`push_back`则调用并插入`val`，否则调用`insert`。你能想到如何使用constexpr if来完成吗？

3. 写一个函数，要求其参数可以转换为`int`。阅读`std::convertible_to`这个concept的文档，用缩写的方式（即无requires clause）给出答案。

4. 实现`std::move_if_noexcept<T>`，当`<T>`的移动构造不会抛异常时，或者无拷贝构造函数时，进行`std::move`；否则返回`const&`。这个函数通常用来保证异常安全性。

5. 补充一个知识，模板类成员函数可以对其模板参数施加约束，**当约束不满足时不会导致编译错误，只会去掉这个成员函数**。判断以下代码中哪些语句不能够编译通过：

   ```c++
   template<std::integral T>
   class A
   {
   public:
       bool Test()
           requires std::same_as<T, int>
       {
           return true;
       }
   };
   
   int main()
   {
       A<int> a;
       a.Test();
   
       A<long> b;
       b.Test();
   
       A<float> c;
   }
   ```

## Move Semantics

1. 在C++23引入了新函数`forward_like`，它的作用是“像`forward<decltype(x)>(x)`一样`forward(y)`”。例如我们举过的`optional`的`value`的例子：

   ```c++
   decltype(auto) value(this auto&& self) {
       return std::forward<decltype(self)>(self).value_;
   }
   ```

   也可以利用`forward_like`写为：

   ```c++
   decltype(auto) value(this auto&& self) {
       return std::forward_like<decltype(self)>(self.value_);
   }
   ```

   从而当`self`为左值时，会以左值的方式forward `self.value_`（也就是什么都不干）；当`self`为右值时，会以右值的方式forward `self.value_`（也就是进行移动）。

   请实现`forward_like`；特别地，这个函数在模板参数为`const`时，这种`const`也会传递给函数参数。例如上式中，若`decltype(self)`为`const&`，则`self.value_`也会转为`const Value&`。在成员访问中这是多此一举（因为const变量的成员只能是const），但是在其他情况未必。

   > 你可以用`std::is_const_v<std::remove_reference_t<T>>`来判断一个引用是不是`const&`。**特别注意，`std::is_const_v<T>`对`T=const&`推断出false，必须先把引用去掉再判断。**

2. 写一个函数`call`，它接受一个可调用对象`func`和一个参数`param`，用一行代码原封不动返回`func`传入`param`的调用结果并返回（返回类型与`call`的返回类型相同），并回答以下问题：

   + 这个调用和直接从caller调用`func`的效果完全一致吗？是否会造成额外开销？（Hint: prvalue）
   + 如果需要对调用结果进行一定的操作再返回，即需要将调用结果存入一个变量，那么`return`应当怎么写？分别讨论C++23之前/之后的情况。