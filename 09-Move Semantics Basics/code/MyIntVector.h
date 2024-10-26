#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

class MyIntVector
{
    int *ptr_ = nullptr;
    std::size_t size_ = 0;

    void Clear_() noexcept { delete[] ptr_; }

    void Reset_() noexcept { ptr_ = nullptr, size_ = 0; }

    // This method assumes currently ptr_ = nullptr && size_ = 0.
    void AllocAndCopy_(const MyIntVector &another)
    {
        if (another.size_ == 0)
            return;
        std::unique_ptr<int[]> arr{ new int[another.size_] };
        std::memcpy(arr.get(), another.ptr_, another.size_ * sizeof(int));

        ptr_ = arr.release();
        size_ = another.size_;
        return;
    }

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

    MyIntVector(const MyIntVector &another) { AllocAndCopy_(another); }

    MyIntVector(MyIntVector &&another) noexcept
        : ptr_{ std::exchange(another.ptr_, nullptr) },
          size_{ std::exchange(another.size_, 0) }
    {
    }

    MyIntVector &operator=(const MyIntVector &another)
    {
        if (this == &another)
            return *this;

        Clear_();
        Reset_(); // For basic exception guarantee.
        AllocAndCopy_(another);
        return *this;
    }

    MyIntVector &operator=(MyIntVector &&another) noexcept
    {
        if (this == &another)
            return *this;

        Clear_();
        ptr_ = std::exchange(another.ptr_, nullptr),
        size_ = std::exchange(another.size_, 0);
    }

    ~MyIntVector() { Clear_(); }
};