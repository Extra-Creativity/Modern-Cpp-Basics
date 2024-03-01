#include <format>
#include <print>
#include <ranges>
#include <vector>

int main()
{
    std::vector v{ 2, 4, 3, 5 };
    auto rng = v | std::views::filter([](auto num) { return num % 2 == 1; });
    // 目前仅libc++支持C++23的整个range打印，其他编译器请用for(auto elem : rng)
    std::println("{}", rng); // 3, 5
    v[0] = 1, v[2] = 2;      // 序列变为{ 1, 4, 2, 5 }，期望输出1, 5
    std::println("{}", rng); // 2, 5 ???
    return 0;
}