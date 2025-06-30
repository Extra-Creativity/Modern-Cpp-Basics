#pragma once
#include <algorithm>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

template<typename Key, typename Value, typename Hasher = std::hash<Key>, 
         typename EqualCompare = std::equal_to<Key>>
class ThreadSafeUnorderedMap
{
public:
    using ValueType = std::pair<Key, Value>;
private:
    class Bucket
    {
        friend class ThreadSafeUnorderedMap;
        std::list<ValueType> elems_;
        mutable std::shared_mutex mutex_;

        auto FindUnsafe_(const Key& key, const EqualCompare& equalCompare) const
        {
            return std::ranges::find_if(elems_, [&](const auto& elem) { 
                return equalCompare(elem.first, key); 
            });
        }

    public:
        std::optional<Value> Find(const Key& key, const EqualCompare& equalCompare) const
        {
            std::shared_lock _{ mutex_ };
            auto it = FindUnsafe_(key, equalCompare);
            if (it == elems_.end())
                return std::nullopt;
            return it->second;
        }

        bool Insert(auto&& key, auto&& value, const EqualCompare& equalCompare)
        {
            std::unique_lock _{ mutex_ };
            if (FindUnsafe_(key, equalCompare) != elems_.end())
                return false;
            elems_.emplace_back(std::forward<decltype(key)>(key),
                                std::forward<decltype(value)>(value));
            return true;
        }

        bool Erase(const Key& key, const EqualCompare& equalCompare)
        {
            std::unique_lock _{ mutex_ };
            auto it = FindUnsafe_(key, equalCompare);
            if (it == elems_.end())
                return false;
            elems_.erase(it);
            return true;
        }
    };

    auto&& GetBucketByKey_(this auto&& self, const Key& key) noexcept
    {
        auto idx = self.hash_(key) % self.buckets_.size();
        return self.buckets_[idx];
    }


public:
    ThreadSafeUnorderedMap(std::size_t bucketNum) : buckets_(bucketNum) { }

    auto Find(const Key& key) const
    {
        auto& bucket = GetBucketByKey_(key);
        return bucket.Find(key, compare_);
    }
    // Here you can also add overloads like the standard library.
    auto Insert(const Key& key, const Value& value)
    {
        auto& bucket = GetBucketByKey_(key);
        return bucket.Insert(key, value, compare_);
    }
    
    auto Erase(const Key& key)
    {
        auto& bucket = GetBucketByKey_(key);
        return bucket.Erase(key, compare_);
    }
    
    std::size_t Size() const
    {
        std::size_t size = 0;
        for (auto& bucket : buckets_)
        {
            std::shared_lock _{ bucket.mutex_ };
            size += bucket.elems_.size();
        }
        return size;
    }

    std::unordered_map<Key, Value> GetSnapshot() const
    {
        std::unordered_map<Key, Value, Hasher, EqualCompare> snapshot;
        for (auto& bucket : buckets_)
        {
            std::shared_lock _{ bucket.mutex_ };
            snapshot.insert_range(bucket.elems_);
        }
        return snapshot;
    }

private:
    std::vector<Bucket> buckets_;

    Hasher hash_{};
    EqualCompare compare_{};
};
