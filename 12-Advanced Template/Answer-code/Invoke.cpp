#include <print>
#include <type_traits>
#include <utility>

namespace Detail
{
template<typename>
constexpr bool is_reference_wrapper_v = false;
template<typename T>
constexpr bool is_reference_wrapper_v<std::reference_wrapper<T>> = true;

template<typename T0>
struct InvokeHelper
{
    template<typename T>
    static constexpr decltype(auto) Call(T &&func, auto &&...args)
    {
        return std::forward<T>(func)(std::forward<decltype(args)>(args)...);
    }
};

template<typename T0>
    requires std::is_member_pointer_v<T0>
struct InvokeHelper<T0>
{
    template<typename PointedType, typename ClassType, typename T,
             typename... Args>
    static constexpr decltype(auto) Call(PointedType ClassType::*func, T &&obj,
                                         Args &&...args)
    {
        using RawT = std::remove_cvref_t<T>;
        constexpr bool isReferenceWrapper = is_reference_wrapper_v<RawT>;
        // 注：这里在下面的constexpr分支中判断isReferenceWrapper后直接调用也可以。
        using ObjectType = std::remove_reference_t<std::conditional_t<
            isReferenceWrapper, std::unwrap_reference_t<RawT>, T>>;

        ObjectType &realObj = obj;
        static constexpr bool matchCase =
            std::is_same_v<ClassType, ObjectType> ||
            std::is_base_of_v<ClassType, ObjectType>;

        if constexpr (std::is_function_v<PointedType>)
        {
            if constexpr (matchCase)
            {
                return (realObj.*func)(std::forward<Args>(args)...);
            }
            else
            {
                return ((*realObj).*func)(std::forward<Args>(args)...);
            }
        }
        else if constexpr (sizeof...(args) == 0)
        {
            if constexpr (matchCase)
            {
                return (realObj.*func);
            }
            else
            {
                return ((*realObj).*func);
            }
        }
        else
        {
            static_assert(
                false,
                "Pointer to data member is not callable with arguments.");
        }
    }
};
} // namespace Detail

template<typename F, typename... Args>
decltype(auto) Invoke(F &&func, Args &&...args)
{
    return Detail::InvokeHelper<std::remove_cvref_t<F>>::Call(
        std::forward<F>(func), std::forward<Args>(args)...);
}

int Test(int param)
{
    std::println("Hello, {}", param);
    return param + 1;
}

class A
{
public:
    int m = 0;
    int Test(int param) const
    {
        std::println("Hello, {}", param);
        return param + 1;
    }
};

class B
{
public:
    int operator()(int param) const
    {
        std::println("Hi, {}", param);
        return param + 1;
    }
};

template<typename T>
void ObjectTest(T &a)
{
    std::println("----Member function pointer + obj caller test----");
    std::println("Hi, {}", Invoke(&A::Test, A{}, 1));
    std::println("Hi, {}", Invoke(&A::Test, a, 1));
    std::println("Hi, {}", Invoke(&A::Test, std::move(a), 1));

    std::println(
        "----Member function pointer + reference_wrapper obj caller test----");
    std::println("Hi, {}", Invoke(&A::Test, std::ref(a), 1));
    std::println("Hi, {}", Invoke(&A::Test, std::cref(a), 1));

    std::println("----Member function pointer + pointer caller test----");
    std::println("Hi, {}", Invoke(&A::Test, &a, 1));

    std::println("----Data member pointer + obj caller test----");
    std::println("Hi, {}", Invoke(&A::m, A{}));
    std::println("Hi, {}", Invoke(&A::m, a));
    std::println("Hi, {}", Invoke(&A::m, std::move(a)));

    std::println(
        "----Data member pointer + reference_wrapper obj caller test----");
    std::println("Hi, {}", Invoke(&A::m, std::ref(a)));
    std::println("Hi, {}", Invoke(&A::m, std::cref(a)));

    std::println("----Data member pointer + pointer caller test----");
    std::println("Hi, {}", Invoke(&A::m, &a));
}

int main()
{
    std::println("----Normal function test----");
    std::println("Hi, {}", Invoke(Test, 1));
    std::println("----Normal function pointer test----");
    std::println("Hi, {}", Invoke(Test, 1));

    B b1;
    std::println("----Normal functor test----");
    std::println("Hi, {}", Invoke(b1, 1));
    std::println("Hi, {}", Invoke(B{}, 2));

    const B b2;
    std::println("----Const functor test----");
    std::println("Hi, {}", Invoke(b2, 3));

    std::println("Non-const object member pointer test: ", Invoke(Test, 1));
    A a1;
    ObjectTest(a1);

    std::println("Const object member pointer test: ", Invoke(Test, 1));
    const A a2;
    ObjectTest(a2);

    return 0;
}