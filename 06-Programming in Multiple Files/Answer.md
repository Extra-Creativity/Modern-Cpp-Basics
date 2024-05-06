# Answer

2. 理论上来说能通过编译，因为在`a.cpp`中调用了`Func<int>`，于是其object file也会生成相应的代码，在链接时就可以使`main.cpp`找到符号。这也称为“隐式实例化”。

   然而，由于编译器可以进行优化，例如在这里可以把`Func<int>`的函数体完全放入`test`，从而就可以抹除`Func<int>`的符号，因此不能保证通过编译。这就是为什么需要显式实例化。

3. 如果`#include "a.tpp"`，则每个源文件都会要实例化一份`Func<int>`，由链接器进行符号的合并；而如果`#include "a.h"`，则每个源文件都只会生成符号，仅在`a.cpp`中进行实例化，在链接时再找到相应的符号，进而缩短编译时间。

   在C++11开始加入了Explicit instantiation declaration（显式实例化声明），从而可以进行合并：

   ```c++
   // a.h
   template<typename T>
   void Func() { std::println("Hello."); }
   extern template void Func<int>(); // 显式实例化声明，声明常用的模板实例
   
   // a.cpp
   template void Func<int>();
   ```

   当`#include "a.h"`时，如果使用`Func<int>`，则不会进行实例化，而是链接时进行符号查找。但一个实体在以下情况下仍可能进行实例化：

   + 函数内联或者consteval语境中使用，因为它们不能拖延到链接。
   + 函数需要自动推断返回值类型，因为需要实例化才能确定这些类型。
   + 类模板和模板别名，因为它们不是通过链接确定的。

   应用这个特性的主要是函数模板（包括类中的模板方法）和我们将来要讲的变量模板。

