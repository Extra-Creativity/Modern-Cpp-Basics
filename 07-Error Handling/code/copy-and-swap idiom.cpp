#include <memory>
#include <algorithm>

template<typename T>
class Vector
{
public:
    Vector(std::size_t num, const T& val){
        std::unique_ptr<T[]> arr{ new T[num] };
        std::ranges::fill(arr.get(), arr.get() + num, val);
        first_ = arr.release();
        last_ = end_ = first_ + num;
    }

    std::size_t size() const noexcept { return last_ - first_; }
    auto& operator[](std::size_t idx) noexcept { return first_[idx]; }
    const auto& operator[](std::size_t idx) const noexcept { return first_[idx]; }

    Vector(const Vector& another){
        auto size = another.size();
        std::unique_ptr<T[]> arr{ new T[size] };
        std::ranges::copy(another.first_, another.last_, arr.get());
        first_ = arr.release();
        last_ = end_ = first_ + size;
    }

    friend void swap(Vector& vec1, Vector& vec2) noexcept {
        std::ranges::swap(vec1.first_, vec2.first_);
        std::ranges::swap(vec1.last_, vec2.last);
        std::ranges::swap(vec1.end_, vec2.end_);
    }

    Vector& operator=(const Vector& another) {
        Vector vec{another};
        swap(vec, *this);
        return *this;
    }

private:
    T* first_, *last_, *end_;
};