# Assignment

## Discussion

有几点对视频中问题的讨论，比较小的问题直接在视频评论中置顶了，比较大的问题在这里讨论一下。

1. 关于`path::iterator`是否应该是`bidirectional_iterator`的问题：我们已经谈到，libstdc++会对`path`进行eager split，而libc++ / MS-STL则会进行lazy split（通过stash完成）。

   但实际上，libc++的`operator*`返回的是`path`（值类型），同时在libc++ 14开始将`iterator_concept`设置为`bidirectional_iterator`（从而可以使用相关constrained algorithm），而MS-STL返回的是`const path&`，同时仍然保持`input_iterator`。

   这是因为stash的行为会破坏标准对`forward_iterator`的[这一条](https://eel.is/c++draft/iterator.concepts#iterator.concept.forward-3)：

   > Pointers and references obtained from a forward iterator into a range [i, s) shall remain valid while [i, s) continues to denote a range.

   变相地要求返回的引用是持久的，不会随着迭代器的变化而导致引用的失效；而如果将值stash在iterator中，同时返回引用，那么下次++就会导致引用失效，从而违反这一规定。

   那么为什么会有这一规定呢？我们不妨考虑一个简单的情况：`reverse_iterator`。虽然它只对`bidirectional_iterator`应用，但是已经足够说明问题；其`operator*`的实现为：

   ```c++
   constexpr decltype(auto) operator*() const
   {
       return *std::prev(current); // <== returns the content of prev
   }
   ```

   于是我们返回了一个临时迭代器的解引用结果；函数返回后，这个迭代器被析构，那么cache的内容也会被析构，于是我们返回了一个悬垂引用。类似地，在许多适配器和算法中都可能会有这个问题，见ranges作者的讨论：[Validity of references obtained from out-of-lifetime iterators · Issue #156 · ericniebler/stl2](https://github.com/ericniebler/stl2/issues/156)。

2. 关于`path`的console输出问题：我们只说了它具有`operator<</>>`，但是具体能否在console输出还没有讨论过。事实上结合我们在String and Stream这一章讲的内容就可以推理出来，我们在这里重新讲述一遍。

   + 使用`cout`输出：

     ```c++
     std::cout << path; // 等价于std::cout << path.string();
     ```

     在Windows上，console的输出encoding是由console ACP决定的；而我们又说其`string`的表示会自动转成ACP，而console ACP和ACP默认是一致的，于是是正确的；在Linux上，一般默认是UTF-8的，而`string`不会进行转码，于是一般仍然是正确的。但是如果改变了console的encoding，那么上述代码就不再直接适配了，需要通过给cout添加locale等方式自己转换。

   + 使用`wcout`输出：

     ```c++
     std::wcout << path; // 等价于std::wcout << path.wstring();
     ```

     在Windows上不进行转码，于是会按照UTF-16进行输出；然而console ACP一般不是UTF-16，于是会造成错误；在Linux上同样不能直接输出UTF-32，仍然造成错误。

   + 使用`print`输出，since C++26：

     ```c++
     std::print("{}", path);
     ```

     implementation-defined behavior，目前还没有实现，但是大概率都可以直接输出成功。

   + 使用`print`输出，by `.string()`：

     ```c++
     std::print("{}", path.string());
     ```

     Linux未经过转码，且默认可以处理UTF-8，于是正确；对于Windows，如果使用了UTF-8的execution charset（严格说是ordinary literal encoding是UTF-8，此时print会使用ACP为UTF-8的console设置进行输出），且ACP不是UTF-8（从而`.string()`的编码不是UTF-8），则输出错误；如果没有使用UTF-8的execution charset，则默认仍然使用ACP作为console ACP，输出仍然是正确的。

3. `std::error_code`：TODO

## Filesystem

### Part 1

1. 在Python中，我们常常使用`os.path.join`来拼接文件路径各个组分：

   ```python
   os.path.join("/home", "foo", "bar") # Result: /home/foo/bar
   ```

   在C++中完成一个类似的函数。

### Part 2

1. 完成`tree`指令，只需要支持`-L X`来使得只列举深度不超过`L`的所有文件；自行搜索具体`tree`的输出效果。

   > 注：你可能会用到以下几个Unicode字符：
   >
   > 1. `U+2500`: ─；
   > 2. `U+2514`: └；
   > 3. `U+251C`:├。