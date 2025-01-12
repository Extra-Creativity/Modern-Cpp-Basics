# Answer

## Templates

### Further Reading

3. 前两个不正确，最后一个正确。如果想间接返回容器，需要用`std::array`。或许有人想这么写：

   ```c++
   constexpr auto ToArray(auto& rng)
   {
       auto vec = ToVector(rng);
       
       using ElemType = std::ranges::range_value_t<T>;
       constexpr std::size_t size = vec.size();
       std::array<ElemType, size> arr;
       std::ranges::copy(vec, arr.begin());
       return arr;
   }
   ```

   然而`constexpr std::size_t size = vec.size()`是错误的，因为`vec`本身不是`constexpr`变量。可以用下面的方式绕过去：

   ```c++
   constexpr auto ToArray(auto& rng)
   {
       // 在这个context中，内部的vector已经析构，我们没有保留下来，所以不违反要求。
       constexpr std::size_t size = ToVector(rng).size();
       
       using ElemType = std::ranges::range_value_t<T>;
       std::array<ElemType, size> arr;
       std::ranges::copy(ToVector(rng), arr.begin());
       return arr;
   }
   ```

   当然这要求`rng`本身不会因为遍历而修改。

4. `foo`可以，`foo2`不行，因为`foo2`内的`v`开启了新的context，在这个context中的异常没有被catch。

### Part 1

1. gcc默认允许VLA。

2. ```c++
   template<typename T, typename U>
   struct is_same
   {
       static inline constexpr bool value = false;
   };
   
   template<typename T>
   struct is_same<T, T>
   {
       static inline constexpr bool value = true;
   };
   
   template<typename T, typename U>
   inline constexpr bool is_same_v = is_same<T, U>::value;
   ```

3. 可以使用constexpr if：

   ```c++
   template<unsigned int N>
   constexpr int Foo()
   {
       if constexpr (N % 2 == 0) {
           return N / 2;
       }
       else {
           return 3 * N + 1;
       }
   }
   ```

   也可以使用特化：

   ```c++
   template<unsigned int N, bool = (N % 2 == 0)>
   struct Foo { };
   
   template<unsigned int N>
   struct Foo<N, true>
   {
       static inline constexpr unsigned int value = N / 2;
   };
   
   template<unsigned int N>
   struct Foo<N, false>
   {
       static inline constexpr unsigned int value = 3 * N + 1;
   };
   
   template<unsigned int N>
   inline constexpr unsigned int Foo_v = Foo<N>::value;
   ```


## Part 2

1. 不合法，虽然逻辑上讲二者是一样的（德摩根定律），但是`!A`和`!B`会被看作Unknown1和Unknown2，而`!(C || D)`则是Unknown3，前者的`&&`并不和后者构成subsume关系。

2. 当然可以很容易地通过特化来完成：

   ```c++
   template<typename T, typename U>
   void Insert(T& cont, const U& val)
   {
       cont.insert(val);
   }
   
   template<typename T, typename U>
   requires requires(T cont, U val) { cont.push_back(val); }
   void Insert(T& cont, const U& val)
   {
       cont.push_back(val);
   }
   ```

   > 当然，你也可以把`U`写成`std::ranges::iter_value_t<>`，少写一个模板参数：
   >
   > ```c++
   > template<typename T>
   > requires requires(T cont, std::ranges::range_value_t<T> val) 
   >     { cont.push_back(val); }
   > void Insert(T& cont, const std::ranges::range_value_t<T>& val)
   > {
   >     cont.push_back(val);
   > }
   > ```
   >
   > 当然写两遍参数感觉非常蠢（，所以补充一点：requires clause是可以写在函数头后面的。这样就可以写成下面的形式：
   >
   > ```c++
   > template<typename T>
   > void Insert(T& cont, const std::ranges::range_value_t<T>& val)
   > requires requires { cont.push_back(val); }
   > {
   >     cont.push_back(val);
   > }
   > ```

   如果使用constexpr if，可以利用requires expression可以转化为bool的性质：

   ```c++
   template<typename T, typename U>
   void Insert(T& cont, const U& val)
   {
       if constexpr (requires { cont.push_back(val); }) {
           cont.push_back(val);
       }
       else {
           cont.insert(val);
       }
   }
   ```

   当然，你也可以利用concept本身可以转化为bool的性质：

   ```c++
   template<typename T, typename U>
   concept SupportPushBack = requires(T cont, U val) { cont.push_back(val); };
   
   template<typename T, typename U>
   void Insert(T& cont, const U& val)
   {
       if constexpr (SupportPushBack<T, U>) {
           cont.push_back(val);
       }
       else {
           cont.insert(val);
       }
   }
   ```

3. ```c++
   void Func(std::convertible_to<int> auto a) {}
   ```

