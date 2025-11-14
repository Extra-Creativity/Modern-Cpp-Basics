# Assignment

1. 这道题讨论一下`optional`的性能问题，这是一个重要的问题。考虑下面的情况：

   ```c++
   void Test(/* TYPE */ optVector)
   {
   	if (optVector && !optVector->empty()) // short-circuit.
           std::println("{}", optVector[0]);
       return;
   }
   
   int main()
   {
       std::vector<int> v{ 1,2,3 };
       Test(v);
       return 0;
   }
   ```

   + 当`TYPE`为`std::optional<std::vector<int>>`时，是否发生了拷贝？
   + 当`TYPE`为`const std::optional<std::vector<int>>&`时，是否发生了拷贝？
   + `TYPE`能否为`std::optional<const std::vector<int>&>`？

   可以看到，一旦涉及到`optional`，那么无论用户如何传递参数，都会导致效率问题。我们之前说过，引用不行，还有一种替代引用的类是？

   没错，一种迂回的策略是使用`std::optional<std::reference_wrapper<std::vector<int>>>`。那么：

   + 当`TYPE`为`std::optional<std::reference_wrapper<std::vector<int>>>`时，`Test`的代码应该如何改动才能得到与之前相同的语义？
   + 除了上面的改动，你认为这种方式相比于`T*`有什么其他问题？

   在[P2988](https://github.com/cplusplus/papers/issues/1661)引入了`std::optional<T&>`的特化，我个人认为这个提案的通过阻力应该比较小，主要的分歧在于`value_or`到底返回`T&`还是`T`，大概能顺利进入C++26，在作业答案中我们会给出其已经确定的使用方法。

2. 给第五节作业中`InplaceVector`增加异常处理相关的API。具体地：

   + 对于`PushBack`，如果大小大于buffer大小，则抛出`std::bad_alloc`。
   + 增加`.at()`，在`operator[]`的基础上进行边界检查，越界则抛出`std::out_of_range`。
   + 对显然的方法增加`noexcept`限定符。

3. 除了直接使用`noexcept`外，函数还允许`noexcept(bool)`来定义noexcept限定。即：

   ```c++
   void Func() noexcept(true); // same as void Func() noexcept;
   void Func() noexcept(false); // same as void Func();
   ```

   试解释以下声明方式：

   ```c++
   template<typename T>
   const T& min(const T& a, const T& b) noexcept(noexcept(a < b))
   {
       return a < b ? a : b;
   }
   ```

4. 练习下RAII和异常安全性，利用Copy-and-swap idiom进行强异常安全性保证。

   ```c++
   template<typename T>
   struct ListNode
   {
       ListNode* prev;
       ListNode* next;
       T val; // 正常来说应该是像InplaceVector一样搞个buffer
              // 这样就不用担心T不能默认构造的问题，我们简化处理
   };
   
   template<typename T>
   class List
   {
       // 可以修改到其他类里
       ListNode<T> sentinel_{ &sentinel_, &sentinel_ };
      
       // 如果你使用基类并把sentinel放到里面，this->是必需的。我们在模板会讲述原因。
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
           bool operator==(const ConstIterator &another) const noexcept = default;
       };    
       
       template<typename It>
       List(It begin, It end)
       {
           // TODO...
           // 如果构造失败，注意对分配的元素进行释放
           // 你可以选择使用标准库中VectorBase或VectorImpl一样的方法
           // 利用已经构造好的部分总会析构的特性
           // 也可以自己写一个Guard类承接sentinel并连接出链表，在退出前再Release给sentinel。
       }
   
       auto begin() const { return ConstIterator{ this->sentinel_.next }; }
       auto end() const { return ConstIterator{ &this->sentinel_ }; }
       
       // 委托构造函数(Delegating ctor)，我记得好像是讲过了
       List(const List& another) : List{ another.begin(), another.end() } {}
       List& operator=(const List &another)
       {
           // TODO...
       }
       // 根据你的实现方式，确定是否需要手动写~List()。
   };
   ```
   
   使用下面三段代码进行测试:
   
   ```c++
   // 功能性测试，需要代码不崩溃正常退出。
   std::vector<int> v{ 1,2,3 };
   List<int> l{ v.begin(), v.end() };
   for (auto it = l.begin(); it != l.end(); it++)
       std::println("{}", *it);
   ```
   
   ```c++
   // 基本异常安全性测试，应该保证Constructed输出两次（拷贝导致的），Dtor输出六次（vector内三次+sentinel一次+异常安全性2次）
   class SomeClassMayThrow
   {
       int val_;
   public:
       SomeClassMayThrow(int val) : val_{val} {}
       SomeClassMayThrow(const SomeClassMayThrow &another) : val_{ another.val_ }
       {
           static int i = 0;
           if (i++ == 2)
           {
               throw std::runtime_error{ "Test" };
           }
           std::println("Constructed.");
       }
       ~SomeClassMayThrow() { std::println("Dtor."); }
       auto GetVal() const noexcept { return val_; }
   };
   
   std::vector<SomeClassMayThrow> a;
   a.reserve(3);
   a.emplace_back(1);
   a.emplace_back(2);
   a.emplace_back(3);
   try
   {
       List<SomeClassMayThrow> l{ a.begin(), a.end() };
   }
   catch (const std::exception &ex)
   {
       std::println("{}", ex.what());
   }
   ```
   
   ```c++
   // 强异常安全性测试，把上面"i++ == 2"改成"i++ == 5"
   std::vector<SomeClassMayThrow> a;
   a.reserve(3);
   a.emplace_back(1);
   a.emplace_back(2);
   a.emplace_back(3);
   List<SomeClassMayThrow> l{ a.begin(), a.end() };
   List<SomeClassMayThrow> l2{ a.begin() + 1, a.end() };
   try
   {
       l2 = l;
   }
   catch (const std::exception &ex)
   {
       std::println("{}", ex.what());
       // 应当输出2, 3，即赋值失败对其无影响。
       for (auto it = l2.begin(); it != l2.end(); ++it)
           std::println("{}", it->GetVal());
   }
   ```
   
5. 假设一个类具有这样的不变量：它的学生名称数量和分数数量一致，即：

   ```c++
   class A
   {
   public:
       void AddAttribute(...);
   private:
       // Invariants: names.size() == scores.size();
       std::vector<std::string> names;
       std::vector<int> scores;
   };
   ```

   对下面的方法，`A`能够做到何种异常安全性的保证？

   ```c++
   void A::AddAttribute(const std::string& name, int score)
   {
       names.push_back("Student: " + name); // strong exception guarantee.
       scores.push_back(score); // strong exception gurantee.
       return;
   }
   ```
   
6. 使用Catch2将第4题的验证改写为单元测试，这可能需要把`SomeClassMayThrow`改为用成员控制何时抛出异常。你也可以自己添加更多的测试。

