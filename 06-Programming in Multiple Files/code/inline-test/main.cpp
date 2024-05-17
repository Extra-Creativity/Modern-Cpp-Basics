#include "test.h"
#include "func.h"
#include <iostream>

int main()
{
    std::cout << "Var in main is " << var << "\n";
    Func();
    std::cout << "Var in main is " << var << "\n";
    return 0;
}