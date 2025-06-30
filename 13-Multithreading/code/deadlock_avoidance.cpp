#include <chrono>
#include <mutex>
#include <thread>
#include <print>

using namespace std::literals;

class MyMutex : public std::mutex
{
public:
    void lock()
    {
        std::this_thread::sleep_for(1s);
        std::mutex::lock();
    }
};

void WorkerDeadlock(int id, MyMutex& mut1, MyMutex& mut2)
{
    std::println("Begin: {}", id);
    std::lock_guard _1{ mut1 }, _2{ mut2 };
    std::println("End: {}", id);
}

void WorkerNoDeadlock(int id, MyMutex& mut1, MyMutex& mut2)
{
    std::println("Begin: {}", id);
    //std::lock(mut1, mut2);
    //std::lock_guard _1{ mut1, std::adopt_lock }, _2{ mut2, std::adopt_lock };
    std::scoped_lock _{ mut1, mut2 };
    std::println("End: {}", id);
}

int main()
{
    MyMutex mut1, mut2;
    // Pass them in reversed order
    std::jthread t1{ WorkerNoDeadlock, 0, std::ref(mut1), std::ref(mut2) },
        t2{ WorkerNoDeadlock, 1, std::ref(mut2), std::ref(mut1) };
    return 0;
}