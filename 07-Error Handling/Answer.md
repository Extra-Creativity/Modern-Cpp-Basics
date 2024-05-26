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

   **总结：不要把`std::optional`作为函数参数**。

   在P2998里，`std::optional<T&>`与指针是比较类似的：

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
   std::optional<int&> dangling{ Func(); } // 编译通过，但是后续访问dangling是错误的。
   // 等价于：int* dangling = &(Func().value());
   ```

   这是因为`std::optional<int>`是临时的，自然指向它内部值的指针在其析构后也就悬垂了。