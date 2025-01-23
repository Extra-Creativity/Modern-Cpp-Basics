#include <algorithm>
#include <array>
#include <print>
#include <string_view>

template<typename CharType, std::size_t N>
class BasicFixedString
{
    constexpr BasicFixedString() = default;

public:
    CharType str_[N]; // 必须是public的，否则不是structural type。

    // 能乐意写这么多constexpr的都是神人，我是真的写烦了。。
    constexpr BasicFixedString(const CharType (&arr)[N + 1])
    {
        std::copy(arr, arr + N, str_);
    }
    constexpr std::size_t size() const noexcept { return N; }
    constexpr const CharType *c_str() const noexcept { return str_; }
    constexpr const CharType *data() const noexcept { return str_; }
    constexpr CharType *data() noexcept { return str_; }
    // 正常来说begin也得写const + non-const两个版本（分别对应iterator和const
    // iterator），但是可以用deducing this简化。
    constexpr auto begin(this auto &&self) noexcept { return self.data(); }
    constexpr auto end(this auto &&self) noexcept { return self.data() + N; }
    constexpr std::basic_string_view<CharType> view() const noexcept
    {
        return { begin(), end() };
    }

    template<typename CharType0, std::size_t N1, std::size_t N2>
    friend constexpr BasicFixedString<CharType0, N1 + N2> operator+(
        const BasicFixedString<CharType0, N1> &,
        const BasicFixedString<CharType0, N2> &);
};

// 顺便加了一个N == 0的特化，数组大小为0是非法的（虽然gcc允许），但是
// std::array<T, 0>是合法的（因为标准库里进行了特化）。
template<typename CharType>
class BasicFixedString<CharType, 0>
{
public:
    std::array<CharType, 0> str_;

    constexpr BasicFixedString(const CharType (&arr)[1]) {}
    constexpr std::size_t size() const noexcept { return 0; }
    constexpr const CharType *c_str() const noexcept { return str_.data(); }
    constexpr auto data(this auto &&self) noexcept { return self.str_.data(); }
    constexpr auto begin(this auto &&self) noexcept { return self.data(); }
    constexpr auto end(this auto &&self) noexcept { return self.data(); }
    constexpr std::basic_string_view<CharType> view() const noexcept
    {
        return { begin(), end() };
    }

    template<typename CharType0, std::size_t N1, std::size_t N2>
    friend constexpr BasicFixedString<CharType0, N1 + N2> operator+(
        const BasicFixedString<CharType0, N1> &,
        const BasicFixedString<CharType0, N2> &);
};

template<typename CharType, std::size_t N>
BasicFixedString(const CharType (&arr)[N]) -> BasicFixedString<CharType, N - 1>;

template<typename CharType, std::size_t N1, std::size_t N2>
constexpr BasicFixedString<CharType, N1 + N2> operator+(
    const BasicFixedString<CharType, N1> &a,
    const BasicFixedString<CharType, N2> &b)
{
    BasicFixedString<CharType, N1 + N2> result;
    auto dst = result.data();
    std::ranges::copy(a.str_, dst);
    std::ranges::copy(b.str_, dst + N1);
    return result;
}

template<std::size_t N>
using FixedString = BasicFixedString<char, N>;

template<FixedString str>
void PrintStr()
{
    std::println("{}", str.view());
}

int main()
{
    PrintStr<"123">();
    // 注意a和b是相同类型的（size == 3），才能一起CTAD；否则要写两行。
    constexpr FixedString a{ "456" }, b{ "789" };
    PrintStr<a + b>();

    constexpr FixedString c{ "" };
    PrintStr<c>(); // 测试特化的N == 0。
    PrintStr<a + c>();

    return 0;
}