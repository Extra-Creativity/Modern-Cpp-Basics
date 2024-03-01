#include <map>
#include <print>
#include <ranges>

namespace stdr = std::ranges;
namespace stdv = std::views;

int main()
{
    std::map<int, double> m{ { 1, 2.0 }, { 3, 4.0 } };
    // 如果是auto&：由于map的key是const的，key的类型为const int&
    for (auto key : m | stdv::keys)
        std::print("{} ", m);
    return 0;
}