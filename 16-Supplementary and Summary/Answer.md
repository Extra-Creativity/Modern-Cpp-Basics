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

1. 见[std::filesystem::directory_iterator::directory_iterator - cppreference.com](https://cppreference.com/w/cpp/filesystem/directory_iterator/directory_iterator.html)，作为error处理。

2. 见`Answer-code/Tree.cpp`。注意几点：

   + 在msvc下使用`/utf-8`进行编译，因为我们使用了Unicode字符，所以希望`std::print`可以使用UTF-8 ACP去打印。

   + 有些人可能希望利用`recursive_directory_iterator`，这时候为了检查当前entry是否为最后一个，可能需要这么写：

     ```c++
     auto nextItAtSameLevel = it;
     nextItAtSameLevel.disable_recursion_pending();
     ++nextItAtSameLevel;
     
     bool endCurrLevel = (nextItAtSameLevel.depth() == currDepth);
     ```

     但是`(recursive_)directory_iterator`是input iterator，也就不保证multi-pass（即拷贝迭代器，不能保证拷贝的解引用与原解引用一致）。实际上，它们的实现通常是`shared_ptr<Impl>`，即所有的拷贝操纵同一个对象，因此`disable_recursion_pending`后会直接影响`it`。

## Chrono

### Part 1

1. 因为`operator+=`的参数是和自己相同的`duration`类型；同时不能从`std::duration<Float>`构造`std::duration<Int>`，构造函数会去除这个overload，所以编译错误。

### Part 2

1. ```c++
   stdc::days GetDiffDays(const stdc::year_month_day& from, 
                          const stdc::year_month_day& to)
   {
       return stdc::sys_days{ to } - stdc::sys_days{ from };
   }
   ```

   当然，你也可以定义为模板函数，这样对于`year_month_weekday`等也可以混合计算。后面的题目也类似。

2. ```c++
   stdc::year_month_day GetNextMonday(const stdc::year_month_day& date)
   {
       auto currWeekday = stdc::weekday{ date };
       auto diffDays = stdc::Monday - currWeekday;
       if (diffDays == 0)
           diffDays = 7;
       // sys_days进行运算，然后构造回year_month_day
       return stdc::sys_days{ date } + diffDays;
   }
   ```

   注意：当今天是周二时，`diffDays`为6（由于weekday运算的取mod性质），此时仍然是下一个周一。虽然月份有类似的性质，但是各个类型直接提供了`months`的`operator+`，所以不需要像上面一样搞个新函数。

3. ```c++
   void OutputSchedule(stdc::year year)
   {
       for (int i = 0; i < 12; i++)
       {
           auto ymd = year / i / 31;
           if (!ymd.ok())
               ymd = stdc::sys_days{ ymd };
           std::cout << "We'll meet on " << ymd << "\n";
       }
   }
   ```

   由于`day`可以存储0~255之间的数值，所以在转换到time point时会自动考虑超过的数值，再转换回来就变成了合法的日期。

## Math

1. 不严格正确，这个问题比较复杂。但假设`random_device`是真随机的，下面的代码是正确的：

   ```c++
   if (std::random_device{}() < 20)
       SendUserReport();
   ```
   
   之前的代码会产生比所需频率更低的用户报告，因为<20的有些数没有出现，而跑到了其他的数字上。

   对于理想随机序列，实际上这种方法就等价于生成 $2^{32}$ 个数字（在 $[0,2^{32}-1]$ 之内），其中前$K$个数字出现的次数统计。抽到这 $K$ 个数中任意一个的概率为 $K/2^{32}$，则符合以下二项分布：
   $$
   X\sim B(N=2^{32},p=K/2^{32})
   $$
   二项分布的期望是 $K$，但同样也存在着 $B(1-B/2^{32})$ 的方差。也就是说，我们并不能保证这 $K$ 个数字恰好出现 $K$ 次。具体地，由于我们这里 $K = 20$，远远小于 $2^{32}$，因此这个计算可以直接由泊松分布 $\text{Po}(20)$ 来近似，于是可以得到只有约9%的概率出现恰好20次。
   
   由于随机数算法是确定性算法，因此我们得到的是固定的$2^{32}$个序列（相当于进行了抽样）。很不巧，在这样的抽样下，它确实出现了不足20次，因此，这种统计方法并不能正好给出$20/2^{32}$的用户报告。这并不是因为算法本身有问题，而是给定一个抽样，出现了某一种情况罢了。
   
   > 实际应用中，你也可以使用“用户id尾号为XXXX”的方式来进行采集，这样就基本可以保证频率了。