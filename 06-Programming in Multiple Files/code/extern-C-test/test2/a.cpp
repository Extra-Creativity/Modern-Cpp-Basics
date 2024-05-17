#include <iostream>
#include "a.hpp"

extern "C" int a = 1;
extern "C" void Func() 
{ 
    std::cout << "a = " << a << '\n';
    std::cout << "Hello, world!\n";
}