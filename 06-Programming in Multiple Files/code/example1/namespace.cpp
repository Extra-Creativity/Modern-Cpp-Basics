#include "namespace.h"
#include <iostream>

namespace Test
{

int A::GetX() const noexcept { return x_; }

void Func(){ std::cout << "Wow.\n"; }

} // namespace Test

namespace Test2
{
namespace Test3
{

void B::Output() { std::cout << "Output B.\n"; };

} // namespace Test2::Test3
} // namespace Test2

namespace Test4::Test5{
void C::Output() { std::cout << "Output C.\n"; };
}

namespace Test6::Implv1{
void Func(){ std::cout << "This is v1.\n"; };
}

// clang will report warning, but this seems a wrong report.
namespace Test6::Implv2{
void Func(){ std::cout << "This is v2.\n"; };
}