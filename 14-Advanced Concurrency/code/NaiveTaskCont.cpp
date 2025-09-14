#include <coroutine>
#include <stdexcept>
#include <print>

class NaiveTaskCont
{
public:
    struct promise_type;
private:
    using CoroHandle = std::coroutine_handle<promise_type>;
    CoroHandle coroHandle_;

    void Destroy_() noexcept { if (coroHandle_) coroHandle_.destroy(); }

public:
    NaiveTaskCont(CoroHandle handle) : coroHandle_{ handle } {}
    NaiveTaskCont(const NaiveTaskCont&) = delete;
    NaiveTaskCont& operator=(const NaiveTaskCont&) = delete;
    NaiveTaskCont(NaiveTaskCont&& another) noexcept : coroHandle_{
        std::exchange(another.coroHandle_, nullptr)
    } { }
    NaiveTaskCont& operator=(NaiveTaskCont&& another) noexcept {
        Destroy_();
        coroHandle_ = std::exchange(another.coroHandle_, nullptr);
        return *this;
    }
    ~NaiveTaskCont() { Destroy_(); }

    struct promise_type
    {
        CoroHandle nextLevelHandle_;

        NaiveTaskCont get_return_object() {
            return NaiveTaskCont{ CoroHandle::from_promise(*this) };
        }
        std::suspend_always initial_suspend() { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
        std::suspend_always final_suspend() noexcept { return {}; }
    };

    class ContAwaiter
    {
        CoroHandle nextLevelHandle_;
        ContAwaiter(CoroHandle handle) : nextLevelHandle_{ handle } {}
        friend NaiveTaskCont;

    public:
        bool await_ready() const noexcept { return false; }
        void await_suspend(CoroHandle continuation) {
            auto& promise = continuation.promise();
            promise.nextLevelHandle_ = nextLevelHandle_;
        }
        void await_resume() const noexcept {}
    };

    ContAwaiter operator co_await() const noexcept { return { coroHandle_ }; }

    bool Resume() {
        if (!coroHandle_ || coroHandle_.done())
            return false;

        auto resumeHandle = coroHandle_;
        while (true)
        {
            auto& promise = resumeHandle.promise();
            auto nextHandle = promise.nextLevelHandle_;
            if (nextHandle == nullptr)
                break;
            if (nextHandle.done())
            {
                promise.nextLevelHandle_ = nullptr;
                break;
            }
            resumeHandle = nextHandle;
        }

        resumeHandle.resume();
        return !coroHandle_.done();
    }
};

NaiveTaskCont Test2()
{
    std::println("Hello,");
    co_await std::suspend_always{};
    std::println("World!");
}

NaiveTaskCont Test()
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