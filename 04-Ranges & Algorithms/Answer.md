1.  
```c++
#include <array>
#include <ranges>

namespace stdr = std::ranges;
namespace stdv = std::views;

template<typename Cont>
Cont QuickSort(const Cont &cont)
{
    // 如果是空，返回空容器；否则...
    return stdr::empty(cont)
            ? cont
            : std::array<Cont, 3>{
                    QuickSort(cont | stdv::drop(1) | stdv::filter([&](const auto &elem) {
                        return elem < *stdr::begin(cont);
                    }) | stdr::to<Cont>()),
                    stdv::single(*stdr::begin(cont)) | stdr::to<Cont>(),
                    QuickSort(cont | stdv::drop(1) | stdv::filter([&](const auto &elem) {
                        return elem >= *stdr::begin(cont);
                    }) | stdr::to<Cont>()),
                } | stdv::join | stdr::to<Cont>();
}
```

2.
```c++
std::vector<std::string> SplitString(const std::string &str,
                                     const std::string &delim)
{
    // 或者lazy_split，在这道题里没有本质区别。
    return str | std::views::split(delim) | std::views::transform([](auto rng) {
               return rng | std::ranges::to<std::string>();
           }) |
           std::ranges::to<std::vector>();
}

int main()
{
    for (auto s : SplitString("a,b,c,d", ","))
        std::print("{}\n", s);
    return 0;
}
```

3. 
```c++
for(auto [idx, elem] : str | stdv::enumerate)
    elem = (elem - 'a' + idx) % 26 + 'a';
```
4. 
```c++
#include <experimental/generator>
#include <print>

std::experimental::generator<int> Fib(int num)
{
    int a = 0, b = 1;
    for (int i = 0; i < num; i++)
    {
        int temp = a;
        a = b;
        b += temp;
        co_yield a;
    }
}

int main()
{
    auto fib5 = Fib(5);
    for (auto elem : fib5)
        std::print("{} ", elem);
    return 0;
}
```