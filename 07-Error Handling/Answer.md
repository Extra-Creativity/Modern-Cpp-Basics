# Answer

1. 全部都会拷贝，不能直接使用引用类型作为模板参数。若使用`std::reference_wrapper`，则代码要改动为：

   ```c++
   void Test(
       std::optional<std::reference_wrapper<std::vector<int>>> optVectorParam)
   {
       if (optVectorParam)
       {
           auto &optVector = optVectorParam->get();
           if (!optVector.empty())
               std::println("{}", optVector[0]);
       }
       return;
   }
   ```

   与指针对比一下：

   ```c++
   void Test(std::vector<int>* optVectorPtr)
   {
       if (optVectorPtr && !optVectorPtr->empty()) // still short-circuit
           std::println("{}", optVector[0]);
       return;
   }
   ```

   显然迂回的策略十分丑陋；此外，我们说过`referece_wrapper`本质上就是有一个`T*`，`optional`又给它搞了一个`bool`，明明空指针就能表示这种空状态，白白浪费了空间。

   **总结：对于函数参数，如果非空参数本来是要传递引用`T&`，则（在相关提案通过前）传递可空参数不要使用`std::optional<T&>`，而是使用`T*`**。

   在P2988里，`std::optional<T&>`与指针是比较类似的：

   ```c++
   void Test(std::optional<const std::vector<int>&> optVectorPtr)
   {
       if (optVectorPtr && !optVectorPtr->empty()) // still short-circuit
           std::println("{}", optVector[0]);
       return;
   }
   ```

   它的本质实现也是存储了一个指针；因此，**不能使用临时值来对其进行初始化**：

   ```c++
   Test(std::vector{ 1,2,3 }); // Compile error
   ```

   因为`optional`不期望进行任何生命周期的管理，所以明确拒绝`const&`这种自动延长临时值生命周期的功能。但是它留了一个空子：

   ```c++
   Test(std::optional{ std::vector{ 1,2,3 } }); // Yes
   ```

   也就是说可以用`std::optional<U>`构造`std::optional<T&>`，此时后者得到的是前者存储的值的地址。**但是这种接受临时值的方式只在函数参数这里有效，因为函数调用的过程中`std:::optional<U>`没有析构，而如果析构了就会导致生命周期上的错误**。例如：

   ```c++
   std::optional<int> Func() { return 1; }
   std::optional<int&> dangling{ return Func(); } // 编译通过，但是后续访问dangling是错误的。
   // 等价于：int* dangling = &(Func().value());
   ```

   这是因为`std::optional<int>`是临时的，自然指向它内部值的指针在其析构后也就悬垂了。

2. 略，比较简单。

3. 内部的`noexcept(a < b)`判断了`a < b`是否有noexcept specifier，如果是则得到true，否则为false，再配合外面的`noexcept`，于是表示当`a < b`为`noexcept`时，当前函数也为`noexcept`，否则反之。

4. 这个题比较麻烦，我们用基类的方式做一下：

   ```c++
   #include <algorithm>
   #include <memory>
   
   template<typename T>
   class ListBase
   {
   protected:
       ListNode<T> sentinel_{ &sentinel_, &sentinel_ };
       ~ListBase()
       {
           for (auto it = sentinel_.next; it != &sentinel_;)
           {
               it = it->next;
               delete it->prev; 
           }
           
           // 错误写法：在it删除后继续使用。
           // for (auto it = sentinel_.next; it != &sentinel_; it = it->next)
           //     delete it;
       }
   };
   
   template<typename T>
   class List : public ListBase<T>
   {
       auto& GetSentinel_() { return this->sentinel_; }
       
   public:
       class ConstIterator
       {
           const ListNode<T> *node_;
   
       public:
           ConstIterator(const ListNode<T> *node) : node_{ node } {}
           ConstIterator operator++(int) noexcept
           {
               auto node0 = node_;
               node_ = node_->next;
               return ConstIterator{ node0 };
           }
   
           ConstIterator &operator++() noexcept
           {
               node_ = node_->next;
               return *this;
           }
           const T &operator*() const noexcept { return node_->val; }
           const T *operator->() const noexcept { return &(node_->val); }
           bool operator==(ConstIterator another) const noexcept
           {
               return node_ == another.node_;
           }
       };    
       
       template<typename It>
       List(It begin, It end)
       {
           auto pos = &this->sentinel_;
           while (begin != end)
           {
               std::unique_ptr<ListNode<T>> ptr{ new ListNode<T>{ pos, pos->next,
                                                            *begin } };
               ++begin;
   
               pos->next->prev = ptr.get();
               pos->next = ptr.release();
               pos = pos->next;
           }
           return;
       }
   
       auto begin() const { return ConstIterator{ this->sentinel_.next }; }
       auto end() const { return ConstIterator{ &this->sentinel_ }; }
       
       List(const List& another) : List{ another.begin(), another.end() } {}
       void swap(List &another)
       {
           std::swap(GetSentinel_().prev, GetSentinel_().prev);
           std::swap(GetSentinel_().next, GetSentinel_().next);
       }
       
       List& operator=(const List &another)
       {
           // 尽管自赋值在Copy-and-swap中不用判断也能处理正确，但是提前离开减少拷贝也是有益的。
           if (this == &another)
               return *this;
           List list{ another };
           swap(list);
           return *this;
       }
   };
   ```

5. 不能，因为如果`scores`插入时异常抛出，则`names`和`scores`的数量将会不一致。一种解决方法是把`name`和`score`合并为一个结构体，只保留一个`std::vector<Info>`；除此之外也可以进行`try-catch`，如果抛出了则看二者大小是否一致，不一致则pop出去一个，然后`throw;`重新抛出异常。我们在模板这一章会写一个通用的`PushbackGuard`来对任意数量的`std::vector`进行整体的数量保持。
