#include <iostream>

template<typename T, typename... Args>
void print(T firstArg, const Args &...args)
{
    std::cout << firstArg << "\n";
    if constexpr (sizeof...(args) != 0)
    {
        print(args...);
    }
}
