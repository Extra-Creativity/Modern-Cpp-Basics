#include <print>
#include <tuple>
#include <type_traits>
#include <vector>

namespace Detail
{
template<typename T>
struct IsVector : std::false_type
{
};

template<typename T>
struct IsVector<std::vector<T>> : std::true_type
{
};

} // namespace Detail

template<typename... Ts, std::size_t... Indices>
void CheckedPushBackGuard(std::tuple<Ts...> args,
                          std::index_sequence<Indices...> _)
{
    std::size_t pushedIdx = 0;
    auto checkOne = [&pushedIdx, &args]<std::size_t Idx>() {
        auto &vec = std::get<Idx * 2>(args);
        auto &&arg = std::get<Idx * 2 + 1>(args);

        using VectorType = std::decay_t<decltype(vec)>;
        using ElemType = decltype(arg);

        // Message is possible only after C++26.
        static_assert(Detail::IsVector<VectorType>::value);
        static_assert(
            std::is_convertible_v<ElemType, std::iter_value_t<VectorType>>);

        vec.push_back(arg);
        std::println("Pushed {}(th) element.", Idx);
        pushedIdx = Idx;
    };

    auto popOne = [pushedIdx, &args]<std::size_t Idx>() {
        std::get<Idx * 2>(args).pop_back();
        std::println("Poped {}(th) element.", Idx);
        return Idx < pushedIdx;
    };

    try
    {
        // 实现成lambda，就要用这种很丑的调用方式。。所以也可以把这俩作为模板函数写到Detail去。
        // 注意不能checkOne<Indices>，因为lambda本身不是模板，而是operator()是模板。
        (checkOne.template operator()<Indices>(), ...);
    }
    catch (...)
    {
        // Short circuit to avoid unnecessary pop.
        (popOne.template operator()<Indices>() && ...);
        throw;
    }
}

template<typename... Ts>
void PushBackGuard(Ts &&...args)
{
    constexpr auto size = sizeof...(args);
    static_assert(size % 2 == 0,
                  "This function should be called with v1, e1, v2, e2..., so "
                  "element number is times of 2.");
    CheckedPushBackGuard(std::tuple<Ts &&...>{ std::forward<Ts>(args)... },
                         std::make_index_sequence<size / 2>());
}

class SomeClassMayThrow
{
    int val_;

public:
    SomeClassMayThrow(int val) : val_{ val } {}
    SomeClassMayThrow(const SomeClassMayThrow &another) : val_{ another.val_ }
    {
        throw std::runtime_error{ "Test" };
        std::println("Constructed.");
    }
    ~SomeClassMayThrow() { std::println("Dtor."); }
    auto GetVal() const noexcept { return val_; }
};

int main()
{
    std::vector<SomeClassMayThrow> a;
    a.reserve(4);
    a.emplace_back(1);
    a.emplace_back(2);
    a.emplace_back(3);

    std::vector<int> v{ 1, 2, 3 };
    std::vector<double> v2{ 1, 2, 3 };
    try
    {
        // Here2应该只打印两次，v2不会被检查size。
        PushBackGuard(v, 4, a, SomeClassMayThrow{ 4 }, v2, 4.0);
        std::println("{}", v.at(3));
        std::println("{}", v2.at(3));
        std::println("{}", a.at(3).GetVal());
    }
    catch (const std::exception &ex)
    {
        std::println("{}", ex.what());
        std::println("{} {} {}", v.size(), v2.size(), a.size());
    }
    return 0;
}