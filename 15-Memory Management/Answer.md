# Answer

## Part 1

1. 见`Answer-code/CompressedPair.cpp`。有以下几点需要注意：

   + 类似`std::pair`，额外实现了`std::piecewise_construct_t`的重载，使得可以使用下面的方式来构造：

     ```c++
     // 10个1的vector
     CompressedPair<std::vector<int>, A> pair{ std::piecewise_construct,
                                               std::tuple{10, 1}, std::tuple{ A{} }};
     ```

     为了解码出`tuple`的所有元素，使用了index sequence。特别地，函数模板参数可以通过推断来划分，因此允许多个pack；如果是函数参数本身，就不能同时出现多个pack。

   + `First`中的deducing this可以对不同的值类别返回不同的引用类型：

     ```c++
     using TestPair = CompressedPair<int, A>;
     static_assert(requires(TestPair pair, const TestPair cpair) {
         { pair.First() } -> std::same_as<int&>;
         { cpair.First() } -> std::same_as<const int&>;
         { std::move(pair).First() } -> std::same_as<int&&>;
     });
     ```

     `std::forward_like`是C++23引入的新函数，我们在模板的作业中有过讨论。

   + `Second`的实现中，我们进行了`EmptySecondType`的额外转型，这样才能让模板推断出的是基类类型，从而返回的类型也是基类的类型。我们之所以使用了C风格的转型而不是`static_cast`，是因为对于`const& self`不能进行`static_cast<EmptySecondType&>`（因为丢掉了`const`）。但我们这里的目的只是为了把原引用传进去，具体转成什么类型由`forward_like`根据`self`完成，所以直接用C风格一步简写。

     > 特别地，这里不能使用`static_cast<const EmptySecondType&>`，因为`const`不论怎么forward都只能是`const&`或者`const&&`，没法直接去掉`const`。

   如果想压缩N个可能的空类，你可以类似地通过递归继承来实现`CompressedTuple`。

2. 显然区别就在于每个成员是否是连续排布的，因此性能也就取决于具体的访问模式。例如，如果某个函数只需要访问所有的`r`通道，那么AOS每次访问都会导致跨越12字节的步长，造成更频繁的cache行替换，就算存在prefetching，也可能比短步长要更慢一些。而如果要同时访问rgb，则SOA的访问需要同时占用三个cache行。

   > 一般来说，结构体数据成员很多、但同时又要循环集中访问某个成员比较常见，所以SOA很多情况下的性能比AOS要高。

3. 不一定，因为`operator delete`接受的是`void*`，因此会发生指针的转型。因此，只能保证下面的代码是正确的：

   ```c++
   ptr->~A();
   ::operator delete(ptr, std::align_val_t{1024});
   ```

   而一旦转型为`C*`，就算虚析构函数可以正常析构，`ptr2`实际的地址和`ptr`是不相同的，转成`void*`丢失了这一信息，从而我们`delete`得到的地址也就不是原来的地址了。

   解决方式是使用`dynamic_cast<void*>`转为most derived object的指针，你可能已经忘记了这个功能，但是在这里它神奇地出现了。

   ```c++
   void* addr = dynamic_cast<void*>(ptr2);
   ptr2->~C();
   ::operator delete(addr, std::align_val_t{1024});
   ```

   注意`dynamic_cast`应该在析构函数之前进行，否则对象已经不存在，也就没有most derived class的说法。而且我们说过，这个转型是非常快的。

   > 特别地，我们课上说的"Deleting destructor"会自动处理这一点，也就是下面的代码是正确的：
   >
   > ```c++
   > // 定义在Derived中
   > void operator delete(void* ptr) noexcept
   > {
   >     std::println("Derived: Called overrided operator delete, ptr={}", ptr);
   >     free(ptr); // 这里ptr是most derived object的地址，相当于已经dynamic_cast过了。
   > }
   > 
   > Base2* ptr = new Derived;
   > delete ptr;
   > ```

## Part 2

1. 一种简单的想法是`data`干脆使用`std::shared_ptr`来存储：

   ```c++
   struct Node
   {
       std::shared_ptr<T> data;
       Node* next;
   };
   ```

   然而，在`Pop`中我们需要把`Node*`给delete掉，这样`data`又无效了。C++ Concurrency in Action这本书中引入了hazard pointer来非常复杂地解决这个问题（当然不排除作者就是为了讲hazard pointer），但实际上可以通过课上讲的aliasing constructor共享ownership的方式简单实现：

   ```c++
   struct Node
   {
       T data;
       Node* next;
   };
   
   std::shared_ptr<T> Pop()
   {
       Node* oldHead = head_.load();
       while (oldHead && !head_.compare_exchange_weak(oldHead, oldHead->next));
       if (!oldHead)
           return nullptr;
       
       std::shared_ptr<Node> node{ oldHead };
       return std::shared_ptr<T>{ std::move(node), &(oldHead->data) };
   }
   ```

   这样，我们就把`Node`的实际管理权限交给了用户，当用户不再使用`data`也即抛弃`shared_ptr`时，会自动对`oldHead`进行`delete`。

