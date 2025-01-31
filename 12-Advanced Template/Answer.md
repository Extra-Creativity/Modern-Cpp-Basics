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

5. 见`Answer-code/Invoke.cpp`。特别地，可以循规蹈矩地按照规定一条一条来实现，但我们这里对两种情况进行了合并（即消除了`reference_wrapper`的`if constexpr`，也不用自己实现判断实例的traits）：

   ```c++
   using ObjectType = std::remove_reference_t<
       std::unwrap_reference_t<std::remove_cvref_t<T>>>;
   ObjectType &realObj = obj;
   static constexpr bool matchCase =
       std::is_same_v<ClassType, ObjectType> ||
       std::is_base_of_v<ClassType, ObjectType>;
   ```

   解释一下：

   + 第一层`std::remove_cvref_t`是标准要求的；
   + 第二层`std::unwrap_reference_t`对`T = reference_wrapper<U>`返回`U&`，其他返回`T&`。这个是C++20的traits。
   + 第三层把这个`&`去掉，得到无reference_wrapper的类型`ObjectType`。

   随后利用`reference_wrapper<T>`能够自动转型为`T&`的特性，下面这一句就统一了两个case：

   ```c++
   ObjectType &realObj = obj;
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

5. 见`Answer-code/PushBackGuard.cpp`，还是很有难度的。特别地，从C++26开始才允许使用如下形式的`static_assert`：

   ```c++
   static_assert(condition, msg); // msg.data()返回const char*，msg.size()返回大小。
   ```

   之前都必须是裸字符串，没法进行复杂操作，所以用户不能知道具体哪个元素出错了。

### Part 4

1. 见`Answer-code/Function.cpp`，思路难度不大，主要是一些细枝末节可能有小问题。

2. 本质上变成了存引用参数的指针而不是存对象本身，当然这就对生命周期有要求。此外由于`function_ref`只存在`operator()`，因此它并没有用虚表，直接把函数指针塞到对象里，这个和我们Lecture 5的作业是一样的。

3. 本质上是特化了`move_only_function<ReturnType(Args...) const>`等情况，于是：

   + 对于非const的特化，能接受任意对象`Obj`（不论其`operator()`是否为`const`）；从而：
     + 如果是`const std::move_only_function`，则只能调用`Obj::operator() const`；
     + 如果是普通的`std::move_only_function`，则可以任意调用`operator()`，不论方法是否后缀`const`。
   + 对于const的特化，只能接受`operator() const`，否则编译报错。调用时就是调用这个`operator()`。

   例如：

   ```c++
   template<typename ReturnType, typename... Args>
   class MoveOnlyFunction<ReturnType(Args...) const>
   {
   public:
       template<typename F>
       requires std::invocable<const std::decay_t<F>, Args...> // 只能const可调用才能构造
       MoveOnlyFunction(F&&) { }
       
       decltype(auto) operator(Args... args)() const
       {
           return GetImplPtr_()->Call(std::forward<Args>(args)...);
       }
   };
   
   template<typename ReturnType, typename... Args>
   class MoveOnlyFunction<ReturnType(Args...)>
   {
   public:
       template<typename F>
       MoveOnlyFunction(F&&) { }
       
       // 要求proxy提供const和non-const两个Call；这里要forward_like是因为我们function里
       // 实现的GetImplPtr_不返回const Proxy*，而是Proxy*。
       decltype(auto) operator(this auto&& self, Args... args)()
       {
           return std::forward_like<decltype(self)>
               (*GetImplPtr_()).Call(std::forward<Args>(args)...);
       }
   };
   ```

   当然实际上除了`const`外，还可以增加noexcept和reference qualifier的限制，写起来这么多特化还是挺麻烦的。

4. 见`Answer-code/Function-SBO.cpp`，实现的思路是MS-STL使用的。特别地：
   + `Move`虽然标注了noexcept，但是并没有检查移动构造函数是否真的`noexcept`，保险起见可以检查一下`nothrow_copy_constructible || nothrow_move_constructible`，然后`move_if_noexcept`来实现`Move`。
   + 你也可以加上一个flag保存是不是trivially copyable，如果是就直接memcpy过去，减少虚函数调用（当然也增加了空间大小）。
