#include <threads.h>

#include <functional>
#include <memory>
#include <optional>
#include <system_error>
#include <tuple>
#include <utility>

#include <iostream>

class Thread
{
    std::optional<thrd_t> handle_;

    template<typename... TupleTypes>
    static int InvokeWrapper_(void *args)
    {
        using TupleType = std::tuple<TupleTypes...>;
        std::unique_ptr<TupleType> ptr{ static_cast<TupleType *>(args) };

        std::apply(
            [](auto &&...args) {
                std::invoke(std::forward<decltype(args)>(args)...);
            },
            std::move(*ptr));
        return 0;
    }

    void CheckJoinable_()
    {
        if (joinable())
            std::terminate();
    }

public:
    Thread() = default;
    Thread(const Thread &) = delete;
    Thread &operator=(const Thread &) = delete;
    Thread(Thread &&another) noexcept
        : handle_{ std::exchange(another.handle_, std::nullopt) }
    {
    }
    Thread &operator=(Thread &&another) noexcept
    {
        CheckJoinable_();
        handle_ = std::exchange(another.handle_, std::nullopt);
        return *this;
    }
    ~Thread() { CheckJoinable_(); }
    void swap(Thread &another) noexcept { handle_.swap(another.handle_); }

    template<typename F, typename... Args>
    Thread(F &&func, Args &&...args)
    {
        using TupleType = std::tuple<std::decay_t<F>, std::decay_t<Args>...>;
        auto ptr = std::make_unique<TupleType>(std::forward<F>(func),
                                               std::forward<Args>(args)...);
        thrd_t handle;
        int err = thrd_create(
            &handle, &InvokeWrapper_<std::decay_t<F>, std::decay_t<Args>...>,
            ptr.get());
        if (err == thrd_nomem || err == thrd_error)
        {
            throw std::system_error{ std::make_error_code(
                std::errc::resource_unavailable_try_again) };
        }
        // noexcept below.
        ptr.release();
        handle_ = handle;
        return;
    }

    bool joinable() const noexcept { return handle_.has_value(); }

    void join()
    {
        if (!joinable())
        {
            throw std::system_error{ std::make_error_code(
                std::errc::invalid_argument) };
        }

        int err = thrd_join(*handle_, nullptr);
        if (err == thrd_error)
        {
            throw std::system_error{ std::make_error_code(
                std::errc::no_such_process) };
        }

        handle_ = std::nullopt;
    }

    void detach()
    {
        if (!joinable())
        {
            throw std::system_error{ std::make_error_code(
                std::errc::invalid_argument) };
        }

        int err = thrd_detach(*handle_);
        if (err == thrd_error)
        {
            throw std::system_error{ std::make_error_code(
                std::errc::no_such_process) };
        }

        handle_ = std::nullopt;
    }
};

template<>
void std::swap<Thread>(Thread &a, Thread &b) noexcept
{
    a.swap(b);
}

class Object
{
public:
    Object() { std::cout << "Construct at " << this << "\n"; };
    ~Object() { std::cout << "Destruct at " << this << "\n"; };
    Object(const Object &) { std::cout << "Const Copy at " << this << "\n"; };
    Object(Object &&) { std::cout << "Move at " << this << "\n"; };
    Object &operator=(const Object &)
    {
        std::cout << "Const Copy Assignment at " << this << "\n";
        return *this;
    };
    Object &operator=(Object &&)
    {
        std::cout << "Move Assignment at " << this << "\n";
        return *this;
    };
};

void foo(Object obj)
{
    std::cout << "Hello, my new thread!\n";
}

int main()
{
    // Object o{};
    Thread t{ foo, Object{} };
    t.join();
    std::cout << "Check joinable: " << std::boolalpha << t.joinable() << "\n";
    return 0;
}