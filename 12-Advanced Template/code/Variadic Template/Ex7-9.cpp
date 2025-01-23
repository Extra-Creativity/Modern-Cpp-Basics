#include "print.h"
#include <string>
#include <variant>
#include <vector>

// Ex 7 - 9
template<typename... Funcs>
struct Overloaded : Funcs...
{
public:
    using Funcs::operator()...;
};

// Only needed in C++17.
template<typename... Args>
Overloaded(Args...) -> Overloaded<Args...>;

template<class... Args>
int h(Args... args)
{
    return sizeof...(Args);
}

template<class... Args>
void g(Args... args)
{
    print(h(args) + args...);
    print(h(args...) + args...);
}

template<typename T, std::size_t... Indices>
void Access(const T &container)
{
    container[Indices]...;
}

template<typename... Ts, typename T>
void Error(Ts... args, T arg);

int main()
{
    auto overloads = Overloaded{ [](int i) { return std::to_string(i); },
                                 [](const std::string &s) { return s; },
                                 [](auto &&val) { return std::string{}; } };
    std::variant<int, std::string> v{ 1 };
    std::visit(overloads, v);

    g(1, 2);

    std::vector<int> v2{ 1, 2, 3 };
    Access<1, 2>(v2);
}