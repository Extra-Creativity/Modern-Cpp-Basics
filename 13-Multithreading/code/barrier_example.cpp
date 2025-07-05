#include <barrier>
#include <thread>
#include <ranges>
#include <vector>
#include <print>

auto FormatLine(auto&& view)
{
    return view | std::views::transform([](auto num) {
              return std::format("{}", num);
           }) | std::views::join_with('\t')
              | std::ranges::to<std::string>();
}

std::vector<int> results(9);
std::barrier barrier{ 9, []() noexcept {
    std::println("{}\t{}", results.size(), FormatLine(results));
    results.resize(results.size() - 1);
} };

void Work(int i)
{
    for (int j = 9; ; j--)
    {
        results[i - 1] = i * j;
        if (j == i)
            break;
        barrier.arrive_and_wait();
    }
    barrier.arrive_and_drop();
}


int main()
{
    std::println("\t{}", FormatLine(std::views::iota(1, 10)));
    int threadNum = 9;
    std::vector<std::jthread> threads;

    for (int i = 1; i <= threadNum; ++i)
        threads.emplace_back(Work, i);
    return 0;
}