#include <list>
#include <print>
#include <ranges>
#include <vector>

namespace stdr = std::ranges;
namespace stdv = std::views;

int main()
{
    std::vector v{ 1, 2, 3 };
    std::list l{ 4, 5, 6, 7 };

    for (auto [ele1, ele2] : stdv::zip(v, l))
    {
        std::println("{} {} {}", ele1, ele2, ele1 + ele2);
        ele1 = ele2; // vector本身的元素也被修改
    }

    for (auto [ele1, ele2] : stdv::zip(v, l))
    { // 验证
        std::println("{} {} {}", ele1, ele2, ele1 + ele2);
    }

    return 0;
}