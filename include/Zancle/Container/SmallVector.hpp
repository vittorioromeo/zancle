#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Container/Priv/VectorUtils.hpp"

#include "Zancle/Math/MinMaxMacros.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/AssertAndAssume.hpp"
#include "Zancle/Base/InitializerList.hpp" // IWYU pragma: keep
#include "Zancle/Base/PlacementNew.hpp"
#include "Zancle/Base/PtrDiffT.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Swap.hpp"

#include "Zancle/Trait/EnableTrivialRelocation.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Vector with inline storage for the first `N` elements (small buffer optimization)
///
/// Behaves like `Vector` but reserves space inside the object for `N`
/// elements, avoiding any heap allocation when `size() <= N`. Once the
/// vector grows past `N`, storage is moved to the heap and the inline
/// buffer becomes unused until `shrinkToFit()` brings the size back
/// below the threshold.
///
/// Implementation note: `m_heapData == nullptr` encodes "currently
/// inline" -- after a `memcpy`, the recomputed inline-storage pointer
/// still addresses the new object's own buffer. This makes the pointer
/// bookkeeping relocation-safe, but `SmallVector` is only trivially
/// relocatable when `TItem` itself is, because in inline mode the
/// elements are stored inside the object and a `memcpy` would bypass
/// their move constructors.
///
////////////////////////////////////////////////////////////
template <typename TItem, SizeT N>
class [[nodiscard]] SmallVector // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    static_assert(N > 0);

