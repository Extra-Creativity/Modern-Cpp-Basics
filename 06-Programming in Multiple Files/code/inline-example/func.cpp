#include "func.h"
#include <iostream>
// Then func.cpp and main.cpp will both have Hello() and A.

void Hello2() { std::cout << A::a++ << "\n"; Hello(); }