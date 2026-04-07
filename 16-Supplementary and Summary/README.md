# Assignment

## Discussion

有几点对视频中问题的讨论，比较小的问题直接在视频评论中置顶了，比较大的问题在这里讨论一下。

1. 关于`path::iterator`是否应该是`bidirectional_iterator`的问题：我们已经谈到，libstdc++会对`path`进行eager split，而libc++ / MS-STL则会进行lazy split（通过stash完成）。

   但实际上，libc++的`operator*`返回的是`path`（值类型），同时在libc++ 14开始将`iterator_concept`设置为`bidirectional_iterator`（从而可以使用相关constrained algorithm），而MS-STL返回的是`const path&`，同时仍然保持`input_iterator`。

   这是因为stash的行为会破坏标准对`forward_iterator`的[这一条](https://eel.is/c++draft/iterator.concepts#iterator.concept.forward-3)：

   > Pointers and references obtained from a forward iterator into a range [i, s) shall remain valid while [i, s) continues to denote a range.

   变相地要求返回的引用是持久的，不会随着迭代器的变化而导致引用的失效；而如果将值stash在iterator中，同时返回引用，那么下次++就会导致引用失效，从而违反这一规定。

   那么为什么会有这一规定呢？我们不妨考虑一个简单的情况：`reverse_iterator`。虽然它只对`bidirectional_iterator`应用，但是已经足够说明问题；其`operator*`的实现为：

   ```c++
   constexpr decltype(auto) operator*() const
   {
       return *std::prev(current); // <== returns the content of prev
   }
   ```

   于是我们返回了一个临时迭代器的解引用结果；函数返回后，这个迭代器被析构，那么cache的内容也会被析构，于是我们返回了一个悬垂引用。类似地，在许多适配器和算法中都可能会有这个问题，见ranges作者的讨论：[Validity of references obtained from out-of-lifetime iterators · Issue #156 · ericniebler/stl2](https://github.com/ericniebler/stl2/issues/156)。

2. 关于`path`的console输出问题：我们只说了它具有`operator<</>>`，但是具体能否在console输出还没有讨论过。事实上结合我们在String and Stream这一章讲的内容就可以推理出来，我们在这里重新讲述一遍。

   + 使用`cout`输出：

     ```c++
     std::cout << path; // 等价于std::cout << path.string();
     ```

     在Windows上，console的输出encoding是由console ACP决定的；而我们又说其`string`的表示会自动转成ACP，而console ACP和ACP默认是一致的，于是是正确的；在Linux上，一般默认是UTF-8的，而`string`不会进行转码，于是一般仍然是正确的。但是如果改变了console的encoding，那么上述代码就不再直接适配了，需要通过给cout添加locale等方式自己转换。

   + 使用`wcout`输出：

     ```c++
     std::wcout << path; // 等价于std::wcout << path.wstring();
     ```

     在Windows上不进行转码，于是会按照UTF-16进行输出；然而console ACP一般不是UTF-16，于是会造成错误；在Linux上同样不能直接输出UTF-32，仍然造成错误。

   + 使用`print`输出，since C++26：

     ```c++
     std::print("{}", path);
     ```

     implementation-defined behavior，目前还没有实现，但是大概率都可以直接输出成功。

   + 使用`print`输出，by `.string()`：

     ```c++
     std::print("{}", path.string());
     ```

     Linux未经过转码，且默认可以处理UTF-8，于是正确；对于Windows，如果使用了UTF-8的execution charset（严格说是ordinary literal encoding是UTF-8，此时print会使用ACP为UTF-8的console设置进行输出），且ACP不是UTF-8（从而`.string()`的编码不是UTF-8），则输出错误；如果没有使用UTF-8的execution charset，则默认仍然使用ACP作为console ACP，输出仍然是正确的。

3. `std::error_code`：由一个category和一个整数错误码组成，默认为`std::system_category()`和`0`（无错误）。在标准库中包含以下几类（全部包含在`<system_error>`中，以函数形式提供单例）：

   + `system_category`：平台相关的错误码，无直接的统一规范；
   + `generic_category`：一般性错误码，使用了POSIX的规范；这种错误码在C++中可以由`std::errc`这个枚举表示，见[std::errc - cppreference.com](https://cppreference.com/w/cpp/error/errc.html)；通过`make_error_code(errc)`就可以把`errc`转换为`error_code`。
   + `io_category`：I/O错误码，在`std::ios_base::failure`中进行使用（也即`.exception()`设置流后抛出的异常）；这种错误码在C++中可以由`std::io_errc`这个枚举表示，见[std::io_errc - cppreference.com](https://cppreference.com/w/cpp/io/io_errc.html)。默认只有一个枚举值，实现允许定义更多的枚举值。
   + `future_category`：`<future>`中的错误码，我们之前讲过了，由`std::future_errc`表示，包含 `broken_promise`，`future_already_retrieved`，`promise_already_satisfied`，`no_state`四个枚举值。

   后三者可以通过`make_error_code`将`xxx_errc`转为`std::error_code`并包含相应的category（也可以直接构造，这时会通过ADL调用`make_error_code`）。以上类型并不直接对用户可见，它们统一继承自`std::error_category`，其包含一系列虚方法，具体可以参见[std::error_category - cppreference.com](https://cppreference.com/w/cpp/error/error_category.html)。

   既然`system_category`是平台相关的，那么是不是我们就不能写出可移植的错误处理了呢？C++其实提供了一个`std::error_condition`来辅助完成这个事情；它同样只有一个category和一个整数错误码；但是当它与一个`std::error_code`判断相等时，会调用其category的虚方法`equivalent`：

   ```c++
   // 默认：看code的category是不是当前category，且condition是不是code的错误码，比较plain的比较。
   virtual bool equivalent(const std::error_code& code,
                           int condition) const noexcept;
   // 默认：调用this->default_error_condition(code)，判断是否与condition逐成员相等
   virtual bool equivalent(int code,
                           const std::error_condition& condition) const noexcept;
   ```

   > 注意，`std::error_code`的比较就是value和category分别比较，并不会使用虚方法，也就没有定制的空间。

   那么核心就在于`default_error_condition(int err) -> std::error_condition`是什么作用；在`std::system_category`中，规定：

   > If the argument `ev` is equal to `0`, the function returns `error_condition(0, generic_category())`. Otherwise, if `ev` corresponds to a POSIX `errno` value `posv`, the function returns `error_condition(posv, generic_category())`. Otherwise, the function returns `error_condition(ev, system_category())`. What constitutes correspondence for any given operating system is unspecified.

   也就是说，它需要在这个函数中尽可能地将平台相关的错误转为POSIX中的错误，并返回`std::error_condition{ POSIX_ERROR, std::generic_category() }`；如果POSIX中没有对应错误，再返回`std::system_category`。例如，我们想要跨平台地判断”文件夹不存在“时，可以使用下面的方式：

   ```c++
   std::error_code err;
   std::directory_iterator it{ root, err }
   if (err) // 如果发生错误
   {
       // 如果错误是POSIX中的no_such_file_or_directory
       if (err == std::error_condition{ std::errc::no_such_file_or_directory })
           std::cerr << "No such directory: " << args.root << "\n";
       return;
   }
   ```

   注意，你不能使用下面的代码：

   ```c++
   if (err == std::error_code{ std::errc::no_such_file_or_directory })
   ```

   因为windows赋的错误码整数不和上述整数一致，从而并不能得到相等的结果。

   当然如果你想用switch case，你也可以手动调用`err.category.default_error_condition()`，先判断一下结果的category是不是generic category，如果是，就调用`.value()`并转成`std::errc`来switch枚举就可以了。

   具体MS-STL的映射可见[STL/stl/src/syserror.cpp](https://github.com/microsoft/STL/blob/a690f8442de28e1bd1c461ad27279c591954482f/stl/src/syserror.cpp#L29)，filesystem中涉及的错误码可见[STL/stl/inc/xfilesystem_abi.h](https://github.com/microsoft/STL/blob/a690f8442de28e1bd1c461ad27279c591954482f/stl/inc/xfilesystem_abi.h#L25)；如果不需要判断这些东西，只想打印一句信息，可以直接使用`error_code`的`.message()`。

4. 本质上，`(recursive_)directory_iterator`的实现是`shared_ptr<Impl>`，所以拷贝iterator会和原来的iterator共享一个`Impl`，使得它们的操作并不独立。例如：

   ```c++
   std::recursive_iterator it{ "." };
   auto it2 = it;
   ++it; // 对Impl进行操作，it2于是也隐式地被++了
   ```

   这是两个iterator在实现上是input iterator而不是forward iterator的原因。

## Filesystem

### Part 1

1. 在Python中，我们常常使用`os.path.join`来拼接文件路径各个组分：

   ```python
   os.path.join("/home", "foo", "bar") # Result: /home/foo/bar
   ```

   在C++中完成一个类似的函数。

### Part 2

1. 查询cppreference，构造`directory_iterator`传入regular file时，得到的iterator是什么结果？

2. 完成`tree`指令，只需要支持`-L X`来使得只列举深度不超过`L`的所有文件；自行搜索具体`tree`的输出效果。

   > 注：你可能会用到以下几个Unicode字符：
   >
   > 1. `U+2500`: ─；
   > 2. `U+2514`: └；
   > 3. `U+251C`:├。

## Chrono

补注：`local_time`输出时要求等价于输出其构造的`sys_time`，而`sys_time`在表示是floating point且单位小于1天时不能输出，所以此时`local_time`也不能输出。

### Part 1

1. 解释下述代码为何编译失败：

   ```c++
   auto a = 1s;
   auto b = 1.0s;
   a += b;
   ```

### Part 2

我们在课上说可以自定义time zone，但是没有进行详细的说明，我们将相关的讲解放在了作业的最末尾。

1. 如何计算两个日期之间相差多少天？

   ```c++
   stdc::days GetDiffDays(const stdc::year_month_day& from, 
                          const stdc::year_month_day& to)
   {
       // TODO
   }
   ```

2. 对于给定日期，给出下一个周一的日期。

   ```c++
   stdc::year_month_day GetNextMonday(const stdc::year_month_day& date)
   {
       // TODO
   }
   ```

3. 小明想要在每月的31日安排会议；当该月没有31日时，按照超过的天数安排在下一个月开头。

   ```c++
   void OutputSchedule(stdc::year year)
   {
       // TODO
   }
   ```

---

关于自定义time zone：由于`zoned_time`提供了第二个模板参数，只是默认是`const time_zone*`，因此我们可以通过替换它来自定义。具体地，我们可能需要提供以下的函数（由于模板lazy instantiation的存在，不使用的功能是允许不提供的）：

1. 对`stdc::zoned_traits<TimeZonePtr>`的特化：可能需要自定义下面的函数

   + 若希望使用“默认时区”（即当构造函数缺少时区时使用的时区），此时需要自定义`static TimeZonePtr default_zone();`。
   + 若希望使用名字对时区进行定位，此时需要自定义`static TimeZonePtr locate_zone(std::string_view name)`。

   本质上`const time_zone*`的特化就是分别`stdc::locate_zone("UTC")`和`stdc::locate_zone(name)`。

2. 类本身需要定义的成员函数：本质上，`zoned_time`会存储一个`sys_time`，用来唯一地表征当前时间。

   + `zoned_time`构造函数：

     + 若只使用`sys_time`初始化的构造函数，则无需提供特殊函数。

     + 若需要使用`local_time`的构造函数，则需要提供`to_sys(stdc::local_time<Duration>) -> stdc::sys_time<XXX>`。如果需要使用`choose`参数，则也要提供对应的重载。

       > 当然使用`const&`也可以，总之能让参数传进去就行了。

   + `zoned_time`成员函数：
     + 若需使用`get_local_time`，则需要提供`to_local(const stdc::sys_time<Duration>&) -> stdc::local_time<XXX>`。
     + 若需使用`get_info`和formatter，则需要提供`get_info(const stdc::sys_time<Duration>&) -> stdc::sys_info`。

例如，我们假设需要定义一个`OffsetZone`，它并不具有一个具体的名字，只是记录偏移多少时间，此时我们可能需要定义下面的函数：

```c++
class OffsetZone
{
    std::chrono::minutes offset;  // UTC offset
public:
    explicit OffsetZone(std::chrono::minutes offs) : offset{offs} {}
    // 为了使用get_local_time
    template<typename Duration>
    auto to_local(stdc::sys_time<Duration> tp) const {
        using LT = stdc::local_time<std::common_type_t<Duration, stdc::minutes>>;
        return LT{ (tp + offset).time_since_epoch() };
    }
    // 为了可以从local_time进行构造
    template<typename Duration>
    auto to_sys(stdc::local_time<Duration> tp) const {
        // define helper type for system time:
        using ST = stdc::sys_time<std::common_type_t<Duration, stdc::minutes>>;
        // convert to system time:
        return ST{(tp - offset).time_since_epoch()};
    }
    // 为了可以使用get_info
    template<typename Duration>
    auto get_info(const stdc::sys_time<Duration>& tp) const {
        return stdc::sys_info{ .offset = offset, .abbrev = "OFFSET" };
    }
};
```

那么我们就可以写出下面的程序：

```c++
int main()
{
    using namespace std::literals;
    
    OffsetZone p3_45{3h + 45min};
    // convert now to timezone with offset:
    auto now = stdc::local_days{ 2021y / 1 / 2 } + 18h + 30min;
    stdc::zoned_time<decltype(now)::duration, OffsetZone*> zt{ &p3_45, now };
    
    std::cout << "UTC: " << zt.get_sys_time() << '\n';
    std::cout << "+3:45: " << zt.get_local_time() << '\n';
    std::cout << zt << '\n';
}
```

最后打印结果如下：

```text
UTC: 2021-01-02 14:45:00
+3:45: 2021-01-02 18:30:00
2021-01-02 18:30:00 OFFSET
```

你可以试试把`stdc::local_days`换成`stdc::sys_days`，看看打印结果有什么变化。