#include <iterator>
#include <list>
#include <print>
#include <type_traits>
#include <vector>

namespace MyStd
{

template<typename Iterator>
constexpr bool IsRandomAccess = std::is_convertible_v<
    typename std::iterator_traits<Iterator>::iterator_category,
    std::random_access_iterator_tag>;

template<typename Iterator>
constexpr bool IsInput = std::is_convertible_v<
    typename std::iterator_traits<Iterator>::iterator_category,
    std::input_iterator_tag>;

template<typename Iterator, typename Distance>
std::enable_if_t<IsRandomAccess<Iterator>> advanceIterImpl(Iterator &x,
                                                           Distance n)
{
    std::println("Call random-access");
    x += n;
}

template<typename Iterator, typename Distance>
std::enable_if_t<(IsInput<Iterator> && !IsRandomAccess<Iterator>)>
advanceIterImpl(Iterator &x, Distance n)
{
    std::println("Call input");
    while (n > 0)
        ++x, --n;
}

template<typename Iterator, typename Distance>
void advance(Iterator &x, Distance n)
{
    advanceIterImpl(x, n);
}

} // namespace MyStd

int main()
{
    std::vector v{ 1, 2, 3 };
    auto it = v.begin();
    // 不加MyStd会导致ADL，非常烦人。。
    MyStd::advance(it, 3);
    std::println("Equiv: {}", it == v.end());

    std::list l{ 1, 2, 3 };
    auto it2 = l.begin();
    MyStd::advance(it2, 3);
    std::println("Equiv: {}", it2 == l.end());
}