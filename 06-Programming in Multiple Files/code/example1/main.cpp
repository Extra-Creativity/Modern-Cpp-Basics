#include "namespace.h"
#include "singleton.h"

int main()
{
    Test::A a{1};
    Test::Func();
    [[maybe_unused]] auto b = a.GetX();
    Test2::Test3::B::Output(); // Namespace::Class::StaticMethod(...);
    Test6::Func();
    [[maybe_unused]] auto singleton = Singleton::GetInstance();
    return 0;
}