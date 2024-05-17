#include <iostream>

template<typename T>
void NonStaticFunc() { std::cout << "Non static func.\n"; }

template<typename T>
static void StaticFunc() { std::cout << "Static func.\n"; }

template void NonStaticFunc<int>();
template void StaticFunc<int>();