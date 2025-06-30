#pragma once
#include <limits>
#include <mutex>
#include <optional>
#include <semaphore>
#include <vector>

using CountingSemaphore =
    std::counting_semaphore<std::numeric_limits<std::ptrdiff_t>::max()>;

class ThreadSafeQueue
{
    std::vector<int> vec_;
    std::size_t head_ = 0, size_ = 0;
    mutable std::mutex mutex_;

    CountingSemaphore sizeSema_;
    CountingSemaphore spaceSema_;

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
    ThreadSafeQueue(std::ptrdiff_t maxSize)
        : vec_(maxSize), sizeSema_{ 0 }, spaceSema_{ maxSize }
    {
    }

    std::optional<int> TryPop()
    {
        if (!sizeSema_.try_acquire())
            return std::nullopt;

        std::lock_guard _{ mutex_ };
        std::optional<int> result{ PopUnsafe_() };
        spaceSema_.release();
        return result;
    }

    bool TryPush(int elem)
    {
        if (!spaceSema_.try_acquire())
            return false;

        std::lock_guard _{ mutex_ };
        PushUnsafe_(elem);
        sizeSema_.release();
        return true;
    }

    int Pop()
    {
        sizeSema_.acquire();

        std::lock_guard _{ mutex_ };
        int result{ PopUnsafe_() };
        spaceSema_.release();
        return result;
    }

    void Push(int elem)
    {
        spaceSema_.acquire();

        std::lock_guard _{ mutex_ };
        PushUnsafe_(elem);
        sizeSema_.release();
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
