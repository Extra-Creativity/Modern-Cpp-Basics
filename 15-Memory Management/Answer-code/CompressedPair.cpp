#include <utility>
#include <tuple>

template<typename FirstType, typename EmptySecondType>
class CompressedPair : public EmptySecondType
{
    FirstType first_;

    template<typename Tuple1, typename Tuple2, std::size_t... Indices1,
             std::size_t... Indices2>
    CompressedPair(Tuple1& tuple1, Tuple2& tuple2,
                   [[maybe_unused]] std::index_sequence<Indices1...> indices1,
                   [[maybe_unused]] std::index_sequence<Indices2...> indices2)
        : EmptySecondType(std::get<Indices2>(tuple2)...), 
          first_(std::get<Indices1>(tuple1)...)
    {
    }

public:
    CompressedPair() = default;

    template<typename T2, typename U2>
    CompressedPair(T2&& first, U2&& second) : EmptySecondType{ std::forward<U2>(second) }, 
        first_{ std::forward<T2>(first) }
    {
    }

    template<class... Args1, class... Args2>
    CompressedPair(std::piecewise_construct_t, std::tuple<Args1...> firstArgs,
                   std::tuple<Args2...> secondArgs)
        : CompressedPair{ firstArgs, secondArgs,
                          std::make_index_sequence<sizeof...(Args1)>(),
                          std::make_index_sequence<sizeof...(Args2)>() }
    {
    }

    auto&& First(this auto&& self) noexcept
    {
        return std::forward_like<decltype(self)>(self.first_);
    }

    auto&& Second(this auto&& self) noexcept
    {
        return std::forward_like<decltype(self)>((EmptySecondType&)self);
    }
};