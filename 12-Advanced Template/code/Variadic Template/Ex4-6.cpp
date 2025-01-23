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
    A(T1 &&...args) : T1{ std::forward<T1>(args) }... {}
};

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