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

## 后半部分

1. 我们用一道题来说明指针的比较不只是内部地址值的比较，还涉及到类型的转化。

   ```c++
   struct A { int a; };
   struct B { int b; };
   struct C : A, B { int ddd; };
   C c;
   ```

   下面的各个`bool`，哪些可以确定结果（是什么？），哪些不能确定？为什么？

   ```c++
   A* aPtr = &c;
   bool test = (aPtr == &c);
   bool test2 = (static_cast<C*>(aPtr) == &c);
   bool test3 = (reinterpret_cast<C*>(aPtr) == &c);
   
   B* bPtr = &c;
   bool test4 = (bPtr == &c);
   bool test5 = (static_cast<C*>(bPtr) == &c);
   bool test6 = (reinterpret_cast<C*>(bPtr) == &c);
   ```

   如果我们把`A`和`B`的成员都去掉，只是空类，上面的结果有变化吗？

   > 这个题还是有难度的，不会可以直接看答案。。

2. 我们来给Slicing problem一个解决方案。对下面的继承关系：

   ```c++
   class A
   {
   public:
       A(int init_a) : a{init_a}{}
       int GetBaseVal() const { return a; }
       virtual ~A() = default;
       // 编写一个Clone方法
   protected:
       // TODO...
   private:
       int a;
   };
   
   class B : public A
   {
   public:
       B(int init_a, int init_b): A{init_a}, b{init_b} {}
       int GetDerivedVal() const { return b; }
       // 编写一个Clone方法
   private:
       int b;
   };
   ```

   使得下面的代码可以成立/报错：

   ```c++
   void Test(A& a)
   {
       B b{2,3};
       a = b; // Make it compile error
       a.Clone(b); // Right
   }
   
   void Test2(A& a)
   {
       A a2{4};
       a = a2; // Make it compile error
       a.Clone(a2); // Right
   }
   
   A a{1};
   B b{4,5};
   Test(a);
   std::println("{}", a.GetBaseVal()); // 2
   Test2(a);
   std::println("{}", a.GetBaseVal()); // 4
   
   Test(b);
   std::println("{} {}", b.GetBaseVal(), b.GetDerivedVal()); // 2, 3
   Test2(b); // 运行时错误（即利用转型的抛异常），因为A不能拷贝到B类型。
   ```

3. `const_cast`的一个例子：[c++ - execv() and const-ness - Stack Overflow](https://stackoverflow.com/questions/190184/execv-and-const-ness)。

4. `typeid`的例子：

   ```c++
   class A { };
   class B : public A { };
   
   void OutputType(A& a)
   {
       std::println("{}", typeid(a).name());
   }
   
   B b;
   OutputType(b);
   ```

   看看输出什么；如果给`A`加一个public的虚析构函数，再看看会输出什么。

5. 练习一下`std::variant`，对`std::vector<std::variant<int, float, std::string>>`，把所有的元素拼接为字符串；特别地，如果是`std::string`，则不变；否则用`std::to_string`变为字符串。

   > **特别地，在C++26开始，`visit`成为了`std::variant`的一个成员方法。**

6. 对于`std::any`，有一种比较巧妙的使用方法来保留类型的部分信息，从而完成无继承和虚函数的情况下实现不同类型的函数调用。这么说可能有点抽象，我们来看一个例子：

   ```c++
   class AnimalInterface { public: void Talk(int); std::string Walk(); };
   class Dog { 实现上面两个函数; };
   class Cat { 实现上面两个函数; };
   
   void Test(const AnimalInterface& animal)
   {
       animal.Walk();
       std::println("{}", animal.Talk(1));
   }
   
   int main()
   {
       Test(Dog{});
       Test(Cat{});
       return 0;
   }
   ```

   和继承的区别在于，如果继承想实现上述功能，就需要`Dog`和`Cat`都继承自`AnimalInterface`，同时`Talk`和`Walk`都是虚的，最后也必须用引用和指针才能指涉子类型。而我们的类的目标在于消除这些问题，**变为非侵入式的设计，即用户只需要实现AnimalInterface规定的接口，就可直接赋给`AnimalInterface`**。

   既然想达到这么一个目的，我们自然的想法是将获得的object赋给`std::any`进行存储，于是大概长这样：

   ```c++
   class AnimalInterface
   {
   public:
       // 1. 写一个模板构造函数，它接受任意对象，用以构造hiddenObject
   private:
       std::any hiddenObject_;
   };
   ```

   随后，我们考虑为它增加`Talk`和`Walk`方法。注意到我们的模板类型只存在于构造函数中，没法在`Talk`和`Walk`中用`any_cast`恢复原来的类型。因此，我们需要在构造函数中就把这个类型以某种方式固定为我们的成员，至少是在函数调用中可以使用的成员。

   我们先不考虑固定的问题，先考虑在构造函数里如何写这么一个方法。自然可以用lambda表达式：

   ```c++
   // 2. 请在构造函数里写一个lambda表达式，它对hiddenObject做any_cast，变换回原本的类型，再调用Walk。
   ```

   我们可以注意到，带有capture的lambda表达式是不能作为成员的，因为它的类型不能跑到成员位置去。但是stateless lambda是可以的！通过转型成函数指针，我们就可以固定下来这个lambda。又注意到，我们只对`hiddenObject`做了capture，完全可以把它变换到参数去，从而让lambda无capture。

   ```c++
   // 3. 请给AnimalInterface写两个函数指针成员walkProxy_和talkProxy_，使得它能够正确地保存两个stateless lambda；如果你没明白这个需求，看一下我们后面要求怎样调用。
   ```

   这样，我们就可以完整的实现`AnimalInterface`的`Walk`和`Talk`：

   ```c++
   void Walk() { walkProxy_(hiddenObject_); }
   std::string Talk(int times) { return talkProxy_(hiddenObject_, times); }
   ```

   来测试一下，对下面的两个类，你的`AnimalInterface`可以正确工作：

   ```c++
   class Dog
   {
   public:
       void Walk() { std::println("Dog walk..."); }
       std::string Talk(int times)
       {
           std::string result{};
           for (int i = 0; i < times; i++)
               result += "Woof! ";
           return result;
       }
   };
   
   class Cat
   {
   public:
       void Walk()  { std::println("Cat walk...");  }
       std::string Talk(int times)
       {
           return "Meow! Lazy to talk for " + std::to_string(times) + " times.";
       }
   };
   
   // 事实上如果我们的Walk和Talk实现为const方法（当然这最好也一并要求Dog和Cat也是const的），这里也可以用const AnimalInterface&。
   void Test(AnimalInterface animal)
   {
       animal.Walk();
       std::println("{}", animal.Talk(1));
   }
   
   int main()
   {
       Test(Dog{});
       Test(Cat{});
       return 0;
   }
   ```

   