2. 优点：方法的实现只需要`override`，不需要手写转发，写起来方便很多；同时`final`也能够去除掉在源文件中调用自身函数的开销。

   缺点：

   + ABI不总是保持稳定；在不合适的地方增加虚方法或打乱虚方法顺序时，会产生错误的虚表偏移，导致动态库使用时调用到错误的方法。
   + 产生的是`unique_ptr`，不能进行拷贝，需要自己写新的`Copy`方法（在C++26可以通过暴露`std::polymorphic`来解决）。

3. 类似地：

   + 值类型：需要掌握对应资源的所有权时，例如我们课上`Work`的例子，因为新线程要`detach`出去，所以为了保证资源有效必须提前掌控。

     > 同样，一般的函数只需要裸指针，并不关心所有权的问题，由调用者保证指针在使用过程中始终有效。

   + `&`：修改`shared_ptr`本身，例如设置新的资源等。

   + `&&`：需要转移`shared_ptr`的所有权时。

   + `const&`：虽然`const shared_ptr`仍然不能直接修改，但是由于可以拷贝，所以少数情况下可能需要，例如：

     ```c++
     void Func(const std::shared_ptr<Resource>& ptr)
     {
         if (needToHoldResource)
             FuncImpl(ptr);
     }
     ```

     如果我们直接把值类型作为`Func`的参数，则无论如何都会发生拷贝，相比于现在的方式可能有少量的性能差距。

4. 不管有没有`virtual`，都将输出：

   ```text
   B destructed.
   A destructed.
   ```

   这还是类型擦除的功劳，`make_shared`正常产生了一个`shared_ptr<B>`，对应的分配出来的control block是以`B`为模板参数的，传递给`shared_ptr<A>`并不会改变这点。类似地，对于下面的代码：

   ```c++
   std::shared_ptr<A> ptr{ new B{} };
   ```

   由于`shared_ptr`的构造函数是模板：

   ```c++
   template< class Y >
   explicit shared_ptr(Y* ptr);
   ```

   同样产生的是以`B`为模板参数的control block，也没有问题。

   > `std::polymorphic`也是类似的原理。

5. 第一个问题：没有这种约束。`std::unique_ptr`之所以有这个问题是因为相当于在头文件里直接写下了下面的语句：

   ```c++
   ~A() { delete impl; } // unique_ptr不允许delete incomplete type。
   ```

   然而，由于`shared_ptr`使用了类型擦除，相当于：

   ```c++
   ~A() { controlBlockPtr->destroy(); }
   ```

   存储的是一个基类的指针，调用其虚方法时不需要知道下面的类的定义（否则就不可能有虚基类了）。对于其他特殊成员函数同理。

   第二个问题：不正确，因为`shared_ptr`中发生了下面的事情：

   ```c++
   shared_ptr<AImpl>() : controlBlockPtr{ new ControlBlock<AImpl> } { }
   ```

   因此在头文件的构造函数中就需要对`ControlBlock<AImpl>`进行实例化，而通常实例化类的虚方法也要直接跟着实例化，于是就在头文件中没能看到完整定义时就定义了`delete`语句，仍然是错误的。相反，此时`std::unique_ptr`可以编译成功，因为只是发生了普通的指针赋值。

6. 原因：本质上`make_shared`对应的control block要在构造函数里主动构造`A`，而`A`的构造函数又是`private`的，control block没有权限去构造，于是编译错误。

   一种可行的解决方案是通过local class继承，可以向`shared_ptr`暴露`A`的构造函数，也基本防止了将其泄露给用户：

   ```c++
   class A : public std::enable_shared_from_this<A>
   {
       A(int a) { /* ... */ }
   public:
       static std::shared_ptr<A> Create(int a)
       {
           class B : public A
           {
           public:
               B(int a) : A{ a } { }
           };
   
           return std::make_shared<B>(a);
       }
   };
   ```

   对于多个构造函数的情况，用模板会方便一些：

   ```c++
   class A : public std::enable_shared_from_this<A>
   {
       A(int a) { /* ... */ }
       A(float b) { /* ... */ }
   
   public:
       template<typename... Args>
       static std::shared_ptr<A> Create(Args&&... args)
       {
           class B : public A
           {
           public:
               B(Args&&... args) : A{ std::forward<decltype(args)>(args)... } { }
           };
   
            return std::make_shared<B>(std::forward<Args>(args)...);
       }
   };
   ```

   这种写法唯一的缺点是IDE可能并不能识别出一连串的转发，用户在使用`Create`的时候需要自己去看`A`的构造函数的参数列表。
