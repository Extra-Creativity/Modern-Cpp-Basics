#include <atomic>
#include <print>
#include <random>
#include <ranges>
#include <thread>

#include "ThreadSafeQueue1.hpp"

float GetRandomNumber()
{
    thread_local std::uniform_real_distribution<float> distrbution;
    thread_local std::default_random_engine engine{ std::random_device{}() };

    return distrbution(engine);
}

using namespace std::literals;

std::atomic<bool> start{ false };
std::atomic<std::size_t> failToPushTimes{ 0 };
std::atomic<std::size_t> failToPopTimes{ 0 };

void Producer(ThreadSafeQueue &queue, int id, int elemNum)
{
    start.wait(false);
    auto m = id * elemNum;
    for (int i = 0; i < elemNum; i++)
    {
        while (!queue.TryPush(m + i))
        {
            failToPushTimes++;
            std::this_thread::yield();
        }
        std::this_thread::sleep_for(GetRandomNumber() * 100ms);
    }
}

void Consumer(ThreadSafeQueue &queue, int elemNum, std::vector<int> &output)
{
    start.wait(false);
    std::vector<int> result(elemNum);
    for (int i = 0; i < elemNum; i++)
    {
        while (true)
        {
            if (auto elem = queue.TryPop())
            {
                result[i] = *elem;
                break;
            }
            failToPopTimes++;
            std::this_thread::yield();
        }
        std::this_thread::sleep_for(GetRandomNumber() * 100ms);
    }
    output = result;
}

int main()
{
    int producerElemNum = 9, consumerElemNum = 10;
    int producerNum = 5, consumerNum = 4;
    std::size_t queueSize = 8;

    if (auto remainingElemNum =
            producerElemNum * producerNum - consumerElemNum * consumerNum;
        remainingElemNum < 0 || remainingElemNum > queueSize)
    {
        std::println("Configuration is wrong.");
        return 0;
    }

    std::vector<std::vector<int>> results(consumerNum);

    ThreadSafeQueue queue(queueSize);
    {
        std::vector<std::jthread> threads;
        for (int i = 0; i < producerNum; i++)
            threads.push_back(
                std::jthread{ Producer, std::ref(queue), i, producerElemNum });
        for (int i = 0; i < consumerNum; i++)
            threads.push_back(std::jthread{ Consumer, std::ref(queue),
                                            consumerElemNum,
                                            std::ref(results[i]) });
        start.store(true);
        start.notify_all();
    }

    for (std::size_t i = 0; i < results.size(); i++)
        std::println("{}: {}", i, results[i]);

    results.push_back(queue.GetSnapshot());
    auto allElems = results | std::views::join | std::ranges::to<std::vector>();
    std::ranges::sort(allElems);
    if (std::ranges::equal(allElems,
                           std::views::iota(std::size_t{ 0 }, allElems.size())))
        std::println("Queue behavior correct.");
    else
        std::println("Queue behavior INCORRECT!");

    std::println("Fail to push: {}, to pop: {}", failToPushTimes.load(),
                 failToPopTimes.load());
    return 0;
}