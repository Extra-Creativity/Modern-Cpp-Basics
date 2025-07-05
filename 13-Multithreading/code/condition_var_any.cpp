#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <print>
#include <shared_mutex>

#include <chrono>
#include <thread>
#include <vector>

using namespace std::literals;

std::condition_variable_any condVar;
std::shared_mutex mutex;
std::uint64_t tick = 0;

void Sleep(std::uint64_t tickNum)
{
    std::shared_lock lock{ mutex };
    auto wakeupTick = tick + tickNum;
    condVar.wait(lock, [wakeupTick]() {
        // Sleep until current tick has exceeded limit.
        return tick >= wakeupTick;
    });
}

void Work(int id, int loopNum, std::uint64_t sleepTick)
{
    for (int i = 0; i < loopNum; i++)
    {
        Sleep(sleepTick);
        std::println("Hello from {}", id);
    }
}

int main()
{
    std::vector<std::jthread> workers;
    workers.emplace_back(Work, 0, 10, 4);
    workers.emplace_back(Work, 1, 15, 3);
    workers.emplace_back(Work, 2, 7, 6);
    workers.emplace_back(Work, 3, 5, 8);

    for (int i = 0; i < 50; i++)
    {
        {
            std::lock_guard lock{ mutex };
            ++tick;
            std::println("Now it's tick {}", tick);
        }
        condVar.notify_all();
        std::this_thread::sleep_for(100ms);
    }
    return 0;
}