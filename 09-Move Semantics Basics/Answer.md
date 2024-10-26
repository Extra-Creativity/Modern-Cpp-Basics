# Answer

1. 只需要改变一句：

   ```c++
   allRows.push_back(std::move(row));
   ```

   这是因为注意到代码中`row`的值在这一句之后是没有再使用过的，因此转移到`vector`中时没有必要拷贝进去再把`row`析构，而是直接进行移动。但如果是下面的代码：

   ```c++
   allRows.push_back(row);
   row += "blabla";
   // row又做了什么操作
   ```

   那么显然就不能移动`row`了，因为我们后面还会用`row`的值，所以存入`vector`需要一个新的副本。换言之，后面不再需要的资源才可以转移，否则需要拷贝。

2. 代码如下：

   ```c++
   template<typename T>
   void swap(T& a, T& b)
   {
       T temp{ std::move(a) };
       a = std::move(b);
       b = std::move(temp);
   }
   ```

   如果不用移动语义，就会出现三次拷贝，移动语义明显提升了性能。

3. 会输出10次`expensive delete`；改成逐成员交换就只输出1次。

   可以注意到，事实上除了第一次的move真的释放了`ptr[0]`的资源，剩下的`ExpensiveDelete`全部都是对空的（也即被移动过的）对象的调用。因此，我们可以改为：

   ```c++
   void ExpensiveDelete()
   {
       if (ptr == nullptr)
           return;
       std::println("expensive delete");
       delete ptr;
   }
   ```

   就可以消除这个调用。在gcc -O3尝试一下，可以发现这种改进后，`erase`生成的汇编和逐成员交换是一致的。因此，如果析构对于空对象也是十分昂贵的，我们需要考虑加上空状态的判断来手动跳过这个部分。