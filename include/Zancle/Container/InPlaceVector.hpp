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
#include "Zancle/Base/PtrDiffT.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/EnableTrivialRelocation.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Bounded vector backed entirely by inline storage (no heap allocation)
///
/// `InPlaceVector<T, N>` provides the usual `Vector` interface but
/// stores all elements in a fixed-size aligned buffer of capacity `N`.
/// It never allocates and `capacity()` always returns `N`. Attempting
/// to grow past `N` is a programming error and is caught by debug
/// assertions on the relevant `unsafe*` and `emplace*` operations.
///
/// Useful when the maximum element count is known statically and you
/// want to avoid both heap traffic and the size overhead of
/// `SmallVector`'s heap escape hatch.
///
////////////////////////////////////////////////////////////
template <typename TItem, SizeT N>
class [[nodiscard]] InPlaceVector // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    static_assert(N > 0);

private:
    ////////////////////////////////////////////////////////////
    alignas(TItem) unsigned char m_storage[sizeof(TItem) * N];
    SizeT m_size{0u};


public:
    ////////////////////////////////////////////////////////////
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
    [[nodiscard]] constexpr InPlaceVector() = default;


    ////////////////////////////////////////////////////////////
    constexpr ~InPlaceVector()
    {
        priv::VectorUtils::destroyRange(data(), data() + m_size);
    }


    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    [[nodiscard]] constexpr explicit InPlaceVector(const SizeT initialSize) : m_size{initialSize}
    {
        ZA_ASSERT(initialSize <= N);
        priv::VectorUtils::defaultConstructRange(data(), data() + initialSize);
    }


    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    [[nodiscard]] constexpr explicit InPlaceVector(const SizeT initialSize, const TItem& value) : m_size{initialSize}
    {
        ZA_ASSERT(initialSize <= N);
        priv::VectorUtils::copyConstructRange(data(), data() + initialSize, value);
    }


    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    [[nodiscard]] constexpr explicit InPlaceVector(const TItem* const srcBegin, const TItem* const srcEnd)
    {
        ZA_ASSERT(srcBegin <= srcEnd);
        const auto srcCount = static_cast<SizeT>(srcEnd - srcBegin);
        ZA_ASSERT(srcCount <= N);

        priv::VectorUtils::copyRange(data(), srcBegin, srcEnd);
        m_size = srcCount;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr /* implicit */ InPlaceVector(const std::initializer_list<TItem> iList) :
        InPlaceVector(iList.begin(), iList.end())
    {
    }


    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    [[nodiscard, gnu::always_inline]] constexpr InPlaceVector(const InPlaceVector& rhs) : m_size{rhs.m_size}
    {
        priv::VectorUtils::copyRange(data(), rhs.data(), rhs.data() + m_size);
    }


    ////////////////////////////////////////////////////////////
    constexpr InPlaceVector& operator=(const InPlaceVector& rhs)
    {
        if (this == &rhs)
            return *this;

        clear();
        priv::VectorUtils::copyRange(data(), rhs.data(), rhs.data() + rhs.m_size);

        m_size = rhs.m_size;
        return *this;
    }


    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    [[nodiscard, gnu::always_inline]] constexpr InPlaceVector(InPlaceVector&& rhs) noexcept : m_size{rhs.m_size}
    {
        priv::VectorUtils::relocateRange(data(), rhs.data(), rhs.data() + rhs.m_size);
        rhs.m_size = 0u;
    }


    ////////////////////////////////////////////////////////////
    constexpr InPlaceVector& operator=(InPlaceVector&& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        clear();

        priv::VectorUtils::relocateRange(data(), rhs.data(), rhs.data() + rhs.m_size);

        m_size     = rhs.m_size;
        rhs.m_size = 0u;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void resize(const SizeT newSize, auto&&... args)
    {
        ZA_ASSERT(newSize <= N);

        const auto   oldSize        = m_size;
        TItem* const currentDataPtr = data();

        if (newSize > oldSize)
        {
            for (auto* p = currentDataPtr + oldSize; p != currentDataPtr + newSize; ++p)
                ZA_PLACEMENT_NEW(p) TItem(args...); // intentionally not forwarding
        }
        else if (newSize < oldSize) // Shrinking
        {
            priv::VectorUtils::destroyRange(currentDataPtr + newSize, currentDataPtr + oldSize);
        }

        m_size = newSize;
    }


    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    [[gnu::always_inline]] constexpr TItem* emplace(TItem* const pos, Ts&&... xs)
    {
        ZA_ASSERT(m_size < N);
        ZA_ASSERT(pos >= begin() && pos <= end());

        const auto index = static_cast<SizeT>(pos - data());

        if (pos == end()) // Append at end: no shift, no aliasing risk.
        {
            ZA_PLACEMENT_NEW(pos) TItem(static_cast<Ts&&>(xs)...);
            ++m_size;
            return data() + index;
        }

        // Construct a copy first to handle self-aliasing (`makeHole` shifts elements in-place,
        // which invalidates any reference into the shifted region).
        TItem copy(static_cast<Ts&&>(xs)...);
        priv::VectorUtils::makeHole(pos, end());
        ZA_PLACEMENT_NEW(pos) TItem(static_cast<TItem&&>(copy));

        ++m_size;
        return data() + index;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr TItem* insert(TItem* const pos, const TItem& value)
    {
        return emplace(pos, value);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr TItem* insert(TItem* const pos, TItem&& value)
    {
        return emplace(pos, static_cast<TItem&&>(value));
    }


    ////////////////////////////////////////////////////////////
    template <typename T = TItem>
    [[gnu::always_inline, gnu::flatten]] constexpr TItem& pushBack(T&& x)
    {
        ZA_ASSERT(m_size < N);
        return unsafeEmplaceBack(static_cast<T&&>(x));
    }


    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    [[gnu::always_inline, gnu::flatten]] constexpr TItem& emplaceBack(Ts&&... xs)
    {
        ZA_ASSERT(m_size < N);
        return unsafeEmplaceBack(static_cast<Ts&&>(xs)...);
    }


    ////////////////////////////////////////////////////////////
    constexpr void shrinkToFit() noexcept
    {
        // no-op, just for compatibility
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] constexpr void reserve([[maybe_unused]] const SizeT targetCapacity)
    {
        ZA_ASSERT(targetCapacity <= N);
        // no-op, just for compatibility
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] constexpr void reserveMore([[maybe_unused]] const SizeT n)
    {
        ZA_ASSERT(size() + n <= N);
        // no-op, just for compatibility
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] constexpr void unsafeEmplaceBackRange(const TItem* const ptr, const SizeT count) noexcept
    {
        ZA_ASSERT(m_size + count <= N);
        ZA_ASSERT(ptr != nullptr);

        priv::VectorUtils::copyRange(data() + m_size, ptr, ptr + count);
        m_size += count;
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] constexpr void clear() noexcept
    {
        priv::VectorUtils::destroyRange(data(), data() + m_size);
        m_size = 0u;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr SizeT size() const noexcept
    {
        return m_size;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] constexpr SizeT capacity() const noexcept
    {
        return N;
    }


    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    [[gnu::always_inline]] constexpr TItem& unsafeEmplaceBack(Ts&&... xs)
    {
        ZA_ASSERT(m_size < N);
        return *(ZA_PLACEMENT_NEW(data() + m_size++) TItem(static_cast<Ts&&>(xs)...));
    }


    ////////////////////////////////////////////////////////////
    constexpr TItem* erase(TItem* const it)
    {
        ZA_ASSERT(it >= begin() && it < end());

        TItem* const newEnd = priv::VectorUtils::eraseImpl(end(), it);
        m_size              = static_cast<SizeT>(newEnd - data());
        return it;
    }


    ////////////////////////////////////////////////////////////
    constexpr TItem* erase(TItem* const first, TItem* const last)
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
    [[gnu::always_inline]] constexpr void unsafePushBackMultiple(TItems&&... items)
    {
        ZA_ASSERT(m_size + sizeof...(items) <= N);
        (..., ZA_PLACEMENT_NEW(data() + m_size++) TItem(static_cast<TItems&&>(items)));
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr TItem* data() noexcept
    {
        return reinterpret_cast<TItem*>(m_storage);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr const TItem* data() const noexcept
    {
        return reinterpret_cast<const TItem*>(m_storage);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] constexpr void unsafeSetSize(SizeT newSize) noexcept
    {
        ZA_ASSERT(newSize <= N);
        m_size = newSize;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void popBack() noexcept
    {
        ZA_ASSERT(!empty());
        --m_size;

        if constexpr (!ZA_IS_TRIVIALLY_DESTRUCTIBLE(TItem))
            (data() + m_size)->~TItem();
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void swap(InPlaceVector& rhs) noexcept
    {
        if (this == &rhs)
            return;

        priv::VectorUtils::swapUnequalRanges(data(), m_size, rhs.data(), rhs.m_size);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr TItem& front() noexcept
    {
        ZA_ASSERT(!empty());
        return *data();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr const TItem& front() const noexcept
    {
        ZA_ASSERT(!empty());
        return *data();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr TItem& back() noexcept
    {
        ZA_ASSERT(!empty());
        return this->operator[](size() - 1u);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr const TItem& back() const noexcept
    {
        ZA_ASSERT(!empty());
        return this->operator[](size() - 1u);
    }


    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_COMMON_VECTOR_OPERATIONS(InPlaceVector);
};

} // namespace za
