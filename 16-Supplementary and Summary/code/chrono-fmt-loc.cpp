#include <chrono>
#include <iostream>
#include <print>

namespace stdc = std::chrono;

// Better: use std::runtime_format since C++26.
auto FormatOptions(const auto& opts, const std::locale& loc, const auto& time)
{
    std::string fmtStr;
    for (auto& opt : opts)
        fmtStr += opt + ": {0:L" + opt + "}, ";
    return std::vformat(loc, fmtStr, std::make_format_args(time));
}

void OutputTime(const std::locale& loc)
{ // Assuming loc gives UTF-8.
    using namespace std::literals;
    std::vector<std::string> options{
        "%I", "%OI", "%r", "%p"
    };

    auto amTime = 5000s, pmTime = 50000s;
    std::println("Locale: {}", loc.name());
    std::println("AM {:%T}: {}", amTime, FormatOptions(options, loc, amTime));
    std::println("PM {:%T}: {}", pmTime, FormatOptions(options, loc, pmTime));
}

void OutputDate(const std::locale& loc)
{
    using namespace std::literals;
    stdc::sys_days date = 2011y / 9 / 25;

    std::vector<std::string> options{
        "%C", "%EC", "%y", "%Oy", "%Ey", "%Y", "%EY", "%b",
        "%h", "%B", "%m", "%Om", "%d", "%Od", "%e", "%Oe",
        "%a", "%A", "%u", "%Ou", "%w", "%Ow", "%g", "%G",
        "%V", "%OV", "%j", "%U", "%OU", "%W", "%OW", "%D",
        "%F", "%x", "%Ex", "%c", "%Ec"
    };

    std::println("Locale: {}\n{}", loc.name(), FormatOptions(options, loc, date));
}

int main()
{
    try
    { // Locale names on Windows; see our slides for Linux correspondance.
        OutputTime(std::locale::classic());
        OutputTime(std::locale{ "Chinese-Simplified.utf8" });
        OutputTime(std::locale{ "Japanese.utf8" });
        OutputTime(std::locale{ "el_GR.utf8" }); // Greek

        std::cout << "2011/9/25\n";
        OutputDate(std::locale::classic());
        OutputDate(std::locale{ "Chinese-Simplified.utf8" });
        OutputDate(std::locale{ "Japanese.utf8" });
        OutputDate(std::locale{ "el_GR.utf8" }); // Greek
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << "\n";
    }
}