4. ```c++
   template<typename T>
   decltype(auto) MoveIfNoexcept(T& arg)
   {
       if constexpr (std::is_nothrow_move_constructible_v<T> || 
                     !std::is_copy_constructible_v<T>)
       {
           return std::move(arg);
       }
       else {
           return static_cast<const T&>(arg);
       }
   }
   ```

   当然，`else`里也可以使用C++17的`std::as_const`，它接受`T&`，返回`const T&`：

   ```c++
   return std::as_const(arg);
   ```

   在C++17之前，你也可以使用`std::add_const_t<T>`的type traits。
   
5. `b.Test()`和`A<float> c`不能编译通过，其他可以。

## Move Semantics

1. 先不考虑`const`，我们可以很容易写出来代码：

   ```c++
   template<typename T, typename U>
   auto&& forward_like(U&& arg)
   {
       if constexpr (std::is_lvalue_reference_v<T>) {
           return arg;
       }
       else {
           return std::move(arg);
       }
   }
   ```
   
   然后每个分支加上`const`的判断就可以了：
   
   ```c++
   template<typename T, typename U>
   auto&& forward_like(U&& arg)
   {
       static constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
       if constexpr (std::is_lvalue_reference_v<T>) {
           if constexpr (is_const) {
               return std::as_const(arg);
           }
           else {
               return arg;
           }
       }
       else {
           if constexpr (is_const) {
               return std::move(std::as_const(arg));
           }
           else {
               return std::move(arg);   
           }
       }
   }
   ```
   
1. 代码如下：

   ```c++
   decltype(auto) call(auto&& func, auto&& param)
   requires std::invocable<decltype(func), decltype(param)> // 当然不加这句也行。
   {
       return func(std::forward<decltype(param)>(param));
   }
   ```

   注意完美转发和使用`decltype(auto)`，后者不能使用`auto&&`，因为如果`func`返回值类型，万能引用会把返回值类型变为右值引用，和原函数不一致。

   > 注意就算`func`的返回类型是`void`也可以写在`return`里，返回类型也会推断为`void`。

   + 不完全一致，假设有如下函数：

     ```c++
     void Func(C val);
     Func(C{});
     ```

     我们知道，由于prvalue的copy elision，只会调用一次构造函数。但是如果使用上述调用，则相当于：

     ```c++
     void call(C&& val)
     {
         return Func(std::move(val)); 
     }
     ```

     prvalue遇到`C&&`会进行materialize，调用构造函数；随后传给`Func`时，构造`C`就会调用移动构造函数，产生了额外开销。

     > 换句话说，尽管有了完美引用，但是C++仍然不能在理论上全无代价地传递值类型参数。不过编译器可能可以在没有副作用时优化掉这次移动。

   + 我们先说C++23的情况，这个写法就非常简单了：

     ```c++
     decltype(auto) call(auto&& func, auto&& param)
     {
         decltype(auto) result = func(std::forward<decltype(param)>(param));
         // do something on result...
         return result;
     }
     ```

     我们讨论一下：

     + 如果函数返回值类型，则`decltype(auto)`推断出值类型，并发生NRVO；
     + 如果函数返回左值引用，则`decltype(auto)`推断出左值引用，也正常返回这个引用；
     + 如果函数返回右值引用，则`decltype(auto)`推断出右值引用；虽然`result`这个右值引用是左值，但是`return`时会视作xvalue，仍然可以正常返回对应的右值引用。

     > 注意，我们课上说过。视作`xvalue`并不影响返回值类型的类型推断，因为`result`是一个变量，对它的推导就是原本的类型。因此，`return result;`并不会推断出右值引用。但是，如果是`return (result);`，则由于后者是表达式，并规定产生xvalue，于是推断出右值引用。
     >
     
     对于C++23之前，可能有人是这么写的：
     
     ```c++
     decltype(auto) call(auto&& func, auto&& param)
     {
         decltype(auto) result = func(std::forward<decltype(param)>(param));
         return static_cast<decltype(result)>(result);
     }
     ```
     
     注意，虽然直接`return result;`推断的返回类型是正确的，但是当`result`是右值引用时，由于`result`是左值，不能直接绑定右值引用，因此需要这个转型。但是，上述写法还有一个问题：对于值类型，`return result;`是可以NRVO的，但`static_cast<T>`由于不是name，因此不能进行NRVO，甚至不能进行implicit cast，于是**多了一次拷贝**。解决方案就是使用`if constexpr`：
     
     ```c++
     decltype(auto) call(auto&& func, auto&& param)
     {
         decltype(auto) result = func(std::forward<decltype(param)>(param));
         using T = decltype(result);
         if constexpr (!std::is_reference_v<T>) {
             return result;
         }
         else {
             return static_cast<T>(result);   
         }
     }
     ```
     
     或者由于只有右值引用需要转型，也可以写成：
     
     ```c++
     decltype(auto) call(auto&& func, auto&& param)
     {
         decltype(auto) result = func(std::forward<decltype(param)>(param));
         using T = decltype(result);
         if constexpr (std::is_rvalue_reference_v<T>) {
             return std::move(result);
         }
         else {
             return result;
         }
     }
     ```
     
     

