# Assignment

1. 对你之前写过的某个类做声明与定义的分离，分别写到头文件和源文件里。别忘了Header guard。

2. 对于下面的例子：

   ```c++
   // a.h
   #pragma once
   template<typename T> void Func();
   
   // a.cpp
   #include "a.h"
   #include <print>
   
   template<typename T>
   void Func() { std::println("Hello."); }
   static void test() { Func<int>(); }
   
   // main.cpp
   #include "a.h"
   int main()
   {
       Func<int>();
       return 0;
   }
   ```

   从理论上来说能够通过编译吗？为什么？实际情况下有没有其他可能？

3. 显式实例化有时可以用来减少实例化次数，缩短编译时间。例如：

   ```c++
   // a.tpp
   template<typename T>
   void Func() { std::println("Hello."); }
   
   // a.h
   template<typename T> void Func();
   
   // a.cpp
   template void Func<int>();
   ```

   在若干个源文件中，如果都`#include "a.tpp"`并使用模板会发生什么？如果有的文件只使用了`Func<int>`，可以改成`#include "a.h"`，为什么要这么做？