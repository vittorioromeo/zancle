#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/FwdStdAlignedNewDelete.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/Memmove.hpp"
#include "Zancle/Base/PlacementNew.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Swap.hpp"

#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"


namespace za::priv::VectorUtils
{
////////////////////////////////////////////////////////////
template <typename T>
[[gnu::always_inline]] inline constexpr void moveRange(T* target, T* const srcBegin, T* const srcEnd)
{
    if constexpr (ZA_IS_TRIVIALLY_COPYABLE(T))
    {
        ZA_MEMCPY(target, srcBegin, sizeof(T) * static_cast<SizeT>(srcEnd - srcBegin));
    }
    else
    {
        for (T* p = srcBegin; p != srcEnd; ++p, ++target)
            ZA_PLACEMENT_NEW(target) T(static_cast<T&&>(*p)); // not exception-safe
    }
}


////////////////////////////////////////////////////////////
template <typename T>
[[gnu::always_inline]] inline constexpr void copyRange(T* target, const T* const srcBegin, const T* const srcEnd)
{
    if constexpr (ZA_IS_TRIVIALLY_COPYABLE(T))
    {
        ZA_MEMCPY(target, srcBegin, sizeof(T) * static_cast<SizeT>(srcEnd - srcBegin));
    }
    else
    {
        for (const T* p = srcBegin; p != srcEnd; ++p, ++target)
            ZA_PLACEMENT_NEW(target) T(*p);
    }
}


////////////////////////////////////////////////////////////
template <typename T>
[[gnu::always_inline]] inline constexpr void destroyRange(T* const srcBegin, T* const srcEnd)
{
    if constexpr (!ZA_IS_TRIVIALLY_DESTRUCTIBLE(T))
    {
        for (T* p = srcBegin; p != srcEnd; ++p)
            p->~T();
    }
}


////////////////////////////////////////////////////////////
template <typename T>
[[gnu::always_inline, gnu::flatten]] inline constexpr void relocateRange(T* target, T* const srcBegin, T* const srcEnd)
{
    if constexpr (ZA_IS_TRIVIALLY_RELOCATABLE(T))
    {
        ZA_MEMCPY(static_cast<void*>(target), srcBegin, sizeof(T) * static_cast<SizeT>(srcEnd - srcBegin));
    }
    else
    {
        moveRange(target, srcBegin, srcEnd);
        destroyRange(srcBegin, srcEnd);
    }
}


////////////////////////////////////////////////////////////
template <typename T>
[[gnu::always_inline]] inline constexpr void defaultConstructRange(T* const begin, T* const end)
{
    for (T* p = begin; p != end; ++p)
        ZA_PLACEMENT_NEW(p) T();
}


////////////////////////////////////////////////////////////
template <typename T>
[[gnu::always_inline]] inline constexpr void copyConstructRange(T* const begin, T* const end, const T& value)
{
    for (T* p = begin; p != end; ++p)
        ZA_PLACEMENT_NEW(p) T(value);
}


////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard, gnu::always_inline]] inline T* allocate(const SizeT capacity)
{
    return capacity == 0u ? nullptr : static_cast<T*>(::operator new(capacity * sizeof(T), std::align_val_t{alignof(T)}));
}


////////////////////////////////////////////////////////////
template <typename T>
[[gnu::always_inline]] inline constexpr void deallocate(T* const p, const SizeT /* capacity */) noexcept
{
    ::operator delete(p, std::align_val_t{alignof(T)});
}


////////////////////////////////////////////////////////////
/// \brief Erase the element at `it`, shifting `[it+1, end)` left by 1
///
/// Takes full responsibility for destruction: the slot at `end - 1`
/// is left uninitialized after the call. The caller must update its
/// size to the returned pointer and must NOT destroy the trailing slot.
///
/// \return Pointer one past the last live element (i.e. `end - 1`)
///
////////////////////////////////////////////////////////////
template <typename T>
[[gnu::always_inline, gnu::flatten]] inline constexpr T* eraseImpl(T* const end, T* const it)
{
    ZA_ASSERT(it < end);

    T* const nextElement = it + 1;

    if constexpr (ZA_IS_TRIVIALLY_RELOCATABLE(T))
    {
        // For non-trivially-destructible types we must release the resources
        // held by `*it` before its bytes get overwritten by the memmove
        // (otherwise we leak). For trivially destructible types (which include
        // all trivially copyable types) the destroy is a no-op.
        if constexpr (!ZA_IS_TRIVIALLY_DESTRUCTIBLE(T))
            it->~T();

        // Bulk shift `[it+1, end)` over `[it, end-1)`.
        if (nextElement < end)
            ZA_MEMMOVE(static_cast<void*>(it), nextElement, static_cast<SizeT>(end - nextElement) * sizeof(T));

        // The slot at `end - 1` now holds duplicate / dead bytes -- caller must not dtor it.
        return end - 1;
    }
    else
    {
        // Generic path: shift via move-assignment, then destroy whatever
        // remains in the trailing slot (either the original element if
        // erasing the last one, or a moved-from element after the shift).
        T* currWrite = it;
        T* currRead  = nextElement;

        while (currRead != end)
            *currWrite++ = static_cast<T&&>(*currRead++);

        // `currWrite == end - 1`; destroy the trailing slot.
        currWrite->~T();
        return currWrite;
    }
}


////////////////////////////////////////////////////////////
/// \brief Erase the half-open range `[first, last)`, shifting the tail left
///
/// Same destruction contract as `eraseImpl`: returns one past the last
/// live element; trailing slots are left uninitialized.
///
////////////////////////////////////////////////////////////
template <typename T>
[[gnu::always_inline, gnu::flatten]] inline constexpr T* eraseRangeImpl(T* const end, T* const first, T* const last)
{
    ZA_ASSERT(first <= last);
    ZA_ASSERT(last <= end);
    ZA_ASSERT(first != last);

    if constexpr (ZA_IS_TRIVIALLY_RELOCATABLE(T))
    {
        // Release resources held by the erased elements before their bytes
        // are overwritten / abandoned. No-op for trivially destructible T.
        destroyRange(first, last);

        // Bulk shift `[last, end)` over `[first, first + (end - last))`.
        if (last < end)
            ZA_MEMMOVE(static_cast<void*>(first), last, static_cast<SizeT>(end - last) * sizeof(T));

        // Slots past `first + (end - last)` are now dead bytes -- caller must not dtor them.
        return first + (end - last);
    }
    else
    {
        // Generic path: shift via move-assignment, then destroy the tail.
        T* currWrite = first;

        T* currRead = last;
        while (currRead != end)
            *currWrite++ = static_cast<T&&>(*currRead++);

        // The tail `[currWrite, end)` covers both still-original elements
        // (when more were erased than shifted) and moved-from elements
        // (after the shift). Both need their destructors called.
        destroyRange(currWrite, end);
        return currWrite;
    }
}


////////////////////////////////////////////////////////////
template <typename T>
[[gnu::always_inline, gnu::flatten]] inline constexpr void makeHole(T* const pos, T* const end)
{
    // Moves elements in `[pos, end)` to `[pos + 1, end + 1)`.
    // Assumes capacity is sufficient.
    ZA_ASSERT(pos <= end);

    if (pos == end)
        return; // Inserting at the end, no move needed.

    if constexpr (ZA_IS_TRIVIALLY_COPYABLE(T) || ZA_IS_TRIVIALLY_RELOCATABLE(T))
    {
        ZA_MEMMOVE(static_cast<void*>(pos + 1),                // Destination
                   pos,                                        // Source
                   static_cast<SizeT>(end - pos) * sizeof(T)); // Number of bytes
    }
    else
    {
        // Move-construct a new element at the end from the old last element.
        ZA_PLACEMENT_NEW(end) T(static_cast<T&&>(*(end - 1)));

        // Move-assign elements backwards to shift them to the right.
        for (T* p = end - 1; p > pos; --p)
            *p = static_cast<T&&>(*(p - 1));

        // The element at `pos` has been moved from, but its memory is still valid.
        // It will be overwritten by the new element. If it's not trivially
        // destructible, we should destroy it first to release its resources.
        if constexpr (!ZA_IS_TRIVIALLY_DESTRUCTIBLE(T))
            pos->~T();
    }
}


////////////////////////////////////////////////////////////
template <typename T>
[[gnu::always_inline]] inline constexpr void swapUnequalRanges(T* lhsData, SizeT& lhsSize, T* rhsData, SizeT& rhsSize)
{
    const SizeT s1 = lhsSize;
    const SizeT s2 = rhsSize;

    const SizeT commonSize = s2 < s1 ? s2 : s1;

    for (SizeT i = 0u; i < commonSize; ++i)
        za::genericSwap(lhsData[i], rhsData[i]); // Swap elements in the common part

    if (s1 > s2) // `lhs` is larger; its tail elements move to `rhs`
        relocateRange(rhsData + commonSize, lhsData + commonSize, lhsData + s1);
    else if (s2 > s1) // `rhs` is larger; its tail elements move to `lhs`
        relocateRange(lhsData + commonSize, rhsData + commonSize, rhsData + s2);

    za::genericSwap(lhsSize, rhsSize);
}


////////////////////////////////////////////////////////////
/// \brief `true` iff `p` lies outside the storage range `[begin, end)`
///
/// Debug-only helper used by `resize` to assert that fill arguments do
/// not alias an element of the container being resized (which would
/// dangle once a reallocation frees the old buffer).
///
////////////////////////////////////////////////////////////
template <typename T, typename U>
[[nodiscard, gnu::always_inline]] inline bool isOutsideStorage(const T* const begin, const T* const end, const U* const p) noexcept
{
    const void* const pv = static_cast<const void*>(p);
    return pv < static_cast<const void*>(begin) || pv >= static_cast<const void*>(end);
}


////////////////////////////////////////////////////////////
#define ZA_PRIV_DEFINE_COMMON_VECTOR_OPERATIONS(vectorType)                                                                    \
                                                                                                                               \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr TItem& operator[](const SizeT i) noexcept             \
    {                                                                                                                          \
        ZA_ASSERT(i < size());                                                                                                 \
        return *(data() + i);                                                                                                  \
    }                                                                                                                          \
                                                                                                                               \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr const TItem& operator[](const SizeT i) const noexcept \
    {                                                                                                                          \
        ZA_ASSERT(i < size());                                                                                                 \
        return *(data() + i);                                                                                                  \
    }                                                                                                                          \
                                                                                                                               \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr TItem* begin() noexcept                               \
    {                                                                                                                          \
        return data();                                                                                                         \
    }                                                                                                                          \
                                                                                                                               \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr const TItem* begin() const noexcept                   \
    {                                                                                                                          \
        return data();                                                                                                         \
    }                                                                                                                          \
                                                                                                                               \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr TItem* end() noexcept                                 \
    {                                                                                                                          \
        return data() + size();                                                                                                \
    }                                                                                                                          \
                                                                                                                               \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr const TItem* end() const noexcept                     \
    {                                                                                                                          \
        return data() + size();                                                                                                \
    }                                                                                                                          \
                                                                                                                               \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr const TItem* cbegin() const noexcept                  \
    {                                                                                                                          \
        return data();                                                                                                         \
    }                                                                                                                          \
                                                                                                                               \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr const TItem* cend() const noexcept                    \
    {                                                                                                                          \
        return data() + size();                                                                                                \
    }                                                                                                                          \
                                                                                                                               \
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool empty() const noexcept                                         \
    {                                                                                                                          \
        return size() == 0u;                                                                                                   \
    }                                                                                                                          \
                                                                                                                               \
    [[nodiscard]] constexpr bool operator==(const vectorType& rhs) const                                                       \
    {                                                                                                                          \
        if (this == &rhs)                                                                                                      \
            return true;                                                                                                       \
                                                                                                                               \
        const SizeT lhsSize = size();                                                                                          \
                                                                                                                               \
        if (lhsSize != rhs.size())                                                                                             \
            return false;                                                                                                      \
                                                                                                                               \
        for (SizeT i = 0u; i < lhsSize; ++i)                                                                                   \
            if (operator[](i) != rhs.operator[](i))                                                                            \
                return false;                                                                                                  \
                                                                                                                               \
        return true;                                                                                                           \
    }                                                                                                                          \
                                                                                                                               \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                                                                           \
    [[gnu::always_inline]] constexpr friend void swap(vectorType& lhs, vectorType& rhs) noexcept                               \
    {                                                                                                                          \
        lhs.swap(rhs);                                                                                                         \
    }                                                                                                                          \
                                                                                                                               \
    template <typename... Ts>                                                                                                  \
    [[gnu::always_inline]] constexpr TItem& reEmplaceByIterator(TItem* const it, Ts&&... xs)                                   \
    {                                                                                                                          \
        ZA_ASSERT(it >= begin() && it < end());                                                                                \
                                                                                                                               \
        if constexpr (!ZA_IS_TRIVIALLY_DESTRUCTIBLE(TItem))                                                                    \
            it->~TItem();                                                                                                      \
                                                                                                                               \
        return *(ZA_PLACEMENT_NEW(it) TItem(static_cast<Ts&&>(xs)...));                                                        \
    }                                                                                                                          \
                                                                                                                               \
    template <typename... Ts>                                                                                                  \
    [[gnu::always_inline]] constexpr TItem& reEmplaceByIndex(const SizeT index, Ts&&... xs)                                    \
    {                                                                                                                          \
        return reEmplaceByIterator(data() + index, static_cast<Ts&&>(xs)...);                                                  \
    }                                                                                                                          \
                                                                                                                               \
    template <typename... TItems>                                                                                              \
    [[gnu::always_inline]] constexpr void pushBackMultiple(TItems&&... items)                                                  \
    {                                                                                                                          \
        reserve(size() + sizeof...(items));                                                                                    \
        unsafePushBackMultiple(static_cast<TItems&&>(items)...);                                                               \
    }                                                                                                                          \
                                                                                                                               \
    [[gnu::always_inline]] constexpr void emplaceRange(const TItem* const ptr, const SizeT count)                              \
    {                                                                                                                          \
        reserve(size() + count);                                                                                               \
        unsafeEmplaceBackRange(ptr, count);                                                                                    \
    }                                                                                                                          \
                                                                                                                               \
    [[gnu::always_inline, gnu::flatten]] constexpr void unsafeEmplaceOther(const vectorType& rhs) noexcept                     \
    {                                                                                                                          \
        unsafeEmplaceBackRange(rhs.data(), rhs.size());                                                                        \
    }                                                                                                                          \
                                                                                                                               \
    [[gnu::always_inline]] constexpr void assignRange(const TItem* const b, const TItem* const e)                              \
    {                                                                                                                          \
        ZA_ASSERT(b <= e);                                                                                                     \
        ZA_ASSERT(b == e || (b != nullptr && e != nullptr)); /* only a non-empty range must be non-null */                     \
                                                                                                                               \
        const auto count = static_cast<SizeT>(e - b);                                                                          \
                                                                                                                               \
        clear();                                                                                                               \
        reserve(count);                                                                                                        \
                                                                                                                               \
        if (count != 0u) /* avoid `memcpy(null, null, 0)` (UB) on the empty-range path */                                      \
            priv::VectorUtils::copyRange(data(), b, e);                                                                        \
                                                                                                                               \
        unsafeSetSize(count);                                                                                                  \
    }                                                                                                                          \
                                                                                                                               \
    [[gnu::always_inline]] constexpr void eraseAt(const SizeT index)                                                           \
    {                                                                                                                          \
        ZA_ASSERT(index < size());                                                                                             \
        erase(data() + index);                                                                                                 \
    }                                                                                                                          \
                                                                                                                               \
    static_assert(true)


} // namespace za::priv::VectorUtils


////////////////////////////////////////////////////////////
/// \file
///
/// \brief Shared low-level helpers for `Vector`, `SmallVector`, `InPlaceVector`
///
/// All non-trivial element-management primitives (allocate / deallocate,
/// construct / destroy / relocate ranges, erase, swap unequal ranges,
/// `ZA_PRIV_DEFINE_COMMON_VECTOR_OPERATIONS` macro) live here so
/// that the three vector flavors can share their implementation
/// without inheritance or virtual dispatch. The helpers branch on
/// trivial relocatability and trivial destructibility to fall back to
/// `memcpy`/`memmove` whenever the element type permits it.
///
////////////////////////////////////////////////////////////
