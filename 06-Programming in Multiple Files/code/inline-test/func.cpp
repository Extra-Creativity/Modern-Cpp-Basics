#include "func.h"
#include "test.h"
#include <iostream>

void Func()
{
    var++;
    std::cout << "Var in library is " << var << "\n";
    std::cout << "What??\n";

    // strip the comment, and recompile with:
    // xmake -b inline-dynamic-lib
    // then xmake run inline-dynamic-test (i.e. no recompilation).

    // for static library, xmake -b inline-static-lib
    // xmake run inline-static-test is still the old code.
}