#include <map>
#include <type_traits>
#include <unordered_map>

namespace Detail
{
template<typename Key, typename = void>
struct SupportLessThan : std::false_type
{
};

template<typename Key>
struct SupportLessThan<
    Key, std::void_t<decltype(std::declval<Key>() < std::declval<Key>())>>
    : std::true_type
{
};

template<typename Key, typename = void>
struct SupportHash : std::false_type
{
};

template<typename Key>
struct SupportHash<Key,
                   std::void_t<decltype(std::hash<Key>{}(std::declval<Key>()))>>
    : std::true_type
{
};

template<typename Key, typename Value>
using ProperDictionary =
    std::conditional_t<SupportLessThan<Key>::value, std::map<Key, Value>,
                       std::unordered_map<Key, Value>>;
} // namespace Detail

template<typename Key, typename Value, typename = void>
class Dictionary
{
};

template<typename Key, typename Value>
class Dictionary<Key, Value,
                 std::enable_if_t<Detail::SupportLessThan<Key>::value ||
                                  Detail::SupportHash<Key>::value>>
    : public Detail::ProperDictionary<Key, Value>
{
};

// template<typename Key, typename Value>
// class Dictionary<Key, Value,
//                  std::enable_if_t<Detail::SupportLessThan<Key>::value>>
//     : public std::map<Key, Value>
// {
// };

// template<typename Key, typename Value>
// class Dictionary<Key, Value,
//                  std::enable_if_t<Detail::SupportHash<Key>::value &&
//                                   !Detail::SupportLessThan<Key>::value>>
//     : public std::unordered_map<Key, Value>
// {
// };

class Integer
{
    int num_;

public:
    Integer(int n) : num_{ n } {}
    auto GetNum() const noexcept { return num_; }
    bool operator==() = default;
};

template<>
struct std::hash<Integer>
{
    auto operator()(Integer integer) const noexcept
    {
        return std::hash<int>{}(integer.GetNum());
    }
};

int main()
{
    Dictionary<int, int> map;
    map[1] = 2;

    Dictionary<Integer, int> umap;
    umap[1] = 2;
}