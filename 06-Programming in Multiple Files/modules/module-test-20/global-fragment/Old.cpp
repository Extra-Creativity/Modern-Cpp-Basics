#include <iostream>
#include "Old.h"

void SomeOldLibFunc(std::uint32_t id)
{
    std::cout << "In header unit, id = " << id << '\n';
}