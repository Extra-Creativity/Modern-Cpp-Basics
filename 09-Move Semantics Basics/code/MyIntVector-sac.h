#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

class MyIntVector
{
    int *ptr_ = nullptr;
    std::size_t size_ = 0;

public:
    MyIntVector(std::size_t initSize)
    {
        if (initSize == 0)
            return;
        ptr_ = new int[initSize], size_ = initSize;
    }
    int &operator[](std::size_t idx) { return ptr_[idx]; }
    int operator[](std::size_t idx) const { return ptr_[idx]; }
    auto size() const { return size_; }
    void swap(MyIntVector &another) noexcept
    {
        std::ranges::swap(ptr_, another.ptr_);
        std::ranges::swap(size_, another.size_);
    }

    MyIntVector(const MyIntVector &another)
    {
        if (another.size_ == 0)
            return;
        std::unique_ptr<int[]> arr{ new int[another.size_] };
        std::memcpy(arr.get(), another.ptr_, another.size_ * sizeof(int));

        ptr_ = arr.release();
        size_ = another.size_;
        return;
    }

    MyIntVector(MyIntVector &&another) noexcept
        : ptr_{ std::exchange(another.ptr_, nullptr) },
          size_{ std::exchange(another.size_, 0) }
    {
    }

    MyIntVector &operator=(const MyIntVector &another)
    {
        if (this == &another)
            return *this;

        MyIntVector temp{ another };
        swap(temp);
        return *this;
    }

    MyIntVector &operator=(MyIntVector &&another) noexcept
    {
        if (this == &another)
            return *this;

        MyIntVector temp{ std::move(another) };
        swap(temp);
        return *this;
    }

    // If implemented as member-wise swap.
    // MyIntVector &operator=(MyIntVector &&another) noexcept
    // {
    //     swap(another);
    //     return *this;
    // }

    ~MyIntVector() { delete[] ptr_; }
};