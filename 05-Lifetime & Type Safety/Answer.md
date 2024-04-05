# Answer

2. 有，在`SomeFunc`调用时构造了一个临时变量，传递给参数`vec`。**在函数调用结束后，临时变量析构，因此`a`变为悬垂引用**。

   > 你可能会问，`const&`不是会延长生命周期吗？一定要注意，只有返回临时变量才会延长，返回引用并不会。

   `functional`的代码类似，把成员的`&`去掉就行。

   如果你觉得还没明白，可以看看下面的一大堆例子：

   ```c++
   #include <vector>
   
   // UB, vec是临时变量，它的data()在离开作用域后直接悬垂
   const int *Func0(std::vector<int> vec) { return vec.data(); }
   // UB, vec是临时变量，悬垂引用
   const std::vector<int> &Func1(std::vector<int> vec) { return vec; }
   
   // 有可能UB，如果vec是临时变量，那么本语句过后它就会析构，悬垂。
   const int *Func2(const std::vector<int> &vec) { return vec.data(); }
   // 和上一个同理，悬垂指针变悬垂引用。
   const std::vector<int> &Func3(const std::vector<int> &vec) { return vec; }
   
   std::vector<int> Func4(const std::vector<int>& vec) { return vec; }
   
   int main()
   {
       // 后面我们说UB还是OK都是指继续使用结果进行其他操作，比如访问元素。
       std::vector<int> a;
       auto b1 = Func0(a); // ub
       auto c1 = Func1(a); // ub
       auto d1 = Func2(a); // ok
       auto e1 = Func3(a); // ok
   
       auto b2 = Func0({1, 2, 3}); // ub
       auto c2 = Func1({1, 2, 3}); // ok, because auto deduces std::vector
       // which copies and makes a new variable.
       auto d2 = Func2({1, 2, 3}); // ub
       auto e2 = Func3({1, 2, 3}); // ok
   
       const auto& c3 = Func1({1, 2, 3}); // ub, const auto& refer to 
       // the destructed local vec.
       const auto& c4 = Func1(a); // ub, same as before.
       const auto& e3 = Func3(a); // ok
       const auto& e4 = Func3({1, 2, 3}); // ub, const auto& refer to the 
       // destructed temporary passed to the function.
   
       auto f1 = Func4(a); // ok, copy to f1
       auto f2 = Func4({1, 2, 3}); // ok, copy to f1
       const auto& f3 = Func4(a); // ok, const auto& will extend lifetime
       // for returned value type (i.e. not reference)
       const auto& f4 = Func4({1, 2, 3}); // ok, same as above.
   
       return 0;
   }
   ```

3. ```c++
   #include <cstddef>
   #include <print>
   
   template<typename T, std::size_t N>
   class InplaceVector
   {
   public:
       InplaceVector() = default;
   
       T* Data() { return reinterpret_cast<T*>(buffer_); }
       const T* Data() const { return reinterpret_cast<const T*>(buffer_); }
   
       std::size_t Size() const { return size_; }
       void PushBack(const T& elem)
       {
           auto newPos = Data() + size_;
           new(newPos) T{ elem };
           size_++;
       }
   
       void PopBack()
       {
           auto currPos = Data() + (size_ - 1);
           currPos->~T();
           size_--;
       }
   
       T& operator[](std::size_t idx) { return *(Data() + idx); }
       const T& operator[](std::size_t idx) const { return *(Data() + idx); }
   
       ~InplaceVector()
       {
           for (auto ptr = Data(), end = ptr + size_; ptr < end; ptr++)
           {
               ptr->~T();
           }
       }
   private:
       alignas(T) std::byte buffer_[sizeof(T) * N];
       std::size_t size_ = 0;
   };
   
   int main()
   {
       InplaceVector<int, 5> a;
       a.PushBack(1);
       a.PushBack(2);
       a.PushBack(3);
       a.PopBack();
       a.PushBack(4);
       for (std::size_t i = 0; i < a.Size(); i++)
           std::println("{}", a[i]);
       return 0;
   }
   ```

   你可以试验一个在析构函数中进行打印的类作为模板参数，检查是否所有的元素都合适地析构了。

   > 从理论上说，原来的`buffer`是一个旧指针，在placement new之后要想获得正确的`T*`头元素指针需要如下代码：
   >
   > ```c++
   > T* Data() { return std::launder(reinterpret_cast<T*>(buffer)); }
   > ```
   >
   > 或者你额外存储一个当前的尾指针，这样用尾指针向前访问；**但是实际上来说编译器一般不会在这里做优化，因为一般只分析到alias的地步，所以不加`std::launder`也没事，也不用多存那么一个指针。**
   >
   > 如果你实在实在讨厌可能的UB，你可以考虑`reinterpret_cast`头指针但是不使用它进行访问，这样也能不用`std::launder`就规避上述问题：
   >
   > ```c++
   > template<typename T, std::size_t N>
   > class InplaceVector
   > {
   > public:
   >     InplaceVector() = default;
   > 
   >     T* Data() { return endPtr_ - Size(); }
   >     const T* Data() const { return endPtr_ - Size(); }
   > 
   >     std::size_t Size() const { return endPtr_ - reinterpret_cast<T*>(buffer_); }
   >     
   >     void PushBack(const T& elem)
   >     {
   >         new(endPtr_) T{ elem };
   >         endPtr_++;
   >     }
   > 
   >     void PopBack()
   >     {
   >         --endPtr_;
   >         endPtr_->~T();
   >     }
   > 
   >     T& operator[](std::size_t idx) { return *(Data() + idx); }
   >     const T& operator[](std::size_t idx) const { return *(Data() + idx); }
   > 
   >     ~InplaceVector()
   >     {
   >         for (auto ptr = Data(); ptr < endPtr_; ptr++)
   >         {
   >             ptr->~T();
   >         }
   >     }
   > private:
   >     alignas(T) std::byte buffer_[sizeof(T) * N];
   > 	T* endPtr_;
   > };
   > ```

4. `a1`的成员重用合法，其他不合法。

5. 合法，因为创建`std::byte` array隐式地创建了对象，我们也在网络中写入了这个buffer使得object进行了suitable created并开启了生命周期，直接`reinterpret_cast`就可以正常访问。