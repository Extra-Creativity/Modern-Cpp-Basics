#include <coroutine>
#include <stdexcept>
#include <print>

class TaskCont
{
public:
    struct promise_type;
private:
    using CoroHandle = std::coroutine_handle<promise_type>;
    CoroHandle coroHandle_;

    void Destroy_() noexcept { if (coroHandle_) coroHandle_.destroy(); }

public:
    TaskCont(CoroHandle handle) : coroHandle_{ handle } {}
    TaskCont(const TaskCont&) = delete;
    TaskCont& operator=(const TaskCont&) = delete;
    TaskCont(TaskCont&& another) noexcept : coroHandle_{
        std::exchange(another.coroHandle_, nullptr)
    } {
    }
    TaskCont& operator=(TaskCont&& another) noexcept {
        Destroy_();
        coroHandle_ = std::exchange(another.coroHandle_, nullptr);
        return *this;
    }
    ~TaskCont() { Destroy_(); }

    struct promise_type
    {
        CoroHandle lastLevelHandle_;
        CoroHandle topLevelHandle_;
        CoroHandle activeHandle_;

        TaskCont get_return_object() {
            // Initially, set itself as top level.
            auto handle = CoroHandle::from_promise(*this);
            topLevelHandle_ = activeHandle_ = handle;
            return TaskCont{ handle };
        }
        std::suspend_always initial_suspend() { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
        std::suspend_always final_suspend() noexcept { return {}; }
    };

    bool await_ready() const noexcept { return false; }
    void await_suspend(CoroHandle continuation) {
        // When it's awaited, set handles upwards.
        auto& promise = coroHandle_.promise();
        promise.lastLevelHandle_ = continuation;
        promise.topLevelHandle_ = continuation.promise().topLevelHandle_;
        promise.topLevelHandle_.promise().activeHandle_ = coroHandle_;
    }
    void await_resume() const noexcept {}

    bool Resume() {
        if (!coroHandle_ || coroHandle_.done())
            return false;
        auto& promise = coroHandle_.promise();
        auto resumeHandle = promise.activeHandle_;
        while (resumeHandle.done())
            resumeHandle = resumeHandle.promise().lastLevelHandle_;
        promise.activeHandle_ = resumeHandle;

        if (!resumeHandle)
            return false;

        resumeHandle.resume();
        return !coroHandle_.done();
    }
};

TaskCont Test2()
{
    std::println("Hello,");
    co_await std::suspend_always{};
    std::println("World!");
}

TaskCont Test()
{
    std::println("Test: before Test2.");
    co_await Test2();
    std::println("Test: after Test2.");
}

int main()
{
    auto m = Test();
    while (m.Resume())
    {
        std::println("Back to main.");
    }
    return 0;
}