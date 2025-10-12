#include <print>

class Base
{
public:
    static void* operator new(std::size_t byteCnt)
    {
        std::println("Base: Called overrided operator new, size={}", byteCnt);
        if (auto ptr = malloc(byteCnt); ptr)
            return ptr;
        throw std::bad_alloc{};
    }

    void operator delete(void* ptr) noexcept
    {
        std::println("Base: Called overrided operator delete, ptr={}", ptr);
        free(ptr);
    }

    void operator delete(void* ptr, std::size_t size) noexcept
    {
        std::println("Base: Called overrided operator delete, ptr={}, size={}",
                     ptr, size);
        free(ptr);
    }

    virtual ~Base() { std::println("Base dtor"); }
};

class Derived : public Base
{
public:
    static void* operator new(std::size_t byteCnt)
    {
        std::println("Derived: Called overrided operator new, size={}", byteCnt);
        if (auto ptr = malloc(byteCnt); ptr)
            return ptr;
        throw std::bad_alloc{};
    }

    void operator delete(void* ptr) noexcept
    {
        std::println("Derived: Called overrided operator delete, ptr={}", ptr);
        free(ptr);
    }

    void operator delete(void* ptr, std::size_t size) noexcept
    {
        std::println("Base: Called overrided operator delete, ptr={}, size={}",
            ptr, size);
        free(ptr);
    }

    virtual ~Derived() { std::println("Derived dtor"); }
};

int main()
{
    Base* ptr = new Derived;
    delete ptr;
    return 0;
}