#include <algorithm>
#include <cassert>
#include <functional>
#include <print>
#include <type_traits>
#include <utility>

template<typename>
class Function;

namespace Detail
{
// 如果类型大小不超过40 bytes，进行SBO
inline constexpr std::size_t s_sboSize = 40;
inline constexpr auto s_sboAlignment = alignof(std::max_align_t);

template<typename T>
static inline constexpr bool s_useSBO =
    sizeof(T) <= Detail::s_sboSize && s_sboAlignment % alignof(T) == 0;

template<typename T, typename... Args>
T *AllocFunction(void *space, Args &&...args)
{
    if constexpr (s_useSBO<T>)
    {
        std::println("SBO here.");
        auto result = new (space) T{ std::forward<Args>(args)... };
        assert(result == space);
        return result;
    }
    else
    {
        std::println("No SBO here.");
        return new T{ std::forward<Args>(args)... };
    }
}

template<typename F, typename ReturnType, typename... Args>
concept GeneralFunctionMatch =
    std::convertible_to<std::invoke_result_t<std::decay_t<F>, Args...>,
                        ReturnType>;

template<typename T>
struct IsFunctionInstantiation : std::false_type
{
};

template<typename T>
struct IsFunctionInstantiation<Function<T>> : std::true_type
{
};

} // namespace Detail

template<typename ReturnType, typename... Args>
class FunctionProxyBase
{
public:
    // 多了一个Move，用来进行SBO上的object移动。
    virtual FunctionProxyBase *Move(void *space) noexcept = 0;
    virtual FunctionProxyBase *Clone(void *space) const = 0;
    virtual ReturnType Call(Args...) = 0;
    virtual const std::type_info &TargetType() const noexcept = 0;
    virtual ~FunctionProxyBase() = default;
};

template<typename T, typename ReturnType, typename... Args>
class FunctionProxy : public FunctionProxyBase<ReturnType, Args...>
{
    T func_;

public:
    FunctionProxy(const T &func) : func_{ func } {}
    FunctionProxy(T &&func) : func_{ std::move(func) } {}

    FunctionProxy *Move(void *space) noexcept override
    {
        return Detail::AllocFunction<FunctionProxy>(space, std::move(func_));
    }
    FunctionProxy *Clone(void *space) const override
    {
        return Detail::AllocFunction<FunctionProxy>(space, func_);
    }

    ReturnType Call(Args... args) override
    {
        return std::invoke(func_, std::forward<Args>(args)...);
    }
    const std::type_info &TargetType() const noexcept override
    {
        return typeid(T);
    }
};

template<typename ReturnType, typename... Args>
class Function<ReturnType(Args...)>
{
    using ProxyBasePtr = FunctionProxyBase<ReturnType, Args...> *;

    alignas(Detail::s_sboAlignment) std::byte buffer_[Detail::s_sboSize]{};
    ProxyBasePtr proxy_ = nullptr;

    bool IsLocal_() const noexcept
    {
        return proxy_ == static_cast<const void *>(buffer_);
    }
    void ResetToLocal() noexcept
    {
        proxy_ = static_cast<ProxyBasePtr>(static_cast<void *>(buffer_));
    }
    void Dealloc_() noexcept
    {
        if (IsLocal_())
        {
            std::println("SBO Dealloc here.");
            GetImplPtr_()->~FunctionProxyBase<ReturnType, Args...>();
        }
        else
        {
            if (GetImplPtr_() != nullptr)
                std::println("Non-SBO Dealloc here.");
            delete GetImplPtr_();
        }
    }

    ProxyBasePtr MoveAndDestruct_(void *space) noexcept
    {
        assert(IsLocal_());
        auto dst = GetImplPtr_()->Move(space);
        GetImplPtr_()->~FunctionProxyBase<ReturnType, Args...>();
        return dst;
    }

    ProxyBasePtr &GetImplPtr_() noexcept { return proxy_; }
    ProxyBasePtr GetImplPtr_() const noexcept { return proxy_; }

public:
    using ResultType = ReturnType;

    Function() noexcept = default;
    Function(std::nullptr_t) noexcept {}
    Function(const Function &another)
        : proxy_{ another ? another.GetImplPtr_()->Clone(buffer_) : nullptr }
    {
    }
    Function(Function &&another)
    {
        if (!another) // Just let it be nullptr.
            return;

        if (another.IsLocal_())
        {
            GetImplPtr_() = another.MoveAndDestruct_(buffer_);
            another.GetImplPtr_() = nullptr;
        }
        else
        {
            GetImplPtr_() = std::exchange(another.GetImplPtr_(), nullptr);
        }
    }

