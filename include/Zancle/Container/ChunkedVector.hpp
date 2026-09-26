#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Container/Priv/VectorUtils.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/InitializerList.hpp" // IWYU pragma: keep
#include "Zancle/Base/PlacementNew.hpp"
#include "Zancle/Base/Prefetch.hpp"
#include "Zancle/Base/PtrDiffT.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Swap.hpp"

#include "Zancle/Trait/Conditional.hpp"
#include "Zancle/Trait/EnableTrivialRelocation.hpp"


////////////////////////////////////////////////////////////
#define ZA_PREFETCH_FOR_READ(ptr) ZA_PREFETCH(ptr, /* read-only */ 0, /* high temporal locality (L1 cache) */ 3)


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Block-allocated dynamic array with stable element addresses
///
/// ## Layout
///
/// A growable directory (`TItem**`) of fixed-size contiguous blocks.
/// Each block holds `2^BlockShift` elements (default 16 384).
///
///     directory [ blk0* | blk1* | blk2* | ... ]
///                  |        |        |
///                  v        v        v
///               [items]  [items]  [items]   (each block is contiguous)
///
/// `operator[]` splits an index into a block index (high bits) and an
/// intra-block offset (low bits) via shift and mask -- O(1).
///
/// ## Growth
///
/// `pushBack`/`emplaceBack` allocate a new block when the current one
/// fills up, and geometrically grow the directory when it runs out of
/// slots. Existing elements are never relocated, so pointers/references
/// to elements remain valid across insertions.
///
///     pushBack / emplaceBack  -- amortized O(1)
///     operator[]              -- O(1)
///     reserve(n)              -- O(n / blockSize) block allocations
///
/// ## Iteration
///
/// The callback-based API (`forEach`, `forEachIndexed`, `forEachBlock`)
/// loops over each block as a contiguous span, giving the compiler the
/// same optimization opportunities as a flat-array traversal. Each
/// method prefetches the next block pointer before entering the inner
/// loop. Random-access iterators are also provided but cross a block
/// boundary on every `operator[]` / dereference (one extra indirection
/// compared to the callback path).
///
////////////////////////////////////////////////////////////
template <typename TItem, SizeT BlockShift = 14u>
class [[nodiscard]] ChunkedVector
{
    static_assert(BlockShift > 0u);
    static_assert(BlockShift < sizeof(SizeT) * 8u);

private:
    ////////////////////////////////////////////////////////////
    TItem** m_directory{nullptr};    //!< Array of block pointers (length: m_directoryCapacity, used: m_numBlocks)
    SizeT   m_size{0u};              //!< Number of live (constructed) elements
    SizeT   m_numBlocks{0u};         //!< Number of allocated blocks (<= m_directoryCapacity)
    SizeT   m_directoryCapacity{0u}; //!< Allocated slots in the directory array


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] static constexpr SizeT sizeToBlockCount(const SizeT n) noexcept
    {
        return (n + blockMask) >> blockShift;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] static constexpr SizeT minSizeT(const SizeT lhs, const SizeT rhs) noexcept
    {
        return lhs < rhs ? lhs : rhs;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] TItem* blockPtrAt(const SizeT blockIndex) noexcept
    {
        ZA_ASSERT(blockIndex < m_numBlocks);
        return m_directory[blockIndex];
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] const TItem* blockPtrAt(const SizeT blockIndex) const noexcept
    {
        ZA_ASSERT(blockIndex < m_numBlocks);
        return m_directory[blockIndex];
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem* slotPtrUnchecked(const SizeT index) noexcept
    {
        return m_directory[index >> blockShift] + (index & blockMask);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem* slotPtrUnchecked(const SizeT index) const noexcept
    {
        return m_directory[index >> blockShift] + (index & blockMask);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] static TItem* allocateBlock()
    {
        return priv::VectorUtils::allocate<TItem>(blockSize);
    }


    ////////////////////////////////////////////////////////////
    static void deallocateBlock(TItem* const p) noexcept
    {
        priv::VectorUtils::deallocate(p, blockSize);
    }


    ////////////////////////////////////////////////////////////
    void setDirectoryCapacityExact(const SizeT targetCapacity)
    {
        if (targetCapacity == m_directoryCapacity)
            return;

        ZA_ASSERT(targetCapacity >= m_numBlocks);

        if (targetCapacity == 0u)
        {
            priv::VectorUtils::deallocate(m_directory, m_directoryCapacity);
            m_directory         = nullptr;
            m_directoryCapacity = 0u;
            return;
        }

        auto** const newDirectory = priv::VectorUtils::allocate<TItem*>(targetCapacity);

        if (m_numBlocks > 0u)
            priv::VectorUtils::copyRange(newDirectory, m_directory, m_directory + m_numBlocks);

        priv::VectorUtils::deallocate(m_directory, m_directoryCapacity);

        m_directory         = newDirectory;
        m_directoryCapacity = targetCapacity;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::cold, gnu::noinline, gnu::flatten]] void growDirectory(const SizeT targetBlockCount)
    {
        const auto currentCapacity       = m_directoryCapacity;
        const auto geometricGrowthTarget = currentCapacity == 0u ? SizeT{4u} : currentCapacity + (currentCapacity / 2u);
        const auto finalNewCapacity = targetBlockCount > geometricGrowthTarget ? targetBlockCount : geometricGrowthTarget;

        ZA_ASSERT(finalNewCapacity > m_directoryCapacity);
        setDirectoryCapacityExact(finalNewCapacity);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void ensureBlockCapacity(const SizeT targetBlockCount)
    {
        if (targetBlockCount <= m_numBlocks)
            return;

        if (m_directoryCapacity < targetBlockCount) [[unlikely]]
            growDirectory(targetBlockCount);

        while (m_numBlocks < targetBlockCount)
            m_directory[m_numBlocks++] = allocateBlock();
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void ensureCapacityForSize(const SizeT targetSize)
    {
        ensureBlockCapacity(sizeToBlockCount(targetSize));
    }


    ////////////////////////////////////////////////////////////
    void destroyIndexRange(SizeT first, const SizeT last) noexcept
    {
        while (first < last)
        {
            const auto   blockIndex  = first >> blockShift;
            const auto   blockOffset = first & blockMask;
            const auto   chunk       = minSizeT(last - first, blockSize - blockOffset);
            TItem* const blockPtr    = m_directory[blockIndex] + blockOffset;

            priv::VectorUtils::destroyRange(blockPtr, blockPtr + chunk);
            first += chunk;
        }
    }


    ////////////////////////////////////////////////////////////
    void copyFromContiguousRange(SizeT targetIndex, const TItem* src, SizeT count)
    {
        while (count > 0u)
        {
            const auto   blockIndex  = targetIndex >> blockShift;
            const auto   blockOffset = targetIndex & blockMask;
            const auto   chunk       = minSizeT(count, blockSize - blockOffset);
            TItem* const blockPtr    = m_directory[blockIndex] + blockOffset;

            priv::VectorUtils::copyRange(blockPtr, src, src + chunk);

            targetIndex += chunk;
            src += chunk;
            count -= chunk;
        }
    }


    ////////////////////////////////////////////////////////////
    void copyFromOther(const ChunkedVector& rhs)
    {
        if (rhs.m_size == 0u)
        {
            m_size = 0u;
            return;
        }

        ensureCapacityForSize(rhs.m_size);

        const SizeT fullBlocks = rhs.m_size >> blockShift;
        const SizeT tail       = rhs.m_size & blockMask;

        for (SizeT blockIndex = 0u; blockIndex < fullBlocks; ++blockIndex)
            priv::VectorUtils::copyRange(m_directory[blockIndex],
                                         rhs.m_directory[blockIndex],
                                         rhs.m_directory[blockIndex] + blockSize);

        if (tail > 0u)
            priv::VectorUtils::copyRange(m_directory[fullBlocks],
                                         rhs.m_directory[fullBlocks],
                                         rhs.m_directory[fullBlocks] + tail);

        m_size = rhs.m_size;
    }


    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    void constructRange(SizeT first, const SizeT last, Ts&&... xs)
    {
        while (first < last)
        {
            const auto   blockIndex  = first >> blockShift;
            const auto   blockOffset = first & blockMask;
            const auto   chunk       = minSizeT(last - first, blockSize - blockOffset);
            TItem* const blockPtr    = m_directory[blockIndex] + blockOffset;

            for (SizeT i = 0u; i < chunk; ++i)
                ZA_PLACEMENT_NEW(blockPtr + i) TItem(xs...); // intentionally not forwarding

            first += chunk;
        }
    }


public:
    ////////////////////////////////////////////////////////////
    ZA_ENABLE_TRIVIAL_RELOCATION;


    ////////////////////////////////////////////////////////////
    static constexpr SizeT blockShift = BlockShift;
    static constexpr SizeT blockSize  = SizeT{1u} << BlockShift;
    static constexpr SizeT blockMask  = blockSize - 1u;


    ////////////////////////////////////////////////////////////
    using value_type      = TItem;
    using pointer         = TItem*;
    using const_pointer   = const TItem*;
    using reference       = TItem&;
    using const_reference = const TItem&;
    using size_type       = SizeT;
    using difference_type = PtrDiffT;


    ////////////////////////////////////////////////////////////
    template <bool IsConst>
    class IteratorImpl
    {
        friend IteratorImpl<!IsConst>;

    public:
        using value_type      = TItem;
        using pointer         = Conditional<IsConst, const TItem*, TItem*>;
        using reference       = Conditional<IsConst, const TItem&, TItem&>;
        using difference_type = PtrDiffT;
        using owner_pointer   = Conditional<IsConst, const ChunkedVector*, ChunkedVector*>;

    private:
        owner_pointer m_owner{nullptr};
        SizeT         m_index{0u};

    public:
        [[nodiscard]] IteratorImpl()                               = default;
        [[nodiscard]] IteratorImpl(const IteratorImpl&)            = default;
        [[nodiscard]] IteratorImpl& operator=(const IteratorImpl&) = default;


        [[nodiscard, gnu::always_inline]] IteratorImpl(const IteratorImpl<false>& rhs) noexcept
            requires(IsConst)
            : m_owner{rhs.m_owner}, m_index{rhs.m_index}
        {
        }


        [[nodiscard, gnu::always_inline]] IteratorImpl(owner_pointer owner, const SizeT index) noexcept :
            m_owner{owner},
            m_index{index}
        {
        }


        [[nodiscard, gnu::always_inline]] reference operator*() const noexcept
        {
            return (*m_owner)[m_index];
        }


        [[nodiscard, gnu::always_inline]] pointer operator->() const noexcept
        {
            return &(*m_owner)[m_index];
        }


        [[nodiscard, gnu::always_inline]] reference operator[](const difference_type delta) const noexcept
        {
            return (*m_owner)[static_cast<SizeT>(static_cast<difference_type>(m_index) + delta)];
        }


        [[gnu::always_inline]] IteratorImpl& operator++() noexcept
        {
            ++m_index;
            return *this;
        }


        [[gnu::always_inline]] IteratorImpl operator++(int) noexcept
        {
            const IteratorImpl result = *this;
            ++m_index;
            return result;
        }


        [[gnu::always_inline]] IteratorImpl& operator--() noexcept
        {
            --m_index;
            return *this;
        }


        [[gnu::always_inline]] IteratorImpl operator--(int) noexcept
        {
            const IteratorImpl result = *this;
            --m_index;
            return result;
        }


        [[gnu::always_inline]] IteratorImpl& operator+=(const difference_type delta) noexcept
        {
            m_index = static_cast<SizeT>(static_cast<difference_type>(m_index) + delta);
            return *this;
        }


        [[gnu::always_inline]] IteratorImpl& operator-=(const difference_type delta) noexcept
        {
            return *this += -delta;
        }


        [[nodiscard, gnu::always_inline]] IteratorImpl operator+(const difference_type delta) const noexcept
        {
            IteratorImpl result = *this;
            result += delta;
            return result;
        }


        [[nodiscard, gnu::always_inline]] IteratorImpl operator-(const difference_type delta) const noexcept
        {
            IteratorImpl result = *this;
            result -= delta;
            return result;
        }


        [[nodiscard, gnu::always_inline]] difference_type operator-(const IteratorImpl& rhs) const noexcept
        {
            ZA_ASSERT(m_owner == rhs.m_owner);
            return static_cast<difference_type>(m_index) - static_cast<difference_type>(rhs.m_index);
        }


        [[nodiscard, gnu::always_inline]] bool operator==(const IteratorImpl& rhs) const noexcept
        {
            return m_owner == rhs.m_owner && m_index == rhs.m_index;
        }


        [[nodiscard, gnu::always_inline]] bool operator<(const IteratorImpl& rhs) const noexcept
        {
            ZA_ASSERT(m_owner == rhs.m_owner);
            return m_index < rhs.m_index;
        }


        [[nodiscard, gnu::always_inline]] bool operator<=(const IteratorImpl& rhs) const noexcept
        {
            return !(rhs < *this);
        }


        [[nodiscard, gnu::always_inline]] bool operator>(const IteratorImpl& rhs) const noexcept
        {
            return rhs < *this;
        }


        [[nodiscard, gnu::always_inline]] bool operator>=(const IteratorImpl& rhs) const noexcept
        {
            return !(*this < rhs);
        }
    };


    ////////////////////////////////////////////////////////////
    using Iterator       = IteratorImpl<false>;
    using ConstIterator  = IteratorImpl<true>;
    using iterator       = Iterator;
    using const_iterator = ConstIterator;


    ////////////////////////////////////////////////////////////
    [[nodiscard]] ChunkedVector() = default;


    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit ChunkedVector(const SizeT initialSize)
    {
        resize(initialSize);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit ChunkedVector(const SizeT initialSize, const TItem& value)
    {
        resize(initialSize, value);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit ChunkedVector(const TItem* const srcBegin, const TItem* const srcEnd)
    {
        ZA_ASSERT(srcBegin <= srcEnd);
        const auto srcCount = static_cast<SizeT>(srcEnd - srcBegin);

        if (srcCount == 0u)
            return;

        reserve(srcCount);
        copyFromContiguousRange(0u, srcBegin, srcCount);
        m_size = srcCount;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] /* implicit */ ChunkedVector(const std::initializer_list<TItem> iList) :
        ChunkedVector(iList.begin(), iList.end())
    {
    }


    ////////////////////////////////////////////////////////////
    ~ChunkedVector()
    {
        clear();

        for (SizeT i = 0u; i < m_numBlocks; ++i)
            deallocateBlock(m_directory[i]);

        priv::VectorUtils::deallocate(m_directory, m_directoryCapacity);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] ChunkedVector(const ChunkedVector& rhs)
    {
        copyFromOther(rhs);
    }


    ////////////////////////////////////////////////////////////
    ChunkedVector& operator=(const ChunkedVector& rhs)
    {
        if (this == &rhs)
            return *this;

        clear();
        copyFromOther(rhs);

        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] ChunkedVector(ChunkedVector&& rhs) noexcept :
        m_directory{rhs.m_directory},
        m_size{rhs.m_size},
        m_numBlocks{rhs.m_numBlocks},
        m_directoryCapacity{rhs.m_directoryCapacity}
    {
        rhs.m_directory         = nullptr;
        rhs.m_size              = 0u;
        rhs.m_numBlocks         = 0u;
        rhs.m_directoryCapacity = 0u;
    }


    ////////////////////////////////////////////////////////////
    ChunkedVector& operator=(ChunkedVector&& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        clear();

        for (SizeT i = 0u; i < m_numBlocks; ++i)
            deallocateBlock(m_directory[i]);

        priv::VectorUtils::deallocate(m_directory, m_directoryCapacity);

        m_directory         = rhs.m_directory;
        m_size              = rhs.m_size;
        m_numBlocks         = rhs.m_numBlocks;
        m_directoryCapacity = rhs.m_directoryCapacity;

        rhs.m_directory         = nullptr;
        rhs.m_size              = 0u;
        rhs.m_numBlocks         = 0u;
        rhs.m_directoryCapacity = 0u;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void resize(const SizeT newSize, auto&&... args)
    {
        const auto oldSize = m_size;

        if (newSize > oldSize)
        {
            reserve(newSize);
            constructRange(oldSize, newSize, args...); // intentionally not forwarding
        }
        else if (newSize < oldSize)
        {
            destroyIndexRange(newSize, oldSize);
        }

        m_size = newSize;
    }


    ////////////////////////////////////////////////////////////
    template <typename T = TItem>
    [[gnu::always_inline, gnu::flatten]] TItem& pushBack(T&& x)
    {
        return emplaceBack(static_cast<T&&>(x));
    }


    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    [[gnu::always_inline, gnu::flatten]] TItem& emplaceBack(Ts&&... xs)
    {
        if (m_size >= capacity()) [[unlikely]]
            reserveMore(1u);

        TItem* const slot = slotPtrUnchecked(m_size);
        ++m_size;

        return *(ZA_PLACEMENT_NEW(slot) TItem(static_cast<Ts&&>(xs)...));
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void shrinkToFit()
    {
        const SizeT requiredBlocks = sizeToBlockCount(m_size);

        if (requiredBlocks < m_numBlocks)
        {
            for (SizeT blockIndex = requiredBlocks; blockIndex < m_numBlocks; ++blockIndex)
                deallocateBlock(m_directory[blockIndex]);

            m_numBlocks = requiredBlocks;
        }

        setDirectoryCapacityExact(requiredBlocks);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void reserve(const SizeT targetCapacity)
    {
        ensureCapacityForSize(targetCapacity);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void reserveMore(const SizeT n)
    {
        reserve(m_size + n);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void unsafeEmplaceBackRange(const TItem* const ptr, const SizeT count) noexcept
    {
        ZA_ASSERT(ptr != nullptr);
        ZA_ASSERT(m_size + count <= capacity());

        copyFromContiguousRange(m_size, ptr, count);
        m_size += count;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void clear() noexcept
    {
        destroyIndexRange(0u, m_size);
        m_size = 0u;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] SizeT size() const noexcept
    {
        return m_size;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] SizeT capacity() const noexcept
    {
        return m_numBlocks * blockSize;
    }


    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    [[gnu::always_inline]] TItem& unsafeEmplaceBack(Ts&&... xs)
    {
        ZA_ASSERT(m_size < capacity());

        TItem* const slot = slotPtrUnchecked(m_size);
        ++m_size;

        return *(ZA_PLACEMENT_NEW(slot) TItem(static_cast<Ts&&>(xs)...));
    }


    ////////////////////////////////////////////////////////////
    template <typename... TItems>
    [[gnu::always_inline]] void unsafePushBackMultiple(TItems&&... items)
    {
        ZA_ASSERT(m_size + sizeof...(items) <= capacity());
        (..., unsafeEmplaceBack(static_cast<TItems&&>(items)));
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void unsafeSetSize(const SizeT newSize) noexcept
    {
        ZA_ASSERT(newSize <= capacity());
        m_size = newSize;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void popBack() noexcept
    {
        ZA_ASSERT(!empty());
        --m_size;
        priv::VectorUtils::destroyRange(slotPtrUnchecked(m_size), slotPtrUnchecked(m_size) + 1);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void swap(ChunkedVector& rhs) noexcept
    {
        if (this == &rhs)
            return;

        za::genericSwap(m_directory, rhs.m_directory);
        za::genericSwap(m_size, rhs.m_size);
        za::genericSwap(m_numBlocks, rhs.m_numBlocks);
        za::genericSwap(m_directoryCapacity, rhs.m_directoryCapacity);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem& operator[](const SizeT i) noexcept
    {
        ZA_ASSERT(i < m_size);
        return *slotPtrUnchecked(i);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem& operator[](const SizeT i) const noexcept
    {
        ZA_ASSERT(i < m_size);
        return *slotPtrUnchecked(i);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] iterator begin() noexcept
    {
        return iterator{this, 0u};
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] const_iterator begin() const noexcept
    {
        return const_iterator{this, 0u};
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] iterator end() noexcept
    {
        return iterator{this, m_size};
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] const_iterator end() const noexcept
    {
        return const_iterator{this, m_size};
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] const_iterator cbegin() const noexcept
    {
        return begin();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] const_iterator cend() const noexcept
    {
        return end();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] bool empty() const noexcept
    {
        return m_size == 0u;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool operator==(const ChunkedVector& rhs) const
    {
        if (this == &rhs)
            return true;

        if (m_size != rhs.m_size)
            return false;

        const SizeT fullBlocks = m_size >> blockShift;
        const SizeT tail       = m_size & blockMask;

        for (SizeT blockIndex = 0u; blockIndex < fullBlocks; ++blockIndex)
        {
            const TItem* const lp = m_directory[blockIndex];
            const TItem* const rp = rhs.m_directory[blockIndex];

            for (SizeT i = 0u; i < blockSize; ++i)
                if (lp[i] != rp[i])
                    return false;
        }

        if (tail > 0u)
        {
            const TItem* const lp = m_directory[fullBlocks];
            const TItem* const rp = rhs.m_directory[fullBlocks];

            for (SizeT i = 0u; i < tail; ++i)
                if (lp[i] != rp[i])
                    return false;
        }

        return true;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] friend void swap(ChunkedVector& lhs, ChunkedVector& rhs) noexcept
    {
        lhs.swap(rhs);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem& front() noexcept
    {
        ZA_ASSERT(!empty());
        return *slotPtrUnchecked(0u);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem& front() const noexcept
    {
        ZA_ASSERT(!empty());
        return *slotPtrUnchecked(0u);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem& back() noexcept
    {
        ZA_ASSERT(!empty());
        return *slotPtrUnchecked(m_size - 1u);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem& back() const noexcept
    {
        ZA_ASSERT(!empty());
        return *slotPtrUnchecked(m_size - 1u);
    }


    ////////////////////////////////////////////////////////////
    template <typename F>
    [[gnu::always_inline]] void forEach(this auto&& self, F&& fn)
    {
        if (self.m_size == 0u)
            return;

        const SizeT fullBlocks = self.m_size >> blockShift;
        const SizeT tail       = self.m_size & blockMask;

        for (SizeT blockIndex = 0u; blockIndex < fullBlocks; ++blockIndex)
        {
            auto* const blockPtr = self.blockPtrAt(blockIndex);

            if (blockIndex + 1u < self.m_numBlocks)
                ZA_PREFETCH_FOR_READ(self.m_directory[blockIndex + 1u]);

            for (SizeT i = 0u; i < blockSize; ++i)
                fn(blockPtr[i]);
        }

        if (tail > 0u)
        {
            auto* const blockPtr = self.blockPtrAt(fullBlocks);

            for (SizeT i = 0u; i < tail; ++i)
                fn(blockPtr[i]);
        }
    }


    ////////////////////////////////////////////////////////////
    template <typename F>
    [[gnu::always_inline]] void forEachIndexed(this auto&& self, F&& fn)
    {
        if (self.m_size == 0u)
            return;

        const SizeT fullBlocks = self.m_size >> blockShift;
        const SizeT tail       = self.m_size & blockMask;
        SizeT       baseIndex  = 0u;

        for (SizeT blockIndex = 0u; blockIndex < fullBlocks; ++blockIndex)
        {
            auto* const blockPtr = self.blockPtrAt(blockIndex);

            if (blockIndex + 1u < self.m_numBlocks)
                ZA_PREFETCH_FOR_READ(self.m_directory[blockIndex + 1u]);

            for (SizeT i = 0u; i < blockSize; ++i)
                fn(baseIndex + i, blockPtr[i]);

            baseIndex += blockSize;
        }

        if (tail > 0u)
        {
            auto* const blockPtr = self.blockPtrAt(fullBlocks);

            for (SizeT i = 0u; i < tail; ++i)
                fn(baseIndex + i, blockPtr[i]);
        }
    }


    ////////////////////////////////////////////////////////////
    template <typename F>
    [[gnu::always_inline]] void forEachBlock(this auto&& self, F&& fn)
    {
        if (self.m_size == 0u)
            return;

        const SizeT fullBlocks = self.m_size >> blockShift;
        const SizeT tail       = self.m_size & blockMask;

        for (SizeT blockIndex = 0u; blockIndex < fullBlocks; ++blockIndex)
        {
            if (blockIndex + 1u < self.m_numBlocks)
                ZA_PREFETCH_FOR_READ(self.m_directory[blockIndex + 1u]);

            auto* const blockPtr = self.blockPtrAt(blockIndex);
            fn(blockPtr, blockPtr + blockSize);
        }

        if (tail > 0u)
        {
            auto* const blockPtr = self.blockPtrAt(fullBlocks);
            fn(blockPtr, blockPtr + tail);
        }
    }


    ////////////////////////////////////////////////////////////
    template <typename TPredicate>
    [[nodiscard]] auto findIf(this auto&& self, TPredicate&& predicate) -> decltype(self.blockPtrAt(0u))
    {
        const SizeT fullBlocks = self.m_size >> blockShift;
        const SizeT tail       = self.m_size & blockMask;

        for (SizeT blockIndex = 0u; blockIndex < fullBlocks; ++blockIndex)
        {
            auto* const blockPtr = self.blockPtrAt(blockIndex);

            if (blockIndex + 1u < self.m_numBlocks)
                ZA_PREFETCH_FOR_READ(self.m_directory[blockIndex + 1u]);

            for (SizeT i = 0u; i < blockSize; ++i)
                if (predicate(blockPtr[i]))
                    return blockPtr + i;
        }

        if (tail > 0u)
        {
            auto* const blockPtr = self.blockPtrAt(fullBlocks);

            for (SizeT i = 0u; i < tail; ++i)
                if (predicate(blockPtr[i]))
                    return blockPtr + i;
        }

        return nullptr;
    }


    ////////////////////////////////////////////////////////////
    template <typename TResult, typename F>
    [[nodiscard]] TResult reduce(TResult init, F&& fn) const
    {
        forEach([&](const TItem& x) { init = fn(init, x); });
        return init;
    }
};

} // namespace za


////////////////////////////////////////////////////////////
#undef ZA_PREFETCH_FOR_READ
