#include "test.hpp"
#include <iostream>

int main()
{
    SomeComplexClass obj{ 1, 2.0f };
    std::cout << obj.Sum() << std::endl;
    return 0;
}