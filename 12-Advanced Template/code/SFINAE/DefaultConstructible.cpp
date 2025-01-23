#include <type_traits>

template<typename T>
struct IsDefaultConstructible
{
private:
    template<typename U, typename = std::void_t<decltype(U())>>
    char test(){};
    template<typename>
    long test(...);

public:
    static constexpr bool value = std::is_same_v<decltype(test<T>()), char>;
};

template<typename, typename = void>
struct IsDefaultConstructible2 : std::false_type
{
};

template<typename T>
struct IsDefaultConstructible2<T, std::void_t<decltype(T())>> : std::true_type
{
};

class A
{
public:
    A(int);
};

static_assert(IsDefaultConstructible<int>::value);
static_assert(IsDefaultConstructible2<int>::value);
static_assert(!IsDefaultConstructible<A>::value);
static_assert(!IsDefaultConstructible2<A>::value);

int main() {}