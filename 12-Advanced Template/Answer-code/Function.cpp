#include <exception>
#include <functional>
#include <print>
#include <type_traits>
#include <utility>

template<typename>
class Function;

namespace Detail
{
template<typename T, typename... Args>
T *AllocFunction(Args &&...args)
{
    return new T{ std::forward<Args>(args)... };
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
    virtual FunctionProxyBase *Clone() const = 0;
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

    FunctionProxy *Clone() const override
    {
        return Detail::AllocFunction<FunctionProxy>(func_);
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

    ProxyBasePtr proxy_ = nullptr;
    void Dealloc_() noexcept { delete GetImplPtr_(); }

public:
    Function() noexcept = default;
    Function(std::nullptr_t) noexcept {}
    Function(const Function &another)
        : proxy_{ another ? another.GetImplPtr_()->Clone() : nullptr }
    {
    }
    Function(Function &&another)
        : proxy_{ std::exchange(another.GetImplPtr_(), nullptr) }
    {
    }

    Function &operator=(const Function &another)
    {
        Function{ another }.swap(*this);
        return *this;
    }
    Function &operator=(Function &&another) noexcept
    {
        Dealloc_();
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

    ProxyBasePtr &GetImplPtr_() noexcept { return proxy_; }
    ProxyBasePtr GetImplPtr_() const noexcept { return proxy_; }

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
                std::forward<F0>(f));
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

    void swap(Function &another) noexcept
    {
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

int main()
{
    Function<int(int)> a{ Test };
    auto result = a(12);
    std::println("{} {} {} {}", result, nullptr == a, nullptr != a,
                 a.TargetType().name());
    Function<int(int)> c = [&](int b) { return a(b) + b; };
    std::println("{}", c(11));
    return 0;
}