    Function &operator=(const Function &another)
    {
        Function{ another }.swap(*this);
        return *this;
    }
    Function &operator=(Function &&another) noexcept
    {
        Dealloc_();
        if (another.IsLocal_())
        {
            GetImplPtr_() = another.MoveAndDestruct_(buffer_);
            another.GetImplPtr_() = nullptr;
        }
        else
            GetImplPtr_() = std::exchange(another.GetImplPtr_(), nullptr);

        return *this;
    }
    Function &operator=(std::nullptr_t) noexcept
    {
        Dealloc_();
        GetImplPtr_() = nullptr;
        return *this;
    }
    template<Detail::GeneralFunctionMatch<ReturnType, Args...> F0>
    Function &operator=(F0 &&f)
    {
        Function{ std::forward<F0>(f) }.swap(*this);
        return *this;
    }
    template<class F>
    Function &operator=(std::reference_wrapper<F> f) noexcept
    {
        Function{ f }.swap(*this);
        return *this;
    }

    ~Function() { Dealloc_(); }

    template<Detail::GeneralFunctionMatch<ReturnType, Args...> F0>
    requires(!std::same_as<std::remove_cvref_t<F0>,
                           Function>) // Required since C++23
    Function(F0 &&f)
    {
        using F = std::decay_t<F0>;
        // Required since C++23
        static_assert(std::is_copy_constructible_v<F> &&
                      std::is_constructible_v<F, F0>);

        // Callable pointer is definitely function pointer.
        if constexpr (std::is_pointer_v<F> ||
                      std::is_member_function_pointer_v<F> ||
                      Detail::IsFunctionInstantiation<F>::value)
        {
            if (f == nullptr)
                return;
        }

        GetImplPtr_() =
            Detail::AllocFunction<FunctionProxy<F, ReturnType, Args...>>(
                buffer_, std::forward<F0>(f));
    }

    ReturnType operator()(Args... args) const
    {
        if (!*this)
        {
            throw std::bad_function_call{};
        }

        return GetImplPtr_()->Call(std::forward<Args>(args)...);
    }

    explicit operator bool() const noexcept { return GetImplPtr_() != nullptr; }

    const std::type_info &TargetType() const noexcept
    {
        if (*this)
            return GetImplPtr_()->TargetType();
        return typeid(void);
    }
    template<typename T>
    T *Target() noexcept
    {
        return typeid(T) == TargetType() ? static_cast<T *>(GetImplPtr_())
                                         : nullptr;
    }

    void swap(Function &another) noexcept
    {
        bool local1 = IsLocal_(), local2 = another.IsLocal_();
        auto complexSwap = [](Function &localOne, Function &nonLocalOne) {
            localOne.GetImplPtr_() =
                std::exchange(nonLocalOne.GetImplPtr_(),
                              localOne.MoveAndDestruct_(nonLocalOne.buffer_));
        };

        if (local1)
        {
            if (local2)
            {
                Function temp;
                temp.GetImplPtr_() = MoveAndDestruct_(temp.buffer_);
                GetImplPtr_() = another.MoveAndDestruct_(buffer_);
                another.GetImplPtr_() = temp.MoveAndDestruct_(another.buffer_);
                temp.GetImplPtr_() = nullptr;
                return;
            }
            complexSwap(*this, another);
            return;
        }
        // else !local1
        if (local2)
            complexSwap(another, *this);
        else
            std::swap(GetImplPtr_(), another.GetImplPtr_());
    }
};

// 其他的由编译器自己生成或调换顺序。
template<class R, class... Args>
bool operator==(const Function<R(Args...)> &f, std::nullptr_t) noexcept
{
    return !static_cast<bool>(f);
}

int Test(int a)
{
    return a;
}

class A
{
    int arr[20];

public:
    A() = default;
    A(const A &) = default;
    A(A &&) = default;
    A &operator=(const A &) = default;
    A &operator=(A &&) = default;
    ~A() { std::println("A destruct."); }

    int operator()(int a) { return a; }
};

int main()
{
    Function<int(int)> a{ Test };
    auto result = a(12);
    std::println("{} {} {} {}", result, nullptr == a, nullptr != a,
                 a.TargetType().name());

    Function<int(int)> b = a;

    Function<int(int)> c = [&](int b) { return a(b) + b; };
    std::println("{}", c(11));

    c = A{};
    std::println("{} {}", c(11), c.TargetType().name());

    std::println("----------Test all-local move assignment------------");
    a = std::move(b);

    std::println("----------All destruction---------");
    return 0;
}