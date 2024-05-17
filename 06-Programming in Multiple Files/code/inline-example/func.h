#pragma once
#include <iostream>

inline void Hello()
{
    std::cout << "Hello, world!\n";
}

void Hello2();

struct A{
    static inline int a = 1;
};