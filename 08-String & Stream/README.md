# Assignment

1. 我们说过，`.replace`本身只会替换一部分字符串；请写一个API，可以全局地进行替换。规定替换是贪婪的、线性的，例如对于字串`aaaa`，将`aa`替换为`ba`得到的是`baba`，即匹配了`aa`后，进行替换，然后就可以跳过，不必重新检视新的string。

   ```c++
   std::string ReplaceAllSubstr(std::string_view str, 
                                std::string_view oldStr, std::string_view newStr)
   {
       /* 你的实现... */
   }
   ```

   如果你想出了多种实现，你可以用`Catch2`的`BENCHMARK`测试一下哪种更高效。

   > 想想能不能直接使用`<algorithm>`中的`std::ranges::replace`呢？为什么？

2. 下面一段代码输出什么？

   ```c++
   void OutputTest(int num, std::size_t val)
   {
       std::println("{} {}", num, val);
   }
   
   std::string str{ " 1234 567" };
   std::size_t end = 0;
   OutputTest(std::stoi(str, &end, 16), end);
   ```

3. 练一下用户定义字面量，写一个`_ToRad`，使得一个角度制的角度转为弧度制。$\pi$从C++20开始可以从`<numbers>`中通过`std::numbers::pi_v<FloatType>`获得，例如`std::numbers::pi_v<float>`得到`float`类型的$\pi$。

4. 为什么以值类型传递`std::string_view`，而不是以`const std::string_view&`？多考虑一些可能的原因。