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

5. 阅读以下关于locale的代码，大概理解它的含义。我们并没有详细地讲解locale，因此用这段补充代码来更深入地进行理解。

   ```c++
   // 目的：认为汉字数字大写数字（即"壹贰叁肆伍陆柒捌玖拾佰仟萬"和"一二三四五六七八九十百千万"）也是数字，并支持它们之间进行相互转换。
   #include <algorithm>
   #include <array>
   #include <iostream>
   #include <locale>
   
   struct CtypeWithChineseNum : std::ctype<wchar_t>
   {
   private:
       using Base = std::ctype<wchar_t>;
       static const std::size_t s_digitNpos = 13;
       static inline const std::array<wchar_t, 13> s_upperChineseNumbers{
           L'壹', L'贰', L'叁', L'肆', L'伍', L'陆', L'柒',
           L'捌', L'玖', L'拾', L'佰', L'仟', L'萬'
       };
       static inline const std::array<wchar_t, 13> s_lowerChineseNumbers{
           L'一', L'二', L'三', L'四', L'五', L'六', L'七',
           L'八', L'九', L'十', L'百', L'千', L'万'
       };
   
   protected:
       static auto GetChineseLowerDigitIdx(wchar_t c)
       {
           return std::ranges::find(s_lowerChineseNumbers, c) -
                  s_lowerChineseNumbers.begin();
       }
   
       static auto GetChineseUpperDigitIdx(wchar_t c)
       {
           return std::ranges::find(s_upperChineseNumbers, c) -
                  s_upperChineseNumbers.begin();
       }
   
       static bool IsChineseDigit(wchar_t c)
       {
           return GetChineseLowerDigitIdx(c) != s_digitNpos ||
                  GetChineseUpperDigitIdx(c) != s_digitNpos;
       }
   
       bool do_is(mask m, char_type c) const override
       {
           if ((m & digit) && IsChineseDigit(c))
           {
               return true;
           }
   
           if ((m & upper) && GetChineseUpperDigitIdx(c) != s_digitNpos)
           {
               return true;
           }
   
           if ((m & lower) && GetChineseLowerDigitIdx(c) != s_digitNpos)
           {
               return true;
           }
   
           return Base::do_is(m, c);
       }
   
       char_type do_toupper(char_type c) const override
       {
           if (auto pos = GetChineseLowerDigitIdx(c); pos != s_digitNpos)
           {
               return s_upperChineseNumbers[pos];
           }
           return Base::do_toupper(c);
       }
   
       char_type do_tolower(char_type c) const override
       {
           if (auto pos = GetChineseUpperDigitIdx(c); pos != s_digitNpos)
           {
               return s_lowerChineseNumbers[pos];
           }
           return Base::do_tolower(c);
       }
   };
   
   int main()
   {
       std::locale chs{ std::locale("zh-CN") };
       std::cout << std::isupper(L'壹', chs) << ' ' << std::islower(L'一', chs)
                 << '\n'; // 输出什么？
       chs = std::locale{ chs, new CtypeWithChineseNum{} };
       std::cout << std::isupper(L'壹', chs) << ' ' << std::islower(L'一', chs)
           << '\n'; // 输出什么？
   
       std::wcout.imbue(chs);
       std::ranges::transform(
           L"壹捌玖捌", std::ostream_iterator<wchar_t, wchar_t>{ std::wcout },
           [](wchar_t c) {
               return std::use_facet<std::ctype<wchar_t>>(std::wcout.getloc())
                   .tolower(c);
           }); // 输出什么？
       return 0;
   }
   ```

6. 做一些格式化上的练习：

   + 以宽度为10的左对齐方式输出数字`10086`和`10085`，多余部分留空格。
   + 以二进制方式、八进制方式和十六进制方式输出数字`1898`，其中16进制带前缀并大写字母。
   + 对于`pi`，分别输出七位有效数字和小数点后七位。
   + 输出`123\n\t`的escaped形式。

7. 还记得我们在上一章实现的`List`吗？现在我们希望给它引入一个新的formatter，使它的元素可以按照`[a=>b=>c=>...]`的方式格式化。给`ConstIterator`类型加两个`public`的类型alias，如下：

   ```c++
   using value_type = T;
   using difference_type = std::ptrdiff_t;
   ```

   这样`ConstIterator`构成的迭代器对就满足了`input_range`的要求，从而可以利用`std::range_formatter`来帮你完成格式化过程（请查询range format的标准库实现情况，至少libc++16已经实现，可以在之前提供的docker上的Clang18上进行测试）。

8. 为以下类型实现一个`std::formatter`，不需要支持格式化符：

   ```c++
   struct Data
   {
       int a;
      	double b;
   };
   ```

   