private:
    ////////////////////////////////////////////////////////////
    // `m_heapData == nullptr` means "using inline storage".
    // This makes SmallVector trivially relocatable: after memcpy,
    // nullptr still means "use my own inline storage" (recomputed from `this`).
    TItem* m_heapData{nullptr};
    SizeT  m_size{0u};
    SizeT  m_capacity{N};
    alignas(TItem) unsigned char m_inlineStorage[sizeof(TItem) * N];


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem* getInlineStorage() noexcept
    {
        return reinterpret_cast<TItem*>(m_inlineStorage);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem* getInlineStorage() const noexcept
    {
        return reinterpret_cast<const TItem*>(m_inlineStorage);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::cold, gnu::noinline, gnu::flatten]] void reserveImpl(const SizeT targetCapacity)
    {
        const auto currentCapacity = capacity();
        const auto geometricGrowthTarget = currentCapacity + (currentCapacity / 2u); // Equivalent to `currentCapacity * 1.5`
        const auto finalNewCapacity = ZA_MAX(targetCapacity, geometricGrowthTarget);

        ZA_ASSERT(finalNewCapacity > capacity()); // Should only be called to grow

        auto*      newData = priv::VectorUtils::allocate<TItem>(finalNewCapacity);
        const auto oldSize = m_size;

        TItem* const oldData = data();

        if (oldSize > 0u)
            priv::VectorUtils::relocateRange(newData, oldData, oldData + oldSize);

        if (isHeap())
            priv::VectorUtils::deallocate(m_heapData, currentCapacity);

        m_heapData = newData;
        m_capacity = finalNewCapacity;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Grow the buffer and construct a new element at `insertIndex`
    ///
    /// Allocates a new buffer, constructs the element at its target
    /// position while the old buffer is still alive (so references
    /// into the old buffer remain valid), then relocates existing
    /// elements around it.
    ///
    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    [[gnu::cold, gnu::noinline, gnu::returns_nonnull]] TItem* growAndEmplace(const SizeT insertIndex, Ts&&... xs)
    {
        const auto oldSize               = m_size;
        const auto currentCapacity       = m_capacity;
        const auto geometricGrowthTarget = currentCapacity + (currentCapacity / 2u);
        const auto finalNewCapacity      = ZA_MAX(oldSize + 1, geometricGrowthTarget);

        auto* newData = priv::VectorUtils::allocate<TItem>(finalNewCapacity);
        ZA_ASSERT_AND_ASSUME(newData != nullptr);

        // Construct new element first (old buffer still alive, references valid).
        ZA_PLACEMENT_NEW(newData + insertIndex) TItem(static_cast<Ts&&>(xs)...);

        // Relocate old elements around the newly constructed element.
        TItem* const oldData = data();
        if (oldSize > 0u)
        {
            priv::VectorUtils::relocateRange(newData, oldData, oldData + insertIndex);
            priv::VectorUtils::relocateRange(newData + insertIndex + 1, oldData + insertIndex, oldData + oldSize);
        }

        if (isHeap())
            priv::VectorUtils::deallocate(m_heapData, currentCapacity);

        m_heapData = newData;
        m_capacity = finalNewCapacity;
        m_size     = oldSize + 1;

        return newData + insertIndex;
    }


public:
    ////////////////////////////////////////////////////////////
    // Trivially relocatable only when `TItem` is: in inline mode the
    // elements live inside the object, so a `memcpy` of the `SmallVector`
    // byte-copies them without running their move constructors -- which is
    // only valid for trivially relocatable elements. The `nullptr`-means-
    // inline bookkeeping survives the `memcpy` regardless (it is recomputed
    // from the new `this`), so that is not the constraint here.
    ZA_ENABLE_TRIVIAL_RELOCATION_IF(ZA_IS_TRIVIALLY_RELOCATABLE(TItem));


    ////////////////////////////////////////////////////////////
    using value_type      = TItem;
    using pointer         = TItem*;
    using const_pointer   = const TItem*;
    using reference       = TItem&;
    using const_reference = const TItem&;
    using size_type       = SizeT;
    using difference_type = PtrDiffT;
    using iterator        = TItem*;
    using const_iterator  = const TItem*;


    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, cppcoreguidelines-pro-type-reinterpret-cast)
    [[nodiscard]] SmallVector() = default;


    ////////////////////////////////////////////////////////////
    ~SmallVector()
    {
        priv::VectorUtils::destroyRange(data(), data() + m_size);

        if (isHeap())
            priv::VectorUtils::deallocate(m_heapData, m_capacity);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit SmallVector(const SizeT initialSize, const TItem& value) : SmallVector{}
    {
        if (initialSize == 0u)
            return;

        reserve(initialSize);

        priv::VectorUtils::copyConstructRange(data(), data() + initialSize, value);
        m_size = initialSize;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit SmallVector(const SizeT initialSize) : SmallVector{}
    {
        if (initialSize == 0u)
            return;

        reserve(initialSize);

        priv::VectorUtils::defaultConstructRange(data(), data() + initialSize);
        m_size = initialSize;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit SmallVector(const TItem* const srcBegin, const TItem* const srcEnd) : SmallVector{}
    {
        ZA_ASSERT(srcBegin <= srcEnd);
        const auto srcCount = static_cast<SizeT>(srcEnd - srcBegin);

        if (srcCount == 0u)
            return;

        reserve(srcCount);
        priv::VectorUtils::copyRange(data(), srcBegin, srcEnd);
        m_size = srcCount;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] /* implicit */ SmallVector(const std::initializer_list<TItem> iList) :
        SmallVector(iList.begin(), iList.end())
    {
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] SmallVector(const SmallVector& rhs) : SmallVector{}
    {
        reserve(rhs.m_size);
        priv::VectorUtils::copyRange(data(), rhs.data(), rhs.data() + rhs.m_size);
        m_size = rhs.m_size;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] bool isHeap() const noexcept
    {
        return m_heapData != nullptr;
    }


    ////////////////////////////////////////////////////////////
    SmallVector& operator=(const SmallVector& rhs)
    {
        if (this == &rhs)
            return *this;

        clear();
        reserve(rhs.m_size);
        priv::VectorUtils::copyRange(data(), rhs.data(), rhs.data() + rhs.m_size);

        m_size = rhs.m_size;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] SmallVector(SmallVector&& rhs) noexcept : SmallVector{}
    {
        if (rhs.isHeap())
        {
            // Steal the heap buffer
            m_heapData = rhs.m_heapData;
            m_size     = rhs.m_size;
            m_capacity = rhs.m_capacity;

            // Reset RHS to inline mode
            rhs.m_heapData = nullptr;
            rhs.m_size     = 0u;
            rhs.m_capacity = N;
        }
        else
        {
            // Move elements from RHS inline storage to LHS inline storage
            // No need to reserve, capacity is N
            priv::VectorUtils::relocateRange(getInlineStorage(), rhs.getInlineStorage(), rhs.getInlineStorage() + rhs.m_size);
            m_size     = rhs.m_size;
            rhs.m_size = 0u;
        }
    }


    ////////////////////////////////////////////////////////////
    SmallVector& operator=(SmallVector&& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        // Destroy current elements
        clear();

        if (rhs.isHeap())
        {
            // If we have a heap buffer, deallocate it before stealing
            if (isHeap())
                priv::VectorUtils::deallocate(m_heapData, m_capacity);

            // Steal pointers
            m_heapData = rhs.m_heapData;
            m_size     = rhs.m_size;
            m_capacity = rhs.m_capacity;

            // Reset RHS
            rhs.m_heapData = nullptr;
            rhs.m_size     = 0u;
            rhs.m_capacity = N;

            return *this;
        }

        // Optimization: Reuse existing heap buffer
        if (isHeap() && m_capacity >= rhs.m_size)
        {
            priv::VectorUtils::relocateRange(m_heapData, rhs.data(), rhs.data() + rhs.m_size);
            m_size = rhs.m_size;

            rhs.m_size = 0u;

            return *this;
        }

        if (isHeap())
        {
            priv::VectorUtils::deallocate(m_heapData, m_capacity);
            m_heapData = nullptr;
            m_capacity = N;
        }

        m_size = 0u; // Reset size

        // Move elements
        priv::VectorUtils::relocateRange(data(), rhs.data(), rhs.data() + rhs.m_size);
        m_size = rhs.m_size;

        rhs.m_size = 0u;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    /// \warning When growing, `args...` must not reference an element of
    ///          `*this`: the growth may reallocate (and, when escaping the
    ///          inline buffer, relocate) before constructing the new
    ///          elements, leaving such a reference dangling. Debug-asserted.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void resize(const SizeT newSize, auto&&... args)
    {
        const auto oldSize = m_size;

        if (newSize > oldSize)
        {
            // See \warning above: fill args must not alias existing elements.
            ZA_ASSERT((priv::VectorUtils::isOutsideStorage(data(), data() + m_size, &args) && ...));

            reserve(newSize);

            TItem* const d = data();
            for (auto* p = d + oldSize; p != d + newSize; ++p)
                ZA_PLACEMENT_NEW(p) TItem(args...); // intentionally not forwarding
        }
        else
        {
            TItem* const d = data();
            priv::VectorUtils::destroyRange(d + newSize, d + oldSize);
        }

        m_size = newSize;
    }


    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    [[gnu::always_inline]] TItem* emplace(TItem* const pos, Ts&&... xs)
    {
        ZA_ASSERT(pos >= begin() && pos <= end());

        const auto index = static_cast<SizeT>(pos - data());

        if (m_size >= m_capacity) [[unlikely]]
            return growAndEmplace(index, static_cast<Ts&&>(xs)...);

        TItem* const d = data();

        if (pos == d + m_size) // Append at end: no shift, no aliasing risk.
        {
            ZA_PLACEMENT_NEW(d + m_size) TItem(static_cast<Ts&&>(xs)...);
            ++m_size;
            return d + index;
        }

        // Construct a copy first to handle self-aliasing (`makeHole` shifts elements in-place,
        // which invalidates any reference into the shifted region).
        TItem        copy(static_cast<Ts&&>(xs)...);
        TItem* const currentPos = d + index;
        priv::VectorUtils::makeHole(currentPos, d + m_size);
        ZA_PLACEMENT_NEW(currentPos) TItem(static_cast<TItem&&>(copy));

        ++m_size;
        return d + index;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] TItem* insert(TItem* const pos, const TItem& value)
    {
        return emplace(pos, value);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] TItem* insert(TItem* const pos, TItem&& value)
    {
        return emplace(pos, static_cast<TItem&&>(value));
    }


    ////////////////////////////////////////////////////////////
    template <typename T = TItem>
    [[gnu::always_inline, gnu::flatten]] TItem& pushBack(T&& x)
    {
        if (m_size < m_capacity) [[likely]]
            return unsafeEmplaceBack(static_cast<T&&>(x));

        return *growAndEmplace(m_size, static_cast<T&&>(x));
    }


    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    [[gnu::always_inline, gnu::flatten]] TItem& emplaceBack(Ts&&... xs)
    {
        if (m_size < m_capacity) [[likely]]
            return unsafeEmplaceBack(static_cast<Ts&&>(xs)...);

        return *growAndEmplace(m_size, static_cast<Ts&&>(xs)...);
    }


    ////////////////////////////////////////////////////////////
    void shrinkToFit()
    {
        if (!isHeap())
            return;

        const SizeT currentSize = m_size;

        if (currentSize <= N)
        {
            // Move back to inline storage
            TItem* const oldData     = m_heapData;
            const auto   oldCapacity = m_capacity;

            // Setup inline pointers
            TItem* const inlinePtr = getInlineStorage();

            priv::VectorUtils::relocateRange(inlinePtr, oldData, oldData + currentSize);
            priv::VectorUtils::deallocate(oldData, oldCapacity);

            m_heapData = nullptr;
            m_capacity = N;
        }
        else if (currentSize < m_capacity)
        {
            // Shrink heap allocation
            auto* const newData     = priv::VectorUtils::allocate<TItem>(currentSize);
            const auto  oldCapacity = m_capacity;

            priv::VectorUtils::relocateRange(newData, m_heapData, m_heapData + currentSize);
            priv::VectorUtils::deallocate(m_heapData, oldCapacity);

            m_heapData = newData;
            m_capacity = currentSize;
        }
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void reserve(const SizeT targetCapacity)
    {
        if (m_capacity < targetCapacity) [[unlikely]]
            reserveImpl(targetCapacity);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void reserveMore(const SizeT n)
    {
        reserve(m_size + n);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void unsafeEmplaceBackRange(const TItem* const ptr, const SizeT count) noexcept
    {
        ZA_ASSERT(m_size + count <= m_capacity);

        TItem* const d = data();
        priv::VectorUtils::copyRange(d + m_size, ptr, ptr + count);
        m_size += count;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void clear() noexcept
    {
        TItem* const d = data();
        priv::VectorUtils::destroyRange(d, d + m_size);
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
        return m_capacity;
    }


    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    [[gnu::always_inline]] TItem& unsafeEmplaceBack(Ts&&... xs)
    {
        ZA_ASSERT(m_size < m_capacity);

        TItem* const slot = data() + m_size;
        ++m_size;

        return *(ZA_PLACEMENT_NEW(slot) TItem(static_cast<Ts&&>(xs)...));
    }


    ////////////////////////////////////////////////////////////
    TItem* erase(TItem* const it)
    {
        ZA_ASSERT(it >= begin() && it < end());

        TItem* const newEnd = priv::VectorUtils::eraseImpl(end(), it);
        m_size              = static_cast<SizeT>(newEnd - data());
        return it;
    }


    ////////////////////////////////////////////////////////////
    TItem* erase(TItem* const first, TItem* const last)
    {
        ZA_ASSERT(first <= last);

        if (first == last)
            return first; // No elements to erase

        TItem* const newEnd = priv::VectorUtils::eraseRangeImpl(end(), first, last);
        m_size              = static_cast<SizeT>(newEnd - data());

        // Return an iterator to the element that now occupies the position
        // where the first erased element (`first`) was. This is `first` itself,
        // as elements were shifted into this position, or it's the new `end()`.
        return first;
    }


    ////////////////////////////////////////////////////////////
    template <typename... TItems>
    [[gnu::always_inline]] void unsafePushBackMultiple(TItems&&... items)
    {
        ZA_ASSERT(m_size + sizeof...(items) <= m_capacity);

        TItem* d = data();
        (..., (ZA_PLACEMENT_NEW(d + m_size) TItem(static_cast<TItems&&>(items)), ++m_size));
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem* data() noexcept
    {
        return m_heapData != nullptr ? m_heapData : getInlineStorage();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem* data() const noexcept
    {
        return m_heapData != nullptr ? m_heapData : getInlineStorage();
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void unsafeSetSize(SizeT newSize) noexcept
    {
        ZA_ASSERT(newSize <= m_capacity);
        m_size = newSize;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void popBack() noexcept
    {
        ZA_ASSERT(!empty());
        --m_size;

        if constexpr (!ZA_IS_TRIVIALLY_DESTRUCTIBLE(TItem))
            (data() + m_size)->~TItem();
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void swap(SmallVector& rhs) noexcept
    {
        if (this == &rhs)
            return;

        const bool lhsHeap = isHeap();
        const bool rhsHeap = rhs.isHeap();

        if (lhsHeap && rhsHeap)
        {
            // Both are heap allocated: standard pointer swap
            za::genericSwap(m_heapData, rhs.m_heapData);
            za::genericSwap(m_size, rhs.m_size);
            za::genericSwap(m_capacity, rhs.m_capacity);

            return;
        }

        if (!lhsHeap && !rhsHeap)
        {
            // Both are inline: use element-wise swap
            // (swapUnequalRanges also swaps the sizes via the SizeT& references)
            priv::VectorUtils::swapUnequalRanges(data(), m_size, rhs.data(), rhs.m_size);

            return;
        }

        // One is heap, one is inline
        SmallVector* const heapVec   = lhsHeap ? this : &rhs;
        SmallVector* const inlineVec = lhsHeap ? &rhs : this;

        // 1. Move elements from inlineVec's inline storage to heapVec's inline storage
        TItem* const heapVecInline = heapVec->getInlineStorage();
        priv::VectorUtils::relocateRange(heapVecInline,
                                         inlineVec->getInlineStorage(),
                                         inlineVec->getInlineStorage() + inlineVec->m_size);

        // 2. Capture heapVec's heap state
        TItem* const savedHeapData     = heapVec->m_heapData;
        const SizeT  savedHeapSize     = heapVec->m_size;
        const SizeT  savedHeapCapacity = heapVec->m_capacity;

        // 3. Point heapVec to its own inline storage (which now holds the data from inlineVec)
        heapVec->m_heapData = nullptr;
        heapVec->m_size     = inlineVec->m_size;
        heapVec->m_capacity = N;

        // 4. Point inlineVec to the captured heap buffer
        inlineVec->m_heapData = savedHeapData;
        inlineVec->m_size     = savedHeapSize;
        inlineVec->m_capacity = savedHeapCapacity;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem& front() noexcept
    {
        ZA_ASSERT(!empty());
        return *data();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem& front() const noexcept
    {
        ZA_ASSERT(!empty());
        return *data();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] TItem& back() noexcept
    {
        ZA_ASSERT(!empty());
        return *(data() + m_size - 1u);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] const TItem& back() const noexcept
    {
        ZA_ASSERT(!empty());
        return *(data() + m_size - 1u);
    }


    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_COMMON_VECTOR_OPERATIONS(SmallVector);
};

} // namespace za
