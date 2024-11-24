# Assignment

1. 实现`exchange`，功能与`std::exchange`相同，注意利用移动语义。假设第二个参数是使用`const&`直接传递的（我们在下一节的作业中会改进这一点）。

2. 小明发现，构造函数和赋值函数都是具有两个重载的（`const& + &&`），而我们课上又说过可以用值类型来替换重载，那么可否可以进行如下的化简？说明理由，思考其中实际发生了什么。

   ```c++
   class A
   {
       A(A obj) { *this = std::move(obj); }
       A& operator=(A obj) { *this = std::move(obj); }
   };
   ```

   其中一句可以进行修改来解决问题，思考如何进行（提示: swap）。这时它等价于什么？