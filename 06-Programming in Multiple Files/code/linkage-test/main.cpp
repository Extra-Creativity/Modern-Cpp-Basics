#include "a.h"
#include <iostream>

template<typename T> void NonStaticFunc();
template<typename T> void StaticFunc();

int main()
{
    NonStaticFunc<int>();
    // StaticFunc<int>(); // Link-time error
    std::cout << "a = " << a << ", b = " << b << "\n";
    return 0;
}