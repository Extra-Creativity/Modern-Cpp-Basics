#include <chrono>
#include <mutex>
#include <thread>
#include <print>
#include <random>

using namespace std::literals;

class MyMutex : public std::timed_mutex
{
public:
    void lock()
    {
        std::timed_mutex::lock();
        std::this_thread::sleep_for(1s);
    }
};

void WorkerDeadlock(int id, MyMutex& mut1, MyMutex& mut2)
{
    std::println("Begin: {}", id);
    std::lock_guard _1{ mut1 }, _2{ mut2 };
    std::println("End: {}", id);
}

float GetRandomNumber()
{
    thread_local std::uniform_real_distribution<float> distrbution;
    thread_local std::default_random_engine engine{ std::random_device{}() };

    return distrbution(engine);
}

void WorkerDeadlockRecovery(int id, MyMutex& mut1, MyMutex& mut2)
{
    std::println("Begin: {}", id);
    while (true)
    {
        std::unique_lock lock1{ mut1 };
        // Wait for a random time between 0 ~ 1s.
        std::unique_lock lock2{ mut2, GetRandomNumber() * 1s};
        if (lock2.owns_lock())
        {
            std::println("End: {}", id);
            break;
        }
        else {
            std::println("Release resources and try again: {}.", id);
            lock1.unlock();
            std::this_thread::sleep_for(GetRandomNumber() * 1s);
        }
    }
}

void WorkerDeadlockRecovery2(int id, MyMutex& mut1, MyMutex& mut2)
{
    std::println("Begin: {}", id);
    std::unique_lock lock1{ mut1, std::defer_lock }, 
                     lock2{ mut2, std::defer_lock };

    while (true)
    {
        lock1.lock();
        // Wait for a random time between 0 ~ 1s.
        if (lock2.try_lock_for(GetRandomNumber() * 1s))
        {
            std::println("End: {}", id);
            break;
        }
        else {
            std::println("Release resources and try again: {}.", id);
            lock1.unlock();
            std::this_thread::sleep_for(GetRandomNumber() * 1s);
        }
    }
}

int main()
{
    MyMutex mut1, mut2;
    // Pass them in reversed order
    std::jthread t1{ WorkerDeadlockRecovery2, 0, std::ref(mut1), std::ref(mut2) },
        t2{ WorkerDeadlockRecovery2, 1, std::ref(mut2), std::ref(mut1) };
    return 0;
}