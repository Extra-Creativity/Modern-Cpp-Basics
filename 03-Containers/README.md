# 03-Containers

### 上半部分

1. 写一个模板函数，对第一个元素`*2`，并累加所有的元素：

   ```c++
   template<typename Range>
   void Function(Range& range)
   {
   }
   // 请测试以下情况：
   std::vector v{1,2,3}; Function(v);
   std::list l{1,2,3}; Function(l);
   int arr[]{1,2,3}; Function(arr); // 看到这里，你就应该明白不能使用.begin，而是使用什么？
   ```

2. 对`vector`做一些简单的API练习（本题中输出`vector`请包装为一个函数，并用`ostream_iterator`输出）：

   + 创建一个`std::vector v1`，它包含10个5（请不要真的写上去`{5,5,5,5,5...}`）。
   + 创建第二个`std::vector v2`，包含`{2,1,4}`。
   + 将`v2`反向插入到`v1`的第三个位置（即`5,5,4,1,2,5,5...`）。注意使用反向迭代器。
   + 移除从第5个元素开始的所有元素（即`5,5,4,1,2`）。
   + 移除所有奇数，请不要使用`O(n^2)`的方法。
   + 与`vector v{1,7}`做三路比较，打印结果。

3. 对`list`做一些简单的API练习：

   + 创建一个`std::list l1`，它包含10个5。
   + 创建第二个`std::list l2`，包含`{2,1,4}`。
   + 将`l2`融合到`l1`中，操作后`l2`变为空。
   + 对`l1`去除相邻的重复元素。
   + 对`l1`排序。

4. `deque`是否具有`.data`方法？为什么？

5. 回忆各个容器的迭代器失效问题。下面的代码有什么问题？

   ```c++
   auto it = vec.find(1);
   vec.insert(it - 1, 110);
   *it = 2;
   ```

   聪明的你发现了在前面插入有迭代器失效问题，那么下面的代码呢？

   ```c++
   auto it = vec.find(1);
   vec.insert(it + 1, 110);
   *it = 2;
   ```

   如果你没有思路，看看下面的代码是否合法：

   ```c++
   auto it = vec.find(1);
   if(it.capacity() > it.size())
   {
       vec.insert(it + 1, 110);
       *it = 2;
   }
   ```

6. 小明写了下面的代码，期望删除vector的最大值：

   ```c++
   std::vector vec{ 10,2,3,10,4 };
   auto it = std::ranges::max_element(vec);
   std::erase(vec, *it);
   for (auto& i : vec)
       std::print("{}", i);
   ```

   运行一下，发现并没有移除所有的`10`；这段代码有什么问题？特别提示，`std::erase`接受的是引用，你可以理解成一直在移除掉`*it`，请注意擦除过程中迭代器下面的内容是否发生了变化。

   解决方法就是拷贝一下，即`auto m = *it`，或者C++23里直接`std::erase(vec, auto{*it})`，又或者使用`std::ranges::max`，直接返回值类型而不是迭代器。

> 注意迭代器失效和引用失效的最重要方法就是看是否在获得迭代器到使用迭代器的过程中容器本身是否修改。