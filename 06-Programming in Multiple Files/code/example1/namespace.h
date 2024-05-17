#pragma once
namespace Test
{

class A
{
public:
    A(int x): x_{x}{}
    int GetX() const noexcept;
private:
    int x_;
};

void Func();

} // namespace Test

namespace Test2
{
namespace Test3
{
class B { public: static void Output(); };

} // namespace Test2::Test3
} // namespace Test2

namespace Test4::Test5 // since C++17
{
class C { public: static void Output(); };
}

namespace Test6
{
namespace Implv1
{
void Func();
}

inline namespace Implv2
{
void Func();
}
}