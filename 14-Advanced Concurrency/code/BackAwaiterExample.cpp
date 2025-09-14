#include <coroutine>
#include <stdexcept>
#include <utility>
#include <print>
#include <string>

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
    } {
    }
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

    struct BackAwaiter
    {
        std::string& convertedVal;

        bool await_ready() const noexcept { return false; }
        void await_suspend(CoroHandle handle) const noexcept {}
        std::string&& await_resume() noexcept { return std::move(convertedVal); }
    };

    struct promise_type
    {
        int val;
        std::string convertedVal;

        BackAwaiter yield_value(int init_val) {
            val = init_val;
            convertedVal.clear();
            return BackAwaiter{ convertedVal };
        }

        Task get_return_object() { return Task{ CoroHandle::from_promise(*this) }; }
        std::suspend_always initial_suspend() { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
        std::suspend_always final_suspend() noexcept { return {}; }
    };

    int GetInteger() const noexcept { return coroHandle_.promise().val; }
    void SetString(std::string newVal) { 
        coroHandle_.promise().convertedVal = std::move(newVal);
    }
};

Task Test()
{
    std::string r1 = co_yield 42;
    std::println("Get string value: {}", r1);
    std::string r2 = co_yield 442;
    std::println("Get string value: {}", r2);
}

int main()
{
    auto task = Test();
    while (task.Resume())
    {
        auto convertedStr = std::to_string(task.GetInteger());
        task.SetString(std::move(convertedStr));
    }
}