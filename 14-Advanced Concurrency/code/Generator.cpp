#include <algorithm>
#include <coroutine>
#include <ranges>
#include <stdexcept>
#include <type_traits>

template<typename T>
class [[nodiscard]] Generator
{
public:
    struct promise_type;
private:
    using CoroHandle = std::coroutine_handle<promise_type>;
    CoroHandle coroHandle_;

    Generator(CoroHandle handle) : coroHandle_{ handle } {}

public:
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    Generator(Generator&& another) noexcept : coroHandle_{
        std::exchange(another.coroHandle_, nullptr)
    } { }
    Generator& operator=(Generator&& another) noexcept {
        std::swap(coroHandle_, another.coroHandle_);
        return *this;
    }
    ~Generator() { if (coroHandle_) coroHandle_.destroy(); }

    using reference = T&&; // reference collapsing.
    using yielded = T&&;

    using YieldedValueType = std::remove_cvref_t<yielded>;

    struct CopyAwaiter : public std::suspend_always
    {
        YieldedValueType val;
        CopyAwaiter(const YieldedValueType& init_val) : val(init_val) {}
    };

    struct promise_type
    {
        std::remove_reference_t<yielded>* ptr;
        CopyAwaiter yield_value(const YieldedValueType& lval) {
            CopyAwaiter awaiter{ lval };
            ptr = &awaiter.val;
            return awaiter;
        }

        std::suspend_always yield_value(yielded val) {
            ptr = &val;
            return {};
        }

        Generator get_return_object() {
            return Generator{ CoroHandle::from_promise(*this) };
        }
        std::suspend_always initial_suspend() const noexcept { return {}; }
        void return_void() const noexcept {}
        void unhandled_exception() { std::terminate(); }
        std::suspend_always final_suspend() noexcept { return {}; }
    };

    class iterator
    {
        friend class Generator;
        CoroHandle handle_;

        iterator(CoroHandle handle) : handle_{ handle } {}
        auto ExchangeEmpty_() { return std::exchange(handle_, nullptr); }

    public:
        iterator(iterator&& another) noexcept : handle_{ another.ExchangeEmpty_()} {}
        iterator& operator=(iterator&& another) noexcept {
            handle_ = another.ExchangeEmpty_();
            return *this;
        }

        reference operator*() const noexcept {
            return static_cast<reference>(*(handle_.promise().ptr));
        }
        iterator& operator++() { handle_.resume(); return *this; }
        void operator++(int) { ++(*this); }

        friend bool operator==(const iterator& it, std::default_sentinel_t) {
            return it.handle_.done();
        }
    };

    iterator begin() { 
        coroHandle_.resume();
        return iterator{ coroHandle_ };
    }

    auto end() const noexcept { return std::default_sentinel; }
};
