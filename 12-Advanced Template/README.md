# Assignment

虽然我们在这一部分留了大量的作业，但是实际的代码中仍然有很多需要更灵活运用的地方。在学完这节课后，你可以自行阅读或者尝试编写标准库中`std::tuple`、`std::variant`等实现（如果不太明白，可以参考C++ Templates The Complete Guide 2nd ed. 24 ~ 26章）。同时，你也需要能够分析出模板的实例化深度（复杂度），来估计实例化对编译时间的影响。

### Part 1

1. 解释如下模板函数头：

   ```c++
   template<typename T, typename T::size_type Size> void Test();
   ```

1. 对于如下模板函数：

   ```c++
   template<typename T> class A;
   
   template<typename T, typename U> void Test(A<T>& a, U);
   ```

   `T = int, U = float`的实例设置为`A`的friend。

1. 实现`std::size`，它对于传入的数组也可以返回其大小（其他调用`.size()`）。注意利用引用推断不decay的性质。

1. 实现`BasicFixedString`，进行如下改进：

   + 模板增加`typename CharType`，像`std::basic_string`一样可以接受任意字符类型，然后对`char`对应的类型进行alias。

   + 注意到当前是从头到尾复制的，但是实际的字符串都有null termination。将`FixedString`面对`[N+1]`的输入改为存储`[N]`，同时要求仍然可以自动推断出`N`。

     > 同时你可以利用`std::copy`而不是自己写一个循环，前者已经是`constexpr`的了。

   + 写一个`operator+`，产生新的`BasicFixedString`。

   + 补全方法`c_str`、`data`、`size`、`begin`、`end`。

   + 实现一个`view`方法，返回对应的string_view。

   记得给各个函数加上`constexpr`。同时，msvc可能会出现内部编译器错误（Internal Compiler Error, ICE），可以换用gcc和clang。

   > msvc是所有编译器里最容易出现ICE的，这就是闭源的坏处。。

### Part 2

1. 分别解释如下两个语句：

   ```c++
   template<typename... Args>
   void Print(const Args &...args)
   {
       auto result = { (std::cout << args << " ", 0)... };
       ((std::cout << args << " "), ...);
   }
   ```

1. 解释如下代码，并说明应该如何调用：

   ```c++
   struct Node
   {
       int value;
       Node* left = nullptr;
       Node* right = nullptr;
   };
   
   template<typename... TP>
   Node* Traverse(Node* np, TP... paths) {
       return (np ->* ... ->* paths);
   }
   ```

   如果所有的`path`都是编译期可确定的，可以使用NTTP改成什么样子？

1. 我们之前所讲的都是对一个pack的解包，但有些时候我们是无法得到一个pack的（例如，我们需要把它们存为成员），此时可以利用`std::tuple`。写一个模板类，它可以存储任意类型的tuple，构造函数对这个tuple进行构造。

1. 实现`std::forward_as_tuple`，对于传入的参数，构造相应引用形式的`tuple`并返回。

   > 这个函数的一个使用场合是，当有可能有多组`Args`构造不同的元素时，仅凭一个变长模板参数是无法区分哪些属于哪个元素的构造参数，这时候要传递tuple，但还要进行正常的forward。例如对于`std::map::emplace(args...)`，`args`需要构造`std::pair<Key, Value>`，如果Key和Value都需要多个元素构造，则可以利用其构造函数：
   >
   > ```c++
   > template< class... Args1, class... Args2 >
   > pair(std::piecewise_construct_t,
   >      std::tuple<Args1...> first_args,
   >      std::tuple<Args2...> second_args);
   > ```
   >
   > 于是可以：
   >
   > ```c++
   > map.emplace(std::piecewise_construct, 
   >             std::forward_as_tuple(1, 2.0f),
   >             std::forward_as_tuple(1, "123")); // 假设Key和Value分别进行这种构造。
   > ```
   >
   > `try_emplace(Key, Args...)`本质上在插入成功时就等价于
   >
   > ```c++
   > map.emplace(std::piecewise_construct, 
   >             std::forward_as_tuple(Key),
   >             std::forward_as_tuple(Args...)); 
   > ```

### Part 3

前三题请全部使用SFINAE而非concept完成。

1. 实现`is_explicit_convertible<From, To>`，即可以通过`static_cast`来进行`From->To`的转型。

   > 这个是标准库没有的traits，仅作为练习。

1. 实现`is_convertible<From, To>`，即可以通过**隐式转型**来进行`From->To`的转型。

   > Hint: 通过函数参数转型。

1. 实现`is_nothrow_convertible<From, To>`，增加了转型过程为`noexcept`的要求。

   > Hint: `noexcept(Exp)`表示当Exp各个步骤都是noexcept时才为`true`。特别地，对于function call，就代表了函数本身`noexcept`，同时各个参数的传递也是noexcept的。

1. 对于上一部分的第3题，可以发现pack存为`tuple`后就不能再进行unpack了，但有些情况下我们可能需要这种unpack。例如：

   ```c++
   int Func(int a, double b) { return 1; }
   
   A params{ 1, 2.0 }; // 假设我们存为tuple的类为A
   auto result = params.apply(Func); // 等价于Func(1, 2.0);（当然准确说是存成左值再传进去）
   ```

   我们又知道，`std::tuple`是可以用`std::get<Idx>`来得到第Idx个成员的。所以一种直观的做法是，我们可以通过NTTP来实现`apply`：

   ```c++
   template<typename T, std::size_t... Indices>
   decltype(auto) apply(T func)
   {
       return func(std::get<Indices>(this->tuple)...);
   }
   ```

   然而，这需要用户进行如下形式的调用：

   ```c++
   params.apply<0, 1>(Func);
   ```

   但是我们事实上需要的就是`0 ~ sizeof...(args)`的序列，直观上来说模板应该有自动生成的方式。标准库中提供了类`integer_sequence<typename T, T... Ints>`和函数`make_integer_sequence<T, Size>`，后者可以生成前者（`integer_sequence<T, 0, 1, ..., Size - 1>`）。当`T = std::size_t`时，又增加了别名`index_sequence<std::size_t... Ints>`和`make_index_sequence<Size>`。

   我们不妨看一个例子，以非循环的方式输出`std::array<T, N>`：

   ```c++
   template<typename T, std::size_t... Indices>
   void OutputAllImpl(T &arr, std::index_sequence<Indices...> _)
   {
       ((std::cout << arr[Indices] << '\n'), ...);
   }
   
   template<typename T, std::size_t N>
   void OutputAll(const std::array<T, N> &arr)
   {
       OutputAllImpl(arr, std::make_index_sequence<N>());
   }
   
   int main()
   {
       std::array<int, 3> arr{ 1, 2, 3 };
       OutputAll(arr);
   }
   ```

   请利用类似的方法实现`apply`。

### Part 4

1. 阅读`std::function`的文档，实现`std::function`；忽略allocator相关的部分，也先不考虑SBO。
1. 实现`std::function`的SBO。

> 如果你想，也可以实现一下它的deduction guide。
