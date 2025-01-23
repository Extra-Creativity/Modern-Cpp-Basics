# Answer

### Part 1

1. 注意`typename T::size_type Size`是NTTP，尽管带着一个`typename`。

2. ```c++
   template<typename T> class A
   {
       void Test<int, float>(A<int>& a, float);  
   };
   ```

   这里也可以把`int, float`给去掉，可以进行自动推断；但是`<>`不能去掉，否则不会当作模板的实例化，而是会当作另一个普通函数。

3. ```c++
   auto Size(const auto& container) { return container.size(); }
   template<typename T, std::size_t N>
   auto Size(T (&arr)[N])
   {
       return N;
   }
   ```

   注意这个参数是不能换为`T arr[N]`的，因为会decay掉，这个`N`不能参与匹配。

4. 见`Answer-code/FixedString.cpp`。

### Part 2

1. 第一个是普通的pack expansion（因此初始化出一个`initilizer_list`或者`int`，根据`sizeof...(args)`决定），第二个是fold expression。作用都是顺序输出元素。

2. 对于一个节点，返回沿着指定的`path`走到的节点，可以进行如下调用：

   ```c++
   Node n; // 假设进行了一些初始化，构成一个合法的树。
   
   auto left = &Node::left;
   auto right = &Node::right;
   Traverse(&node, left, right, left); // 左-右-左的节点。
   ```

   如果方向全部编译期可确定，可以写成如下形式：

   ```c++
   template<decltype(&Node::left)... paths> // 当然你用auto...也可以
   Node* Traverse(Node* np)
   {
       return (np->* ... ->* paths);
   }
   
   Traverse<left, right, left>(&node);
   ```

3. ```c++
   template<typename... Args>
   class A
   {
       std::tuple<Args...> tuple_;
   
   public:
       A(Args &&...args) : tuple_{ std::forward<Args>(args)... } {}
   };
   ```

4. ```c++
   template<typename... Ts>
   auto ForwardAsTuple(Ts&&... args)
   {
       // 这里的模板参数必须手动加上，不能CTAD，因为CTAD的guide不推导引用。
       return std::tuple<Ts&&...>{ std::forward<Ts>(args)...};
   }
   ```

### Part 3

1. ```c++
   template<typename From, typename To, typename = void>
   struct IsExplicitConvertible : std::false_type
   {
   };
   
   template<typename From, typename To>
   struct IsExplicitConvertible<
       From, To, std::void_t<decltype(static_cast<To>(std::declval<From>()))>>
       : std::true_type
   {
   };
   
   template<typename From, typename To>
   constexpr bool IsExplicitConvertibleV = IsExplicitConvertible<From, To>::value;
   ```

2. ```c++
   template<typename From, typename To>
   struct IsConvertible
   {
   private:
       static void CheckConvert(To);
       static int CheckConvert(...);
   
   public:
       static constexpr bool value =
           std::is_same_v<decltype(CheckConvert(std::declval<From>())), void>;
   };
   
   template<typename From, typename To>
   constexpr bool IsConvertibleV = IsConvertible<From, To>::value;
   ```

   可以测试一下：

   ```c++
   class A
   {
   public:
       operator int() { return 0; } // 加上explicit之后编译不通过。
   };
   
   static_assert(IsConvertibleV<int, float>);
   static_assert(IsConvertibleV<A, int>);
   ```

   特别地，标准里还规定了一些特殊情况，例如`To = void`总是得到`true`，但是这些通过特化做都比较简单，就略过了。

3. ```c++
   template<typename From, typename To>
   struct IsNothrowConvertible
   {
   private:
       static void CheckConvert(To) noexcept;
       static int CheckConvert(...);
   
   public:
       static constexpr bool value = noexcept(CheckConvert(std::declval<From>()));
   };
   
   template<typename From, typename To>
   constexpr bool IsNothrowConvertibleV = IsNothrowConvertible<From, To>::value;
   ```

   当`From`不能转为`To`时，匹配到第二个重载，此时由于函数没有`noexcept`而一定得到`value = false`；而如果能转为`To`，则匹配到第一个，函数本身`noexcept`，所以检查的是`From->To`是否为`noexcept`，满足我们的需求。

4. 续上一部分第三题中`A`

   ```c++
   private:
   template<typename F, std::size_t... Indices>
   decltype(auto) applyImpl(F func, std::index_sequence<Indices...> _)
   {
       return func(std::get<Indices>(this->tuple_)...);
   }
   
   public:
   template<std::invocable<Args...> F>
   decltype(auto) apply(F func)
   {
       return applyImpl(func, std::make_index_sequence<sizeof...(Args)>());
   }
   ```

   几个Note：

   1. 事实上标准库中还有对这种`sizeof...`的新helper，即可以写为：

      ```c++
      return applyImpl(func, std::index_sequence_for<Args...>());
      ```

   2. `make_integer_sequence`实际上是现有模板不能非常好地实现的（需要$O(N)$的实例化深度，编译不友好），都是编译期开洞完成的（即有一个内置函数来$O(1)$完成这个操作）。但是C++26的静态反射（static reflection）可以解决这个问题，见提案P2996 [Reflection for C++26](https://isocpp.org/files/papers/P2996R9.html#implementing-make_integer_sequence)。

   3. `apply`这个函数在标准库里实际上就是C++17的`std::apply(tuple, Method)`，例如：

      ```c++
      std::tuple a{ 1, 2.0 }; // <int, dounle>
      std::apply(a, [](int b, double c) { std::println("{} {}", b, c); });
      ```

### Part 4

1. move_only_function怎么加上const的，可以写一下。
2. 主要是加了成员，还给`Clone`加了参数。

