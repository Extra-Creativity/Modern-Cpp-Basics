# Assignment

### 前半部分

1. 我们先来看一个生命周期UB造成的问题，以说明生命周期的重要性，防止大家说我危言耸听...

   ```c++
   #include <iostream>
   #include <cassert>
   
   void SetValue(void* dst)
   {
       int* m = (int*)dst;
       for(int i = 0; i < 4; i++)
           m[i] = i;
       return;
   }
   
   int main()
   {
       assert(sizeof(int) == sizeof(float) && alignof(int) == alignof(float));
       
       float a[4]{};
       SetValue(a);
       for(int i = 0; i < 4; i++)
           std::cout << a[i] << ' ';
       return 0;
   }
   ```

   在gcc（善用compiler explorer）上用`-O0`和`-O2`分别尝试，看看输出的结果是否一样。

   > 理论上的原因就是`a`里面有四个在生命周期中的`float`，用`int*`去访问是不对的，UB使得gcc随意优化。

2. 下面的函数是否有违反生命周期的情况？

   ```c++
   const std::vector<int>& SomeFunc(const std::vector<int>& vec)
   {
       return vec;
   }
   
   const auto& a = SomeFunc({1,2,3});
   std::cout << a[0];
   ```

   这道题需要比较灵活地运用临时变量和引用的生命周期的关系分析才能得出正确结论。

   看一道类似的题，来自[Stack Overflow - Lambda passed by reference runs when invoked in the constructor, but not when later stored in a data member](https://stackoverflow.com/questions/76815744/lambda-passed-by-reference-runs-when-invoked-in-the-constructor-but-not-when-la)：

   ```c++
   #include <functional>
   #include <iostream>
   
   class LambdaStore
   {
   public:
       LambdaStore(const std::function<void(float)>& init_fn)
           : fn{init_fn}
       {
           fn(11.1f);    // works, why?
       }
   
       // crashes, why?
       void ExecuteStoredLambda() { fn(99.9f); }
   
   private:
       const std::function<void(float)>& fn;
   };
   
   int main()
   {
       LambdaStore lambdaStore([](float a) { std::cout << a << '\n'; });
       lambdaStore.ExecuteStoredLambda();
       return 0;
   }
   ```

   这里只需要改变一个字符，就可以让代码无UB，是哪个？

   > 特别说一句（和这题关系不大），无捕获的lambda表达式（stateless lambda）是可以隐式转型成函数指针的，当lambda表达式离开生命周期后也能正常用函数指针访问相应的函数（结合没有捕获来理解，这件事情还算自然）。

3. 我们写一个非常简单的`inplace_vector`，练习一下placement new。这是C++26会引入的新容器，它在栈上分配，但与`std::array`不同的是，它只是保留了固定大小的内存，在插入的时候才会真正构造。我们只完成它的析构函数、`.size()`、`operator[]`、`push_back`和`pop_back`。

   ```c++
   template<typename T, std::size_t N>
   class InplaceVector
   {
   public:
       InplaceVector() = default;
       
       std::size_t Size() const;
       void PushBack(const T&);
       void PopBack();
       // operator[]，const和非const两个版本
       
       ~InplaceVector() { ... }
   private:
       // 我们假设成员是一个具有合适对齐的buffer，能够放下N个元素。
       // 同时需要一个std::size_t来记录它目前的实际大小。
   };
   ```

   我们要求：

   + 析构函数正确地释放每一个元素。
   + `operator[]`、`push_back`和`pop_back`暂时不检查当前的实际大小是否超过`N`。

   > 思考一下，这个buffer和`std::array<T, N>`内部使用的`T[N]`有什么区别？

4. 对下面的变量进行内存重用，哪些合法，哪些非法？

   ```c++
   struct A { int a; const float b; } a1;
   static const int a2 = 2;
   
   int main()
   {
       const int a3 = 2;
       const A a4{ 1, 2.0f };
       // 重用a1.a是否合法？a1.b是否合法？a2, a3, a4,a, a4.b呢？
   }
   ```

5. 下面的代码合法吗？

   ```c++
   std::byte* dataFromNetwork = stream->getBytes(); // 得到一个已经写入内容的byte buffer
   if (dataFromNetwork[0] == std::byte{'A'})
       process_foo(reinterpret_cast<Foo*>(dataFromNetwork));
   else
       process_bar(reinterpret_cast<Bar*>(dataFromNetwork));
   ```

   假设`Foo`和`Bar`都是trivially copyable的。