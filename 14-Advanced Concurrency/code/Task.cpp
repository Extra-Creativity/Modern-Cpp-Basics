#include <coroutine>
#include <stdexcept>
#include <utility>

class Task
{
public:
    struct promise_type;
private:
    using CoroHandle = std::coroutine_handle<promise_type>;
    CoroHandle coroHandle_;

    void Destroy_() noexcept { if (coroHandle_) coroHandle_.destroy(); }

public:
    Task(CoroHandle handle) : coroHandle_{ handle } { }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&& another) noexcept : coroHandle_{ 
        std::exchange(another.coroHandle_, nullptr)
    } {}
    Task& operator=(Task&& another) noexcept {
        Destroy_();
        coroHandle_ = std::exchange(another.coroHandle_, nullptr);
        return *this;
    }
    ~Task() { Destroy_(); }

    bool Resume() {
        if (!coroHandle_ || coroHandle_.done())
            return false;

        coroHandle_.resume();
        return !coroHandle_.done();
    }

    struct promise_type
    {
        Task get_return_object() { return Task{ CoroHandle::from_promise(*this) }; }
        std::suspend_always initial_suspend() { return {}; }
        void return_void() { }
        void unhandled_exception() { std::terminate(); }
        std::suspend_always final_suspend() noexcept { return {}; }
    };
};