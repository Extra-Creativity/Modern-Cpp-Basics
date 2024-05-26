#include <charconv>
#include <cmath>
#include <expected>
#include <print>
#include <string_view>

std::expected<double, std::errc> parse_number(std::string_view &str)
{
    double result;
    auto begin = str.data();
    auto [end, ec] = std::from_chars(begin, begin + str.size(), result);

    if (ec != std::errc{})
        return std::unexpected{ ec };
    if (std::isinf(result)) // we regard inf as out of range too.
        return std::unexpected{ std::errc::result_out_of_range };

    str.remove_prefix(end - begin);
    return result;
}

int main()
{
    auto process = [](std::string_view str) {
        std::print("str: {:?}, ", str);
        parse_number(str)
            .transform([](double val) {
                std::println("value: {}", val);
                return val;
            })
            .transform_error([](std::errc err) {
                if (err == std::errc::invalid_argument)
                    std::println("error: invalid input");
                else if (err == std::errc::result_out_of_range)
                    std::println("error: overflow");
                return err;
            });
    };

    for (auto src : { "42", "42abc", "meow", "inf" })
        process(src);
    return 0;
}
