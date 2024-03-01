1. 例子出现位置在19:30 (example 01)、26:40 (example 02)、40:15 (example 04) 46:50 (example 05)、51:25 (example 06)

2. 16:40口误，不是做引用，而是做“修改”；auto&和const auto&都是指一个东西（对于不存在的序列本质上也可以用const auto&，不过引用的对象是临时对象，因此不能使用auto&）；后面说的被引用也是“通过引用修改”的含义。一个例子就可以说明区别：
    ```c++
    // 这里也可以使用const auto&，但是不能使用auto&，因为返回的是临时值
    for(auto ele1: stdv::iota(1, 10)) {}

    const std::vector v{1,2,3};
    // 这里既可以使用auto&，也可以使用const auto&，不过二者都是相同的const int&，因为面对的是非临时的只读对象
    for(auto ele1: v){ }
    ```

3. 38:40-39:40，所有“lazy_split_view对iterator解引用的时候”，都是指“对iterator解引用的view进行迭代的时候”，请见github的example 03

4. 44:05不是对list，是对vector保留cache