#include <chrono>
#include <print>
namespace stdc = std::chrono;

int main()
{
    using namespace std::literals;
    auto begin = stdc::local_days{ 2026y / 1 / stdc::Friday[1] }, 
        end = stdc::local_days{ 2026y / 12 / stdc::Friday[stdc::last] };
    auto zoneBeiJing = stdc::locate_zone("Asia/Shanghai"), 
        zoneLA = stdc::locate_zone("America/Los_Angeles");
    auto time = 18h + 30min;

    for (; begin <= end; begin += stdc::weeks{ 1 })
    {
        stdc::zoned_time timeBeiJing{ zoneBeiJing, begin + time },
            timeLA{ zoneLA, timeBeiJing };
        std::println("We'll meet at:\n{}\n{}", timeBeiJing, timeLA);
    }
}
