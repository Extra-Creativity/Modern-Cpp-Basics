#include <type_traits>

// WRONG:
// template<typename T>
// struct TryUnsignedT
// {
//     using type =
//         std::conditional_t<std::is_integral_v<T> && !std::is_same_v<T, bool>,
//                            std::make_unsigned_t<T>, T>;
// };

template<typename T>
struct TryUnsignedT
{
    using type =
        std::conditional_t<std::is_integral_v<T> && !std::is_same_v<T, bool>,
                           std::make_unsigned<T>, std::type_identity<T>>::type;
};

using T = TryUnsignedT<float>::type;

int main() {}