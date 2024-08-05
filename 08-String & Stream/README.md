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

9. 我们上课时提到了`istream_iterator`和`istreambuf_iterator`的本质不同；对下面的代码，当`Iter`分别为`std::istream_iterator<char>`和`std::istreambuf_iterator<char>`，请说明输出：

   ```c++
   #include <iostream>
   #include <fstream>
   #include <sstream>
   
   using Iter = xxx;
   
   int main()
   {
       std::stringstream str{ "123 456\n789" };
       std::string s;
       std::copy(Iter{ str }, Iter{}, std::back_inserter(s));
       std::cout << s;
       return 0;
   }
   ```

   这种区别的原因是什么？考虑它们分别实际调用的流函数进行说明。

   > 思考题：小明加了一个IO manipulator，就消除了上述问题，应该怎么做？

10. 考虑下面的代码，我们在课上写过：

    ```c++
    std::wofstream fout{ "test.txt" };
    fout << L"眼底未名水，胸中黄河月";
    ```

    但是运行后可以发现并没有输出，我们也说过这是因为默认的locale是global的，而后者默认又是C locale，它的`std::codecvt<wchar_t, char>`总是失败的（即不知道如何处理宽字符）。我们的解决方法是使用`std::locale{ "zh-CN" }`（在Linux/Mac上可能名字不同），于是就有了一个有效的`codecvt`，我们也可以成功转换并输出GBK编码。

    现在我们不妨构想一个新任务：我们希望把`wchar_t`原封不动地输出，即在Windows上是UTF16，在Linux/Mac上是UTF32，同时仍然使用`std::wofstream`。回答以下问题：

       + 直接换用binary mode，能否直接解决问题？
    
       + 小昊想，我们给一个新的`codecvt`不就好了？于是ta在cppreference上查找了相关资料，并写下了如下代码：
    
         ```c++
         class WcharFacet
             : public std::codecvt<wchar_t, char, std::char_traits<wchar_t>::state_type>
         {
             using Base =
                 std::codecvt<wchar_t, char, std::char_traits<wchar_t>::state_type>;
             using state_type = Base::state_type;
             using result = Base::result;
         
             result do_in(state_type &state, const char *from, const char *from_end,
                          const char *&from_next, wchar_t *to, wchar_t *to_limit,
                          wchar_t *&to_next) const override
             {
                 // 我们不妨不进行Unicode的合法性校验了。
                 std::size_t outSize = (to_limit - to) * sizeof(wchar_t),
                             inSize = from_end - from;
                 std::size_t copySize = std::min(inSize, outSize) / sizeof(wchar_t) *
                                        sizeof(wchar_t); // 算一下完整的wchar_t有多大
         
                 std::memcpy(to, from, copySize);
                 from_next = from + copySize, to_next = to + copySize / sizeof(wchar_t);
                 return copySize == inSize ? partial : ok;
             }
         
             result do_out(state_type &state, const wchar_t *from,
                           const wchar_t *from_end, const wchar_t *&from_next, char *to,
                           char *to_limit, char *&to_next) const override
             {
                 std::size_t outSize = to_limit - to,
                             inSize = (from_end - from) * sizeof(wchar_t);
                 std::size_t copySize =
                     std::min(inSize, outSize) / sizeof(wchar_t) * sizeof(wchar_t);
         
                 std::memcpy(to, from, copySize);
                 from_next = from + copySize / sizeof(wchar_t), to_next = to + copySize;
                 return copySize == inSize ? partial : ok;
             }
         };
         ```
         
         然后：
         
         ```c++
         std::wofstream fout{ "test.txt" };
         fout.imbue(std::locale{ fout.getloc(), new WcharFacet });
         // Unicode要写一下BOM
         if (std::endian::native == std::endian::little)
    	    fout.write(0xFEFF);
         else
             fout.write(0xFFFE);
         fout << L"眼底未名水，胸中黄河月";
         ```
         
         他发现正常地进行地输出了，并且切换编码后得到的也是正确的文字。ta于是高兴地输出了一个自己的名字“小昊”：
         
         ```c++
         fout << L"小昊";
         ```
    
         他发现在Windows中，文件里的内容又不正确了，这是为什么？查阅“昊”字的UTF-16编码，思考原因。为了解决这个问题，应当如何修改代码（注：不是修改上述`codecvt`的代码）？
         
       + 小昊问：为什么`std::locale{"zh-CN"}`没有这个问题？查阅GBK的编码规则进行回答。


11. 我们在课上没有讲`operator>>(std::basic_streambuf*)`的作用，只是提到它是Unformatted output。实际上，它的作用是将自己的`streambuf`内容输入到参数的`buf`中，直到：

    + 自己eof了；
    + 对参数的输出序列插入失败了；
    + 抛出了异常。

    请你利用这个函数，快速读取文件全部内容到内存中，得到对它的`string_view`。如果没有读到end of file，则抛出异常。

    提示：你可以同时利用file stream和string stream。

12. 对我们课上`stringstream`的例子，使用`spanstream`来进行改造，了解一些API的变化。

    ```c++
    std::string s0{ "1.234 567 " };
    std::stringstream s{ s0 };
    float a; int b;
    s >> a >> b;
    std::println("a = {}, b = {}", a, b);
    s << b;
    std::println("get pos = {}, put pos = {}", s.tellg() - std::streampos{0}, 
                 s.tellp() - std::streampos{0});
    std::println("original string = {},\nunderlying string = {}", s0, s.str());
    ```

    最后的输出又有何变化？

    > 小明没有拷贝上述代码，自己一点点抄了上去，但是对`s0`漏抄了最后的空格，于是他得到了下述的输出：
    >
    > ```text
    > a = 1.234, b = 567
    > get pos = -1, put pos = 0
    > original string = 1.234 567,
    > underlying string = 1.234 567
    > ```
    >
    > 他说：真奇怪，为什么underlying string没有改变呢？put的指针不是明明指在最前面吗？
    >
    > 请你回答这个问题。