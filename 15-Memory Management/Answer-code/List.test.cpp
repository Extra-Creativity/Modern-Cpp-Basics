#include "List.hpp"

#include <print>

void OutputRange(auto &list)
{
    std::println("Size: {}; Values = {}", std::size(list), list);
}

template<typename T, typename Allocator>
void Test(const Allocator &alloc = {})
{
    T a[3]{ 1, 2, 3 };
    std::vector<T> b{ 4, 5, 6, 7, 8 };

    List<T, Allocator> list1{ a, a + 3 }, list2{ b.begin(), b.end(), alloc };
    OutputRange(list1), OutputRange(list2), OutputRange(b);
    list1.Splice(++list1.begin(), list2, ++list2.begin(), --list2.end());
    OutputRange(list1), OutputRange(list2);

    auto it = list2.Insert(++list2.begin(), std::next(list1.cbegin(), 3),
                           --list1.cend());
    OutputRange(list1), OutputRange(list2);
    std::println("Returned position: {}", *it);

    list1 = std::move(list2);
    OutputRange(list1), OutputRange(list2);

    list2.Splice(list2.begin(), list1);
    OutputRange(list1), OutputRange(list2);

    list1.Assign(b.cbegin(), b.cend());
    it = list2.Erase(++list2.begin(), --list2.end());
    OutputRange(list1), OutputRange(list2);
    std::println("Returned position: {}", *it);
}

template<typename T>
class DetailedAllocator
{
    static inline int s_counter_ = 0;

public:
    int id;

    // 如果想要测试POCMA的情况，把下面这行注释掉。
    // using propagate_on_container_move_assignment = std::true_type;
    using value_type = T;

    DetailedAllocator() noexcept : id{ s_counter_++ } {}
    template<typename U>
    DetailedAllocator(const DetailedAllocator<U> &another) : id{ another.id }
    {
        std::println("Allocator copy: from {}", another.id);
    }
    DetailedAllocator &operator=(const DetailedAllocator &another)
    {
        std::println("Allocator assign: from {} to {}", another.id, id);
        id = another.id;
        return *this;
    }

    friend bool operator==(const DetailedAllocator<T> &a,
                           const DetailedAllocator<T> &b)
    {
        std::println("Allocator compare: {} and {}", a.id, b.id);
        return a.id == b.id;
    }

    void deallocate(T *const ptr, const size_t _Count) noexcept { free(ptr); }

    T *allocate(const size_t _Count)
    {
        return static_cast<T *>(malloc(_Count * sizeof(T)));
    }
};

class A
{
    int id = -1;

public:
    int ID() const { return id; }

    A() = default;
    A(int a) : id{ a } { std::println("Construct : {}", a); }
    A(const A &another) : id{ another.id }
    {
        std::println("Copy Construct : {}", id);
    }
    A(A &&another) noexcept : id{ another.id }
    {
        another.id = -1;
        std::println("Move Construct : {}", id);
    }

    A &operator=(const A &another)
    {
        id = another.id;
        std::println("Copy Assign : {}", id);
        return *this;
    }
    A &operator=(A &&another) noexcept
    {
        id = another.id, another.id = -1;
        std::println("Move Assign : {}", id);
        return *this;
    }

    friend std::ostream &operator<<(std::ostream &os, const A &a)
    {
        os << a.id;
        return os;
    }

    ~A() { std::println("Destruct: {}", id); }
};

template<typename CharT>
struct std::formatter<A, CharT> : std::formatter<int, CharT>
{
    using Base = std::formatter<int, CharT>;
    auto format(const A &val, auto &&ctx) const
    {
        return Base::format(val.ID(), std::forward<decltype(ctx)>(ctx));
    }
};

int main()
{
    Test<int, std::allocator<int>>();
    Test<int, DetailedAllocator<int>>();

    Test<A, std::allocator<A>>();
    Test<A, DetailedAllocator<A>>();
    return 0;
}