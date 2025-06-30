#pragma once
#include <mutex>
#include <optional>
#include <vector>

class ThreadSafeQueue
{
    std::vector<int> vec_;
    std::size_t head_ = 0, size_ = 0;
    mutable std::mutex mutex_;

    bool IsFullUnsafe_() const noexcept { return size_ == vec_.size(); }
    bool IsEmptyUnsafe_() const noexcept { return size_ == 0; }

    auto PopUnsafe_()
    {
        auto elem = vec_[head_];

        size_--, head_++;
        if (head_ == vec_.size())
            head_ = 0;

        return elem;
    }

    auto GetTailIndex_() const noexcept
    {
        auto idx = head_ + size_;
        auto limit = vec_.size();
        return idx >= limit ? idx - limit : idx;
    }

    void PushUnsafe_(int elem)
    {
        auto idx = GetTailIndex_();
        vec_[idx] = elem;
        size_++;
    }

public:
    ThreadSafeQueue(std::size_t maxSize) : vec_(maxSize) {}

    std::optional<int> TryPop()
    {
        std::lock_guard _{ mutex_ };
        if (IsEmptyUnsafe_())
            return std::nullopt;
        std::optional<int> result{ PopUnsafe_() };
        return result;
    }

    bool TryPush(int elem)
    {
        std::lock_guard _{ mutex_ };
        if (IsFullUnsafe_())
            return false;
        PushUnsafe_(elem);
        return true;
    }

    std::vector<int> GetSnapshot() const
    {
        std::lock_guard _{ mutex_ };
        std::vector<int> snapshot;
        snapshot.reserve(size_);

        auto tail = GetTailIndex_();
        if (tail > head_)
        {
            snapshot.insert(snapshot.end(), vec_.begin() + head_,
                            vec_.begin() + tail);
        }
        else if (size_ != 0) // When tail == head_, maybe empty.
        {
            snapshot.insert(snapshot.end(), vec_.begin() + head_, vec_.end());
            snapshot.insert(snapshot.end(), vec_.begin(), vec_.begin() + tail);
        }

        return snapshot;
    }
};