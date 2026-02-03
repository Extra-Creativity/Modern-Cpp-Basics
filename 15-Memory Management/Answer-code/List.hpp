#pragma once

#include <algorithm>
#include <format>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include <cassert>

template<typename FirstType, typename EmptySecondType>
class CompressedPair : public EmptySecondType
{
    FirstType first_;

    template<typename Tuple1, typename Tuple2, std::size_t... Indices1,
             std::size_t... Indices2>
    CompressedPair(Tuple1 &tuple1, Tuple2 &tuple2,
                   [[maybe_unused]] std::index_sequence<Indices1...> indices1,
                   [[maybe_unused]] std::index_sequence<Indices2...> indices2)
        : EmptySecondType(std::get<Indices2>(tuple2)...),
          first_(std::get<Indices1>(tuple1)...)
    {
    }

public:
    CompressedPair() = default;

    template<typename T2, typename U2>
    CompressedPair(T2 &&first, U2 &&second)
        : EmptySecondType{ std::forward<U2>(second) },
          first_{ std::forward<T2>(first) }
    {
    }

    template<class... Args1, class... Args2>
    CompressedPair(std::piecewise_construct_t, std::tuple<Args1...> firstArgs,
                   std::tuple<Args2...> secondArgs)
        : CompressedPair{ firstArgs, secondArgs,
                          std::make_index_sequence<sizeof...(Args1)>(),
                          std::make_index_sequence<sizeof...(Args2)>() }
    {
    }

    auto &&First(this auto &&self) noexcept
    {
        return std::forward_like<decltype(self)>(self.first_);
    }

    auto &&Second(this auto &&self) noexcept
    {
        return std::forward_like<decltype(self)>((EmptySecondType &)self);
    }
};

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

template<typename T, typename Alloc>
struct ListNode
{
    using RealAllocTraits = std::allocator_traits<
        Alloc>::template rebind_traits<ListNode<T, Alloc>>;
    using PointerType = RealAllocTraits::pointer;

    PointerType prev;
    PointerType next;
    T val;

    bool IsOrphan() const noexcept { return prev == this && next == this; }
};

#define SELF std::forward<decltype(self)>(self)

template<typename T, typename Alloc>
class ListBase
{
protected:
    using RealAlloc =
        std::allocator_traits<Alloc>::template rebind_alloc<ListNode<T, Alloc>>;
    using RealAllocTraits = std::allocator_traits<RealAlloc>;

private:
    CompressedPair<ListNode<T, Alloc>, RealAlloc> sentinel_;

    void Reset_() noexcept
    {
        auto ptr = GetSentinelPtr();
        ptr->prev = ptr->next = ptr;
    }

    void EraseAll_() noexcept
    {
        auto &sentinel = GetSentinel();
        auto &alloc = GetAllocator();
        for (auto it = sentinel.next; it != &sentinel;)
        {
            it = it->next;
            RealAllocTraits::destroy(alloc, it->prev);
        }
    }

protected:
    // Here we introduce an additional move, which will be eliminated if
    // ListNode uses char buffer instead of explicit T. Also, take address will
    // leads to inconsistency when pointer is special.
    ListBase(const Alloc &alloc) : sentinel_{ ListNode<T, Alloc>{}, alloc }
    {
        Reset_();
    }
    // For move ctor
    ListBase(RealAlloc &&alloc)
        : sentinel_{ ListNode<T, Alloc>{}, std::move(alloc) }
    {
        Reset_();
    }

    auto &&GetSentinel(this auto &&self) { return SELF.sentinel_.First(); }
    // Rvalue is not useful so omit forwarding.
    auto GetSentinelPtr(this auto &&self) { return &(self.GetSentinel()); }
    auto &&GetAllocator(this auto &&self) { return SELF.sentinel_.Second(); }

    void EraseAllAndReset_() noexcept
    {
        EraseAll_();
        Reset_();
    }

    ~ListBase() { EraseAll_(); }
};

#undef SELF

template<typename T, typename Alloc = std::allocator<T>>
class List : public ListBase<T, Alloc>
{
    std::size_t size_ = 0;

    using AllocTraits = std::allocator_traits<Alloc>;
    using Super_ = ListBase<T, Alloc>;
    using RealAlloc = Super_::RealAlloc;
    using RealAllocTraits = std::allocator_traits<RealAlloc>;

    using Node = ListNode<T, Alloc>;
    using NodePointer = Node::PointerType;
    using ConstNodePointer = Node::RealAllocTraits::const_pointer;

    static constexpr bool AllocAlwaysEqual =
        AllocTraits::is_always_equal::value;

    // To prevent code duplication in Iterator and ConstInterator.
    template<typename Derived, typename ValueT>
    class IteratorBase
    {
        NodePointer node_;
        friend class List;

