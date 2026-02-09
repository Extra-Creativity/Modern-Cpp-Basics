# Answer

## Filesystem

### Part 1

1. ```c++
   template<typename... Args>
   stdfs::path Join(Args&&... args)
   {
       return (stdfs::path{} / ... / args);
   }
   ```

### Part 2

1. 见`Answer-code/Tree.cpp`。注意几点：

   + 在msvc下使用`/utf-8`进行编译，因为我们使用了Unicode字符，所以希望`std::print`可以使用UTF-8 ACP去打印。

   + 有些人可能希望利用`recursive_directory_iterator`，这时候为了检查当前entry是否为最后一个，可能需要这么写：

     ```c++
     auto nextItAtSameLevel = it;
     nextItAtSameLevel.disable_recursion_pending();
     ++nextItAtSameLevel;
     
     bool endCurrLevel = (nextItAtSameLevel.depth() == currDepth);
     ```

     但是`(recursive_)directory_iterator`是input iterator，也就不保证multi-pass（即拷贝迭代器，不能保证拷贝的解引用与原解引用一致）。实际上，它们的实现通常是`shared_ptr<Impl>`，即所有的拷贝操纵同一个对象，因此`disable_recursion_pending`后会直接影响`it`。