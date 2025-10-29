# Assignment

## Part 1

1. 在存在一些可能为空的辅助类（例如allocator、deleter等）时，我们通常会使用EBO来缩小这个类的大小。请写一个类`CompressedPair<T, U>`，其中`U`为可能为空的类，并暴露`first`和`second`的函数接口。

1. 通常情况下，我们会将互相关联的数据包装成一个结构体，例如：

   ```c++
   struct Color { float r; float g; float b; };
   ```

   如果我们想表达一张RGB图片，可以使用数组：

   ```c++
   Color color[1920 * 1080];
   ```

   除此之外，我们还可以使用下面的方式来表示一张图片：

   ```c++
   struct Picture
   {
       float r[1920 * 1080];
       float g[1920 * 1080];
       float b[1920 * 1080];
   };
   ```

   试讨论两种存储方式在内存排布与访问性能上的差别。

   > 这两种方式分别称为Array of Struct和Struct of Array（AOS & SOA）。

1. 对于无特殊对齐的类A，小明想要分配出一个1024对齐的对象。在栈上分配是容易的：

   ```c++
   alignas(1024) A a;
   ```

   然而在堆上，由于对齐没有在A的定义上指定，因此普通的`new`是无效的。我们在课上要求加一个结构体类来进行包装，而小明觉得太麻烦，于是写下如下代码：

   ```c++
   A* ptr = new(std::align_val_t{1024}) A;
   ```
   
   不过小明的类`A`是多继承自`B, C`的，在之后他转型成了`C`的指针`ptr2`，在之后用下面的方式进行了delete：
   
   ```c++
   ptr2->~C();
   // To call the correct aligned delete
   ::operator delete(ptr2, std::align_val_t{1024});
   ```
   
   `B`和`C`都存在虚析构函数时，请问上述代码是否一定正确？
   
   > msvc编译器[存在bug](https://stackoverflow.com/questions/55207941/alignment-in-new-operator-c17-visual-studio)，不能手动调用aligned operator new。

## Part2

补充：我们在PPT中说`delete ptr`中，当指针的底层类型`T` incomplete且有non-trivial dtor时，表达式是UB。在C++26开始这一点得到了加强，当`T`为incomplete时直接导致编译错误。

1. 在上一章中，我们提到`LockFreeStack`的实现中假设了拷贝不会抛出异常，否则下面的代码是错误的：

   ```c++
   std::optional<T> Pop()
   {
       Node* oldHead = head_.load();
       while (oldHead && !head_.compare_exchange_weak(oldHead, oldHead->next));
       // 如果下面这句抛出异常，则虽然head的数据没有被读到，但是head已经被pop出去了，造成数据丢失。
       std::optional<T> result = oldHead ? std::nullopt : oldHead.data;
       return result;
   }
   ```

   此外，为了防止悬垂指针的问题，我们也必须让内存泄漏出去。思考如何使用智能指针来解决这两个问题。

2. [有人提出](https://solidean.com/blog/2025/the-vimpl-pattern-for-cpp/)，可以使用"vimpl"来代替pimpl，如下：

   ```c++
   // 头文件
   class A
   {
   public:
       virtual void Func(int a) = 0;
       virtual ~A() = default;
       
       static std::unique_ptr<A> Create(float b);
   };
   
   // 源文件
   class AImpl final : public A
   {
   public:
       A(float b);
       void Func(int a) override { /* ... */ }
   };
   
   std::unique_ptr<A> A::Create(float b)
   {
       return std::make_unique<AImpl>(b);
   }
   ```

   试说明这种方式和pimpl相比有哪些优点与缺点。

3. 在课堂上，我们讨论了“不要滥用智能指针”的问题，并通过`unique_ptr`和裸指针作为函数参数的例子进行了解释。试讨论`shared_ptr`的各种函数参数变体（值类型、`&`等等）应当在何时使用。

4. 观察下面的代码，应当输出什么？

   ```c++
   class A
   {
   public:
       virtual ~A() { std::println("A destructed."); }
   };
   
   class B : public A
   {
   public:
       ~B() { std::println("B destructed."); }
   };
   
   int main() { std::shared_ptr<A> ptr = std::make_shared<B>(); }
   ```

   如果把`virtual`去掉，又会输出什么？在实际的编译器上进行测试，看看结果是否符合你的预期，并解释原因。

5. `shared_ptr(Y*)`为什么要求complete type（这个实际上是PPT里举得`A&&!B`的最后一个例子，我们在讲解的时候没有特别提醒）。

   在pimpl中，我们提到`std::unique_ptr`必须把特殊的成员函数的定义全部放到源文件（即`Impl`的完整定义之后）；如果像下述代码一样使用`std::shared_ptr`，是否还有这种约束？

   ```c++
   class A
   {
       struct Impl;
       std::shared_ptr<Impl> impl_
   public:
       // A(A&& another); ?
   };
   ```

   我们提到，在pimpl中实现继承时，可以像QT一样将`DerivedImpl*`赋值给基类的`Impl*`。小明不想将`ptr`本身暴露为`protected`，于是写了一个构造函数给子类进行指针的赋值：

   ```c++
   class A
   {
       struct Impl;
       std::unique_ptr<Impl> impl_;
       
   protected:
       A(Impl* initImpl) : ptr{ initImpl } { }
       // 还有其他放在源文件里的特殊成员函数，略。
   };
   ```

   思考此时直接改用`std::shared_ptr`是否正确。

6. 在`enable_shared_from_this`中，我们提到将类的构造函数标记为`private`，而只暴露产生`shared_ptr`的接口。然而，我们可以发现使用`make_shared`会导致编译错误：

   ```c++
   class A : public std::enable_shared_from_this<A>
   {
       A(int a) { /* ... */ }
   public:
       static std::shared_ptr<A> Create(int a)
       {
           // WRONG: return std::make_shared<A>(a);
           return std::shared_ptr<A>{ new A{ a } };
       }
   };
   ```

   思考原因，并尝试给出解决方案。

   > Hint：使用local class + 继承。

### Part 3

补充：allocator导致move assignment和swap不一定是`noexcept`的，例如`vector`的[specification](https://cppreference.com/w/cpp/container/vector/operator=.html#Exceptions)：

```c++
noexcept(
    std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
    std::allocator_traits<Allocator>::is_always_equal::value
)
```

也即仅当指针可直接交换时`noexcept`，同时编译期不能确定allocator的比较结果，所以只能使用`is_always_equal`。特别地，这就导致`pmr`的容器move assignment和swap都不是noexcept的。

我们之前说过，让move ctor, move assignment & swap变为`noexcept`可以提升异常安全性或运行效率；不过仅仅让move ctor是`noexcept`已经可以应付许多优化（如`vector`的`push_back`），见[StackOverflow的回答](https://stackoverflow.com/questions/66459162/where-do-standard-library-or-compilers-leverage-noexcept-move-semantics-other-t/66498981#66498981)。

----------

1. Alloctor-aware uninitialized memory algorithm & construction guard.
1. 改写出Allocator-aware list，注意POCMA之类的问题。
