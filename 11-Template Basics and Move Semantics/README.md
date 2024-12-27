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
       if (dynamic_cast<const Derived*>(obj) == nullptr) {
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

   + 使`vector`和`string`的构造函数及成员函数变为`constexpr`的；
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
       constexpr std::vector<int> v{ 1,2,3 };
       return v.size();
   }
   constexpr int size = Test(); // Error
   ```

   可以这么理解：每遇到一个必须要求编译期求值的位置（例如`constexpr`变量、数组大小等），就进入了新的context，此时内部所有语句都必须满足编译期可求值的要求。

   + 对于`size = Test()`进入的环境，由于`v`析构时会delete，所以满足了前面说的new的放松条件；
   + 对于`size = Test2()`，注意到`Test2`中`v`是一个constexpr变量，因此开启了新的context，需要探查其内部本身是否满足条件。而构造函数内部不会进行delete，因此不再满足放松条件，

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

7. 对`context`外变量的修改：不允许。例如：

   ```c++
   constexpr int Inc(int& n) { return ++n; }
   constexpr int Test() {
       int n = 1;
       constexpr int m = Inc(n); // compile error.
       int l = Inc(n); // ok，因为没有开启一个新的context，修改操作不在context外。
   }
   ```

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



### Part 3

1. concept是否德摩根定律来一道题，就是`!(a&&b)`和`!a || !b`不等价，因为concept subsumption只考虑`&&`和`||`（严格说还有`()`，即`(Concept)`和`Concept`等价），`!`只是产生了一个新的unknown name。这个PPT上也写了。

## Move Semantics

1. 写一个函数`call`，它接受一个可调用对象`func`和一个参数`param`，用一行代码原封不动返回`func`传入`param`的调用结果并返回（返回类型与`call`的返回类型相同），并回答以下问题：
   + 这个调用和直接从caller调用`func`的效果完全一致吗？是否会造成额外开销？（Hint: prvalue）
   + 如果需要对调用结果进行一定的操作再返回，即需要将调用结果存入一个变量，那么`return`应当怎么写？分别讨论C++23之前/之后的情况。