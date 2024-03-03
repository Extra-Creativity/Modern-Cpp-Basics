#include <functional>
#include <print>

struct B
{
    int b;
    void test(int a)
    {
        b = 3;
        std::println("{}, {}", a, b);
    }
};

int main()
{
    B b{ 1 };
    std::function<void(B *, int)> testPtr1 = &B::test; // Okay
    testPtr1(&b, 2);
    std::println("{}", b.b);

    b.b = 1;
    std::function<void(B &, int)> testPtr2 = &B::test; // Okay
    testPtr2(b, 3);
    std::println("{}", b.b);

    b.b = 1;
    std::function<void(B, int)> testPtr3 = &B::test; // Still okay
    testPtr3(b, 4); // not change the original b.
    std::println("{}", b.b);

    return 0;
}