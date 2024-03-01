#include <map>
#include <print>
#include <ranges>
#include <vector>

namespace stdr = std::ranges;
namespace stdv = std::views;

int main()
{
    std::map<int, double> m{ { 1, 2.0 }, { 3, 4.0 } };
    // Nope: auto view = m | stdv::values; std::println("{}", view[1]);
    auto valueVec = m | stdv::values | stdr::to<std::vector>();
    std::println("{} ", valueVec[1]);
    return 0;
}