        auto Unwrap() { return node_; }

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = ValueT;
        using pointer = ValueT *;
        using reference = ValueT &;
        using iterator_category = std::bidirectional_iterator_tag;

        IteratorBase(NodePointer node = nullptr) : node_{ node } {}
        // A better implementation should check nullptr in following methods.

        Derived operator++(int) noexcept
        {
            auto node0 = node_;
            node_ = node_->next;
            return Derived{ node0 };
        }
        Derived &operator++() noexcept
        {
            node_ = node_->next;
            return static_cast<Derived &>(*this);
        }

        Derived operator--(int) noexcept
        {
            auto node0 = node_;
            node_ = node_->prev;
            return Derived{ node0 };
        }
        Derived &operator--() noexcept
        {
            node_ = node_->prev;
            return static_cast<Derived &>(*this);
        }

        reference operator*() const noexcept { return node_->val; }
        pointer operator->() const noexcept { return &(node_->val); }
        friend bool operator==(Derived a, Derived b) noexcept
        {
            return a.node_ == b.node_;
        }
    };

public:
    class Iterator : public IteratorBase<Iterator, T>
    {
    };

    class ConstIterator : public IteratorBase<ConstIterator, const T>
    {
        using Super_ = IteratorBase<ConstIterator, const T>;

    public:
        using Super_::Super_;
        ConstIterator(Iterator it) : Super_{ it.node_ } {}
    };

    using iterator = Iterator;
    using const_iterator = ConstIterator;

private:
    template<typename InputIt>
    Iterator InsertBeforeImpl_(NodePointer pos, InputIt first, InputIt last)
    {
        auto posPrev = pos->prev;
        while (first != last)
        {
            AllocConstructionGuard<RealAlloc> guard{ this->GetAllocator(),
                                                     pos->prev, pos, *first };

            ++first;

            pos->prev->next = guard.Get();
            pos->prev = guard.Release();
            // To ensure basic exception guarantee, we need to change `size_`
            // in the loop instead of caching in local var and writing back.
            size_++;
        }
        // Return iterator of first inserted element.
        return Iterator{ posPrev->next };
    }

    bool SpliceImpl_(NodePointer pos, NodePointer first, NodePointer last)
    {
        if (first == last) // Orphan node, nothing to splice.
            return false;

        // Now first and lastPrev are definitely valid nodes.
        auto firstPrev = first->prev, lastPrev = last->prev,
             posPrev = pos->prev;
        // Then make original list orphan [first, last).
        firstPrev->next = last, last->prev = firstPrev;

        posPrev->next = first, first->prev = posPrev;
        pos->prev = lastPrev, lastPrev->next = pos;
        return true;
    }

    std::size_t SpliceWithSizeImpl_(NodePointer pos, NodePointer first,
                                    NodePointer last)
    {
        if (!SpliceImpl_(pos, first, last))
            return 0;
        // Now [first, lastPrev] is connected before pos.
        std::size_t betweenSize = 0;
        while (first != pos)
            first = first->next, betweenSize++;
        return betweenSize;
    }

    void StealNodes_(NodePointer anotherRoot)
    {
        assert(size_ == 0);
        SpliceImpl_(this->GetSentinelPtr(), anotherRoot->next, anotherRoot);
    }

    void EraseAllAndReset_()
    {
        this->Super_::EraseAllAndReset_();
        size_ = 0;
    }

public:
    template<typename InputIt>
    iterator Insert(const_iterator pos, InputIt first, InputIt last)
    {
        return InsertBeforeImpl_(pos.Unwrap(), first, last);
    }

    void Splice(const_iterator pos, List &another)
    {
        SpliceImpl_(pos.Unwrap(), another.begin().Unwrap(),
                    another.end().Unwrap());
        size_ += another.size_, another.size_ = 0;
    }
    void Splice(const_iterator pos, List &&another) { Splice(pos, another); }
    void Splice(const_iterator pos, List &another, const_iterator first,
                const_iterator last)
    {
        if (this == &another)
        { // Size doesn't change.
            SpliceImpl_(pos.Unwrap(), first.Unwrap(), last.Unwrap());
            return;
        }

        auto spliceSize =
            SpliceWithSizeImpl_(pos.Unwrap(), first.Unwrap(), last.Unwrap());
        size_ += spliceSize, another.size_ -= spliceSize;
    }
    void Splice(const_iterator pos, List &&another, const_iterator first,
                const_iterator last)
    {
        Splice(pos, another, first, last);
    }

    List(const Alloc &alloc = Alloc{}) : Super_{ alloc } {}

    template<typename InputIt>
    List(InputIt first, InputIt last, const Alloc &alloc = Alloc{})
        : Super_{ alloc }
    {
        Insert(cend(), first, last);
    }

    List(const List &another, const Alloc &alloc)
        : List{ another.begin(), another.end(), alloc }
    {
    }
    List(const List &another)
        : List{ another, AllocTraits::select_on_container_copy_construction(
                             another.GetAllocator()) }
    {
    }

    List(List &&another, const Alloc &alloc) noexcept : Super_{ alloc }
    {
        if (another.GetAllocator() == this->GetAllocator())
        { // Then just plain move.
            StealNodes_(another.GetSentinelPtr());
            size_ = std::exchange(another.size_, 0);
            return;
        }

        // Otherwise allocators are not inter-operatable, move element by
        // element.
        Insert(cend(), std::move_iterator{ another.begin() },
               std::move_iterator{ another.end() });
    }
    List(List &&another) noexcept
        : Super_{ std::move(another).GetAllocator() },
          size_{ std::exchange(another.size_, 0) }
    {
        StealNodes_(another.GetSentinelPtr());
    }

    // Enhancement: destroy is assumed to be noexcept.
    iterator Erase(const_iterator first, const_iterator last) noexcept
    {
        auto prevFirst = std::prev(first);
        std::size_t decSize = 0;
        auto &alloc = this->GetAllocator();
        for (auto it = first; it != last;)
        {
            auto node = it++;
            RealAllocTraits::destroy(alloc, node.Unwrap());
            decSize++;
        }

        prevFirst.Unwrap()->next = last.Unwrap();
        last.Unwrap()->prev = prevFirst.Unwrap();
        size_ -= decSize;

        return iterator{ last.Unwrap() };
    }

    template<typename InputIt>
    void Assign(InputIt first, InputIt last)
    {
        for (auto it = begin(), endIt = end();; ++it, ++first)
        {
            if (it == endIt)
            { // Add new nodes
                Insert(cend(), first, last);
                break;
            }
            if (first == last)
            { // Release unused nodes;
                Erase(it, endIt);
                break;
            }

            *it = *first;
        }
    }

    List &operator=(const List &another)
    {
        if (this == &another)
            return *this;
        static constexpr bool POCCA =
            AllocTraits::propagate_on_container_copy_assignment::value;
        if constexpr (POCCA)
        {
            auto &alloc = this->GetAllocator(),
                 anotherAlloc = another.GetAllocator();
            // We don't bother to if constexpr here since we expect compiler to
            // optimize.
            if (AllocAlwaysEqual || alloc == anotherAlloc)
            { // Then no need to release all nodes, reuse memory by Assign.
                Assign(another.cbegin(), another.cend());
                alloc = anotherAlloc;
            }
            else
            { // First release all nodes, then insert.
                EraseAllAndReset_();
                alloc = anotherAlloc;
                Insert(cend(), another.begin(), another.end());
            }
        }
        else
        { // Do not propagate, so just assign one by one.
            Assign(another.cbegin(), another.cend());
        }

        return *this;
    }

    List &operator=(List &&another) noexcept(AllocAlwaysEqual)
    {
        if (this == &another)
            return *this;

        static constexpr bool POCMA =
            AllocTraits::propagate_on_container_move_assignment::value;
        auto &alloc = this->GetAllocator(),
             anotherAlloc = another.GetAllocator();
        if constexpr (POCMA)
        { // Then just release self and steal all nodes, no matter allocators
          // are equal or not.
            EraseAllAndReset_();
            alloc = anotherAlloc;
            StealNodes_(another.GetSentinelPtr());
            size_ = std::exchange(another.size_, 0);
        }
        else
        { // Do not propagate, so steal when equal and assign when unequal.
            if (AllocAlwaysEqual || alloc == anotherAlloc)
            {
                EraseAllAndReset_();
                StealNodes_(another.GetSentinelPtr());
                size_ = std::exchange(another.size_, 0);
            }
            else
            {
                Assign(std::move_iterator{ another.begin() },
                       std::move_iterator{ another.end() });
            }
        }

        return *this;
    }

    auto begin() noexcept { return Iterator{ this->GetSentinel().next }; }
    auto end() noexcept { return Iterator{ this->GetSentinelPtr() }; }
    // We will protect data to make it actually immutable, to ensure safety of
    // this const_cast.
    auto cbegin() const noexcept
    {
        return ConstIterator{ const_cast<List *>(this)->begin() };
    }
    auto cend() const noexcept
    {
        return ConstIterator{ const_cast<List *>(this)->end() };
    }
    auto begin() const noexcept { return cbegin(); }
    auto end() const noexcept { return cend(); }
    auto size() const noexcept { return Size(); }
    auto empty() const noexcept { return Empty(); }

    auto Size() const noexcept { return size_; }
    bool Empty() const noexcept { return size_ == 0; }
};

template<typename T, typename Alloc, typename CharT>
struct std::formatter<List<T, Alloc>, CharT>
    : public std::range_formatter<T, CharT>
{
};