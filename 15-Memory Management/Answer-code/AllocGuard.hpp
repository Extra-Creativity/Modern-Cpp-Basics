#pragma once
#include <memory>
#include <utility>

template<typename T>
class AllocGuard
{
protected:
    using AllocTraits = std::allocator_traits<T>;
    using PointerType = AllocTraits::pointer;

    T &alloc_;
    PointerType ptr_;
    std::size_t size_;

public:
    AllocGuard(T &alloc, std::size_t n = 1)
        : alloc_{ alloc }, ptr_{ AllocTraits::allocate(alloc, n) }, size_{ n }
    {
    }

    auto Get() const noexcept { return ptr_; }
    auto Release() noexcept { return std::exchange(ptr_, PointerType{}); }

    ~AllocGuard()
    {
        if (ptr_)
            AllocTraits::deallocate(alloc_, ptr_, size_);
    }
};

template<typename T>
class AllocConstructionGuard : public AllocGuard<T>
{
    using Super_ = AllocGuard<T>;
    using AllocTraits = Super_::AllocTraits;

public:
    template<typename... Args>
    AllocConstructionGuard(T &alloc, Args &&...args) : Super_{ alloc, 1 }
    {
        AllocTraits::construct(this->alloc_, this->ptr_,
                               std::forward<Args>(args)...);
    }

    ~AllocConstructionGuard()
    {
        if (this->ptr_)
            AllocTraits::destroy(this->alloc_, this->ptr_);
    }
};
