#include <utility>

// Ex 4 - 6
template<class... Args>
int h(Args... args)
{
    return sizeof...(Args);
}

template<class... Args>
void g(Args... args)
{
    print(h(args) + args...);
    print(h(args...) + args...);
}

template<typename... T1>
class A : public T1...
{
public:
    // 注意不能用T1&&，因为不是universal reference.
    template<typename... T2>
    A(T2 &&...args) : T1{ std::forward<T2>(args) }...
    {
    }
};

template<typename... T2>
A(T2...) -> A<T2...>;

class B
{
};
class C
{
};

int main()
{
    A a{ B{}, C{} };
}