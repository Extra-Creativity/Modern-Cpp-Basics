#include <concepts>
#include <iostream>

struct RGB
{
    int r, g, b;
};

template<decltype(&RGB::r) Channel>
void Test(RGB &color)
{
    color.*Channel = 0;
}

void Func(int p)
{
    std::cout << p;
}

template<decltype(&Func) Pointer>
void Test2(int param)
{
    Func(param);
}

template<std::invocable auto Callable>
class A
{
public:
    constexpr decltype(auto) operator()() { return Callable(); }
};

int main()
{
    RGB color{ 1, 2, 3 };
    Test<&RGB::g>(color);
    std::cout << color.g;

    Test2<&Func>(1);
    Test2<Func>(1); // Also correct;

    A<[]() { return 0; }> a;
    A<[]() { std::cout << "Hello, world"; }> b;
}
