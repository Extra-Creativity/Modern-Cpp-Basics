#include <print>
#include <stop_token>
#include <thread>
#include <vector>

using namespace std::literals;

class ThreadGroup
{
    std::stop_source stopSource_;
    std::vector<std::thread> threads_;

    void Clean_()
    {
        stopSource_.request_stop();
        join_all();
    }

public:
    ThreadGroup() = default;
    ThreadGroup(const ThreadGroup &) = delete;
    ThreadGroup &operator=(const ThreadGroup &) = delete;
    ThreadGroup(ThreadGroup &&) noexcept = default;
    ThreadGroup &operator=(ThreadGroup &&another) noexcept
    {
        Clean_();
        stopSource_ = std::move(another.stopSource_);
        threads_ = std::move(another.threads_);
        return *this;
    }

    template<typename F, typename... Args>
    void push_back(F &&func, Args &&...args)
    {
        if constexpr (std::invocable<F, std::stop_token, Args...>)
        {
            threads_.emplace_back(std::forward<F>(func), get_token(),
                                  std::forward<Args>(args)...);
        }
        else
        {
            threads_.emplace_back(std::forward<F>(func),
                                  std::forward<Args>(args)...);
        }
    }

    void join_all()
    {
        for (auto &thread : threads_)
        {
            if (thread.joinable())
                thread.join();
        }
    }
    void join(std::size_t i) { threads_.at(i).join(); }

    void detach_all()
    {
        for (auto &thread : threads_)
        {
            if (thread.joinable())
                thread.detach();
        }
    }
    void detach(std::size_t i) { threads_.at(i).detach(); }

    auto get_source() const noexcept { return stopSource_; }
    auto get_token() const noexcept { return stopSource_.get_token(); }
    bool request_stop() noexcept { return stopSource_.request_stop(); }

    ~ThreadGroup() { Clean_(); }
};

void Test(std::stop_token token, int i)
{
    while (!token.stop_requested())
    {
        std::println("Hello, {}", i);
        std::this_thread::sleep_for(2ms);
    }
}

int main()
{
    ThreadGroup group;
    for (int i = 0; i < 10; i++)
        group.push_back(Test, i);
    std::this_thread::sleep_for(10ms);
    return 0;
}