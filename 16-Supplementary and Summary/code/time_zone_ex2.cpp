#include <chrono>
#include <iostream>
#include <print>

namespace stdc = std::chrono;

int main()
{
    using namespace std::literals;
    std::string abbrev{ "GMT+8" };

    auto day = stdc::sys_days{ 2021y / 1 / 1 };
    auto& db = stdc::get_tzdb();
    std::cout << db.version << " " << stdc::remote_version() << "\n";

    // print time and name of all timezones with abbrev:
    std::cout << day << " UTC maps to these '" << abbrev << "' entries:\n";
    // iterate over all timezone entries:
    for (const auto& z : db.zones) {
        if (z.get_info(day).abbrev == abbrev) {
            stdc::zoned_time zt{ &z, day };
            std::cout << "  " << zt << "  " << z.name() << '\n';
        }
    }
    std::cout << "And these links:\n";
    for (const auto& link : db.links) {
        if (link.name() == abbrev) {
            stdc::zoned_time zt{ link.target(), day };
            std::cout << "  " << zt << "  " << link.target() << '\n';
        }
    }
    return 0;
}