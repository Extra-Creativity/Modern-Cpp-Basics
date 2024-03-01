#include <format>
#include <print>
#include <ranges>
#include <vector>

int main()
{
    std::vector v{ 2, 4, 3, 5 };
    for (auto elem : v | std::views::filter([](auto num) {
                         return num % 2 == 1;
                     }) | std::views::transform([](int a) { return a * 2; }))
        std::println("{}", elem); // 6,10
    return 0;
}