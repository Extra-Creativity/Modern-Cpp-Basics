# Answer

1. 有以下几种可能的实现：

   ```c++
   // Impl 1，ranges版本
   std::string ReplaceAllSubstr(std::string_view str, std::string_view substr,
                                std::string_view newStr)
   {
       return str | std::views::split(substr) | std::views::join_with(newStr) |
           std::ranges::to<std::string>();
   }
   
   // Impl 2，这个是和ranges的表意是一样的，最基本的实现方式。
   std::string ReplaceAllSubstr(std::string_view str, std::string_view substr,
                                std::string_view init_newStr)
   {
       std::string retVal, newStr{ init_newStr };
       std::size_t lastPos = 0, pos;
       // pos总是指向下一个match的开头，lastPos指向上一个match的end。
       while ((pos = str.find(substr, lastPos)) != std::string::npos)
       {
           // 如果P2591进入C++26，直接+newStr就行了。
           retVal += std::string{ str.substr(lastPos, pos - lastPos) } + newStr;
           lastPos = pos + substr.size();
       }
       // 别忘了把最后一段加上去。
       return retVal + std::string{ str.substr(lastPos) };
   }
   
   // Impl 3，对matching的部分进行优化，因为.find使用的是蛮力匹配，最坏复杂度是O(NM)。
   std::string ReplaceAllSubstr(std::string_view str, std::string_view substr,
                                std::string_view newStr)
   {
       std::boyer_moore_searcher searcher{ substr.begin(), substr.end() };
       std::string result;
       auto beginIter = str.begin(), endIter = str.end();
       while (true)
       {
           auto searchResult = searcher(beginIter, endIter);
           result.append(beginIter, searchResult.first);
           if (searchResult.first == str.end())
               break;
           beginIter = searchResult.second;
           result.append(newStr);
       }
       return result;
   }
   ```

   不能直接使用`std::ranges::replace(rng, ...)`，因为它查询并替换的是`rng`的元素类型（即`char`），而不是它的集合的比较（即字符串）。

   在MS-STL/MSVC(Release mode)的环境下的10K长度的文段下进行测试，如下：

   ```text
   benchmark name                       samples       iterations    est run time
                                        mean          low mean      high mean
                                        std dev       low std dev   high std dev
   -------------------------------------------------------------------------------
   test                                           100             1    33.0804 ms 
                                           326.588 us    324.712 us    328.979 us
                                           10.7403 us    8.55758 us    14.9615 us
   
   test2                                          100             4     2.6012 ms 
                                           6.68575 us    6.52975 us    7.38225 us
                                           1.42628 us    193.971 ns    3.36398 us
   
   test3                                          100             4      2.304 ms 
                                            5.6695 us    5.60175 us    5.84175 us
                                           514.085 ns    252.066 ns    1.06011 us
   ```

   在libstdc++/gcc(-O3)的环境下测试（docker），如下：

   ```text
   benchmark name                       samples       iterations    est run time
                                        mean          low mean      high mean
                                        std dev       low std dev   high std dev
   -------------------------------------------------------------------------------
   test                                           100             1    97.3245 ms
                                           969.302 us    960.969 us    980.166 us
                                           48.3523 us    39.5531 us    59.2359 us
   
   test2                                          100             2      3.861 ms
                                           19.5875 us     19.256 us    20.5414 us
                                           2.62736 us    1.11556 us    5.64231 us
   
   test3                                          100             4     4.2392 ms
                                           10.8068 us    10.2295 us    12.2654 us
                                            4.1026 us    355.575 ns    7.46029 us
   ```

   可以看到，ranges版本虽然写起来简单而优雅，但是优化非常差；而后两种就比较快了。brute-force和BM算法有些不分伯仲的意味，因为我们的文段比较随机，而且pattern比较短，不会让brute-force大大退化。

   > 想必有同学是类似这么实现的：
   >
   > ```c++
   > std::string ReplaceAllSubstr(std::string str, std::string_view substr,
   >                              std::string_view newStr)
   > {
   >     std::size_t pos = 0;
   >     while ((pos = str.find(substr, pos)) != std::string::npos)
   >     {
   >         str.replace(pos, substr.size(), newStr);
   >     }
   >     return str;
   > }
   > ```
   >
   > 想一想，这个和我们手动一段一段append字符串相比，有什么劣势（不考虑参数类型变化为`std::string`）？这可能需要你思考一下`.replace`的实现机理。我们在答案最后会给大家答案。
   >
   > 如果你实际测试，你可能又会发现这种方法比我们前面的方法快，这又是为什么呢？思考一下原因，想想我们没有控制好什么变量。

2. 这道题是带大家回顾一下运算符的性质；在function call中，C++17开始只保证各个参数的evaluate是不会交错的，即每个参数的表达式树完全eval后才会进行其他参数的eval，**但是并没有规定哪个参数先eval**。因此，既可能`end`先被eval从而输出`0`，也可能`stoi`先eval使得`end`变为`5`。

   第一个输出是确定的，就是`0x1234`即`4660`。别忘了`stoi`相关的API会忽略掉起始的空白字符。

3. ```c++
   constexpr long double operator"" _ToRad(long double angle) noexcept
   {
       return angle * std::numbers::pi_v<long double> / 180.0;
   }
   ```

4. + `std::string_view`的类型足够小，它本身又携带了一个指针，传递它的引用类似于传递二级指针。不如直接值类型传递，还有可能放到寄存器里。
   + `(const std::string_view&, std::size_t&)`会出现指针aliasing而关闭潜在的优化机会，因为`std::size_t&`有可能引用`std::string_view`的`size`；而传递值类型则没问题，因为它的`std::size_t`是拷贝来的，肯定不会是引用所引用的对象。

   特别地，在MSVC（x86-64）中，结构体被规定不能放入寄存器（不管它多么小），必须放入栈里，结构体参数会被翻译成传递指针；而且MSVC在alias上的优化相当保守，使得传递什么都是一样的。前者在MSVC（ARM）上就没问题了，后者则在任意平台都有问题。

   大多数情况下MSVC的这种表现不是严重问题，因此就统一用`std::string_view`就行了。如果你极端地注意性能，例如某段代码甚至要在x86-64汇编级别进行优化了，那么可以使用`const char* ptr, std::size_t len`来使用两个参数进行传递，从而可以放到寄存器上。

   > Credit：[A footnote on "Three reasons to pass `std::string_view` by value" – Arthur O'Dwyer – Stuff mostly about C++](https://quuxplusone.github.io/blog/2021/11/19/string-view-by-value-ps/)

5. 输出如下：换了facet之后才能检测出来。

   ```text
   0 0
   1 1
   一八九八
   ```

6. ```c++
   std::format("{:<10} {:<10}", 10086, 10085); // 也可以是": <10"，但是默认就是空格，所以可以不写
   std::format("{0:b} {0:o} {0:#X}", 1898); // 1898是北大诞生的年份...
   std::format("{0:.7} {0:.7f}", std::numbers::pi);
   std::format("{:?}", "123\n\t");
   ```

7. 有了`std::range_formatter`后定制起来非常简单：

   ```c++
   template<typename T> struct std::formatter<List<T>> : std::range_formatter<T>
   {
       constexpr formatter() { this->set_separator("=>"); }
   };
   ```

8. ```c++
   template<> struct std::formatter<Data>
   {
       constexpr auto parse(std::format_parse_context& context)
       {
           auto it = context.begin();
           if (it != context.end() && *it != '}')
               throw std::format_error{ "Don't support specifiers currently." };
           return it;
       }
   
       auto format(const Data& data, auto& context) const
       {
           return std::format_to(context.out(), "{} {}", data.a, data.b);
       }
   };
   ```

   如果想要支持格式化符，需要加上“分隔符”来分隔成员的格式化，然后可以想想怎么方便地实现。

9. 对`istream_iterator`，输出`123456789`，对`istreambuf_iterator`原封不动输出。不妨考虑如下代码：

   ```c++
   char c;
   while (true) { std::cin >> c; }
   ```

   此时，输入会自动跳过空格、换行等；这是受到IO manipulator控制的，如果我们加上：

   ```c++
   str >> std::noskipws;
   ```

   就不会有这个问题了。`streambuf_iterator`是调用它的`sgetc`的，不受到IO manipulator的影响。如果你忘了，不妨回看一下我们overview的整体架构图。

10. + 只换用binary mode不能解决问题，因为filebuf无论如何都要经过`codecvt`再写回文件。

       + 因为昊字的Unicode是`U+660A`（在UTF-16和UTF-32中都可以直接表示下，因此保持这个编码，即得到`0x660A`）。我们又注意到，`0A`在ASCII中是换行（`\n`）的表示，而我们是以text mode打开的文件，Windows上会转换为`\r\n`，凭空多出来一个字符，于是得到了非法的UTF-16序列。

         解决这个问题的方式也很简单，使用binary mode即可，即：

         ```c++
         std::wofstream fout{ "test.txt", std::ios::binary };
         ```
       
    + GBK中不存在这个问题，是因为GBK刻意避开了`0A`、`1F`这些相关Windows特殊识别的编码，因此可以直接输出。对于一般的编码规则，还是需要binary mode的。
    
    总结一下：
    
       + 已经确定了编码（比如已经用外部库转换好了），直接`fstream` + binary mode输入输出就完了
       + 如果直接使用locale想进行编码的转换，就要利用`wfstream` + locale + binary mode
       + 如果使用locale想进行编码的转换，但目标编码能保证不覆盖当前系统在文本模式下特殊处理的字符，那不用binary mode也可以。


11. ```c++
    std::ifstream fin{ "test.txt" };
    fin.exceptions(std::ios::badbit | std::ios::failbit); // 只允许eof，其他抛异常。
    std::ostringstream str;
    fin >> str.rdbuf();
    std::string_view fileContentView = str.view();
    ```
    
    这种方式由于一次性读入，理论上来说比其他方式要更快。
    
    > 我们还有一个`operator>>(Func)`没有提到，即可以自定义I/O manipulator作用在流上，感兴趣者可以自行搜索。
    
12. 代码如下：

    ```c++
    std::string s0{ "1.234 567 " };
    std::spanstream s{ s0 };
    float a; int b;
    s >> a >> b;
    std::println("a = {}, b = {}", a, b);
    s << b;
    std::println("get pos = {}, put pos = {}", s.tellg() - std::streampos{0}, 
                 s.tellp() - std::streampos{0});
    
    auto span = s.span(); // 这个本质上就是原来的{ s0.data(), s0.data() + s0.size() };
    std::println("original string = {},\nunderlying string = {}", s0, 
                 std::string_view{ span.data(), span.data() + span.size() });
    ```

    前面的输出不变，最后两行的输出变为：

    ```text
    original string = 5674 567,
    underlying string = 567
    ```

    小明的问题是最后`stringstream`的输入到达了结尾（因为输入数字会在非数字处才停止，而没有左后一个空格作为非数字，就到达了末尾），于是设置了`eofbit`，此时流不再`good`，需要`clear`才可以正常输出。

### 附加题

1. 对于`Send/Recv`，可以如下解决：

   ```c++
   while (true)
   {
       int bufferSize = std::min<std::streamsize>(size,
                                                  std::numeric_limits<int>::max());
       // 然后使用bufferSize
   }
   ```

   小明说：我记得C++20开始signed integer overflow好像良定义了，我直接`static_cast<int>`不行吗？注意到事实上signed integer overflow是`mod 2^n`的结果，因此有可能`mod`后填入`int`直接变为负数了；就算是`unsigned int`也有问题，因为有可能很大的数`mod`之后变成0。

   对于`gbump/pbump`，也可以类似地写一个循环。

   > 小哲是这么写的：
   >
   > ```c++
   > int bufferSize = std::min(size, std::numeric_limits<int>::max());
   > ```
   >
   > 发现编译不过，一怒之下瞎写了一个：
   >
   > ```c++
   > int bufferSize = std::min<int>(size, std::numeric_limits<int>::max());
   > ```
   >
   > 这下编译过了，想想有没有什么问题？和我们上面写的有什么区别？

2. 没有意义，因为`sync`的作用是从external device重新load内容，这样当external device的内容实际改变时，能够得到最新内容。然而，网络的内容发送或接受后就不能反悔了，没有所谓“最新”。

   这只是针对我们当前的情况；小刘想实现一个基于FTP协议的流，那么情况完全不同了。虽然这仍然基于网络，但是external device变为了另一方的文件，文件自然可以反悔式修改，此时`sync`的同步就有意义了。

## Final

第一题的思考题的答案：`.replace`的实现就是：如果新字串更长就向后移动原字串后面的部分，更短就向前移动，因此`.replace`的复杂度是$O(k)$的，$k$是剩余长度。在上述循环下，于是最差的复杂度会退化到$O(N^2)$，我们事实上做了很多不必要的字符串尾巴的移动。

在libstdc++/gcc下，却测得了如下的结果：

```text
test                                           100             1    95.8712 ms
                                        972.987 us    960.765 us    1.00945 ms
                                        98.9104 us     40.274 us    216.125 us

test2                                          100             2     3.8242 ms
                                        19.5749 us    19.4459 us    19.8198 us
                                        879.553 ns    544.642 ns    1.44718 us

test3                                          100             4     4.1924 ms
                                        10.4349 us    10.3545 us    10.5845 us
                                        546.095 ns    348.388 ns    894.885 ns

test4                                          100             7     3.6645 ms
                                        5.15629 us    5.10353 us    5.26113 us
                                        364.804 ns     225.34 ns    640.348 ns
```

`replace`反而是最快的，这个原因是我们传递进来了一个`string`作为参数，它从原`string`拷贝过来，于是具有很大的`capacity`，因此后续大大减少了reallocation的次数。如果每个函数的返回值都加上`.reserve(str.size())`，并加长文段到20K，得到的测试结果如下：

```text
test                                           100             1    193.617 ms
                                        1.94382 ms    1.93322 ms    1.95829 ms
                                        62.6952 us    49.1574 us    82.7921 us

test2                                          100             1     3.7055 ms
                                         36.405 us    36.2081 us     36.775 us
                                        1.34082 us    833.808 ns    2.03363 us

test3                                          100             2     4.1356 ms
                                        20.5508 us    20.3272 us    21.3085 us
                                        1.84857 us    596.039 ns    4.10043 us

test4                                          100             4     4.6808 ms
                                         12.053 us    11.5945 us    13.2012 us
                                        3.44302 us     1.6102 us    6.70891 us
```

这次就比较符合预期了。特别可以注意到，`ranges`版本的时间几乎翻倍了，这更说明了优化的问题；而第二个和第三个方法表现比较好，文段长度的增加没有显著增加时间开销。
