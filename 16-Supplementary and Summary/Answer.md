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
