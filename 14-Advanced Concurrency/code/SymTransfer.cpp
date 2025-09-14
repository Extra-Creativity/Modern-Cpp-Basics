#include <coroutine>
#include <stdexcept>
#include <print>

class Task
{
public:
    struct promise_type;
private:
    using CoroHandle = std::coroutine_handle<promise_type>;
    CoroHandle coroHandle_;

    void Destroy_() noexcept { if (coroHandle_) coroHandle_.destroy(); }

public:
    Task(CoroHandle handle) : coroHandle_{ handle } {}
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&& another) noexcept : coroHandle_{
        std::exchange(another.coroHandle_, nullptr)
    } { }
    Task& operator=(Task&& another) noexcept {
        Destroy_();
        coroHandle_ = std::exchange(another.coroHandle_, nullptr);
        return *this;
    }
    ~Task() { Destroy_(); }

    struct promise_type
    {
        CoroHandle lastLevelHandle;

        Task get_return_object() { return Task{ CoroHandle::from_promise(*this) }; }
        std::suspend_always initial_suspend() const noexcept { return {}; }
        void return_void() const noexcept { }
        void unhandled_exception() const noexcept { std::terminate(); }
        auto final_suspend() noexcept;
    };

    struct Awaiter
    {
        CoroHandle handle;

        bool await_ready() const noexcept { return false; }
        auto await_suspend(CoroHandle lastLevelHandle)
        {
            handle.promise().lastLevelHandle = lastLevelHandle;
            return handle;
        }
        void await_resume() const noexcept {}
    };
    Awaiter operator co_await() && noexcept { 
        return { coroHandle_ };
    }
    void Start() { coroHandle_.resume(); }
};

auto Task::promise_type::final_suspend() noexcept
{
    struct FinalAwaiter
    {
        bool await_ready() noexcept { return false; }
        std::coroutine_handle<> await_suspend(CoroHandle handle) noexcept
        {
            auto lastLevelHandle = handle.promise().lastLevelHandle;
            if (lastLevelHandle)
                return lastLevelHandle;
            return std::noop_coroutine();
        }
        void await_resume() noexcept {}
    };

    return FinalAwaiter{};
}

Task completes_synchronously() { std::println("Here"); co_return; }

Task loop_synchronously(int count) {
    for (int i = 0; i < count; ++i) {
        co_await completes_synchronously();
    }
}

int main()
{
    auto task = loop_synchronously(5);
    task.Start();
    return 0;
}