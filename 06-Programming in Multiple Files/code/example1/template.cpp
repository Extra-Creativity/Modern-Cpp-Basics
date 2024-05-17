#include "template.h"

// define the template
template<typename T>
void Func(const T&){ /* ... */ }

// instantiate explicitly.
template void Func<int>(const int&);