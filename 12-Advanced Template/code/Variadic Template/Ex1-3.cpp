#include "print.h"
#include <concepts>
#include <tuple>
#include <utility>

// Ex 1 - 3
template<typename F, typename... Args>
decltype(auto) Invoke(F func, Args &&...args)
{
    return func(std::forward<Args>(args)...);
}

template<typename T, typename... IdxTypes>
void print_elems(const T &container, IdxTypes... idx)
{
    print(container[idx]...);
}

template<typename... T1>
class A
{
public:
    template<typename... T2>
    static auto func()
    {
        return std::tuple<std::pair<T1, T2>...>();
    }
};

int main()
{
    using ExpectedType =
        std::tuple<std::pair<int, char>, std::pair<float, long>>;
    std::same_as<ExpectedType> auto result = A<int, float>::func<char, long>();
}