#include <print>
#include <ranges>
#include <string_view>
#include <vector>

struct A
{
    int a;
    auto operator<=>(const A &another) const
    {
        std::print("Hello?");
        return a <=> another.a;
    }
    auto operator==(const A &another) const
    {
        std::print("Hello2?");
        return a == another.a;
    }
};

int main()
{
    std::vector<A> source{ A{ 1 }, A{ 2 }, A{ 3 }, A{ 0 }, A{ 4 }, A{ 5 },
                           A{ 6 }, A{ 0 }, A{ 7 }, A{ 8 }, A{ 9 } };
    A delimiter{ 0 };
    std::ranges::lazy_split_view outer_view{ source, delimiter };
    std::ranges::split_view outer_view2{ source, delimiter };

    std::println("For lazy split....");
    auto it = outer_view.begin();
    std::println("No search when getting iterator...comparision begin at "
                 "iterating the result view.");
    for (auto m : *it)
        std::print("{} ", m.a);
    // 为什么*it（即lazy_split_view产生的view）只能是forward_range呢？因为它
    // 需要不断地判断是否等于delimiter，才能知道是否到达了end，因此它只能从前向后不断迭代。
    // 再次迭代*it，还是会重新判断一遍。

    std::println("For split....");
    auto it2 = outer_view2.begin();
    std::println("\nAfter searching...no comparision is needed.");
    for (auto m : *it2)
        std::print("{} ", m.a);
    // 为什么*it（即split_view产生的view）可以保持原来view的category呢？因为它
    // 早已判断好了end的位置。
    // 再次迭代*it2，不会重新判断了。

    // 例如对于random access range，split里面m完全知道了范围[begin,
    // end)，可以使用m[index]访问；而lazy_split_view
    // 不能确定end，因此没法提供random access
    // （否则无法知道m[index]是否超过了end，对吧？），只能边迭代边判断。
    return 0;
}