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

   在[P2998](https://github.com/cplusplus/papers/issues/1661)引入了`std::optional<T&>`的特化，我个人认为这个提案的通过阻力应该比较小，主要的分歧在于`value_or`到底返回`T&`还是`T`，大概能顺利进入C++26，在作业答案中我们会给出其已经确定的使用方法。