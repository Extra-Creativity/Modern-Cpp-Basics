#include <algorithm>
#include <optional>
#include <print>
#include <string>

std::optional<std::size_t> FindInStr(const std::string &str, auto pred)
{
    // Of course, you can add std::reference_wrapper.
    auto pos = std::ranges::find_if(str, pred);
    return pos == str.end() ? std::nullopt : std::optional{ pos - str.begin() };
}

int main()
{
    std::string s{ "133" };

    auto pos = FindInStr(s, [](char ch) {
        return ch >= '0' && ch <= '9' && (ch - '0') % 2 == 0;
    });

    auto ch = pos.transform([&s](auto idx) {
                     std::println("The first occurence: {}", s[idx]);
                     return s[idx];
                 })
                  .or_else([]() {
                      std::println("Not find any occurence");
                      return std::optional{ '?' };
                  });

    std::println("Get char: {}", *ch);

    return 0;
}