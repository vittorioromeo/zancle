#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/RemoveCVRef.hpp"

#include "Zancle/Base/Launder.hpp"
#include "Zancle/Base/MaxAlignT.hpp"
#include "Zancle/Base/PlacementNew.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief In-place storage for the PImpl idiom (no heap allocation)
///
/// Stores a `T` inside an aligned buffer of `BufferSize` bytes,
/// providing pointer-like access through `operator->`/`operator*`.
/// This implements the PImpl idiom without paying for a heap
/// allocation: the user includes a forward declaration of `T` in the
/// header and only the source file needs to see the full definition.
///
/// `BufferSize` must be at least `sizeof(T)`. A static assertion in the
/// constructor verifies that this holds and that `T`'s alignment does
/// not exceed the maximum fundamental alignment.
///
////////////////////////////////////////////////////////////
template <typename T, decltype(sizeof(int)) BufferSize>
class InPlacePImpl
{
private:
    alignas(MaxAlignT) char m_buffer[BufferSize]; //!< Raw aligned storage for the implementation type

public:
    ////////////////////////////////////////////////////////////
    /// \brief Member access on the contained implementation
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] T* operator->() noexcept
    {
        return ZA_LAUNDER_CAST(T*, m_buffer);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Member access on the contained implementation (const overload)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] const T* operator->() const noexcept
    {
        return ZA_LAUNDER_CAST(const T*, m_buffer);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Reference to the contained implementation
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] T& operator*() noexcept
    {
        return *ZA_LAUNDER_CAST(T*, m_buffer);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Reference to the contained implementation (const overload)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] const T& operator*() const noexcept
    {
        return *ZA_LAUNDER_CAST(const T*, m_buffer);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct the implementation in-place from `args...`
    ///
    /// Statically asserts that the buffer is large enough and aligned
    /// suitably for `T`. Disabled for a single `InPlacePImpl` argument, so
    /// that it does not hijack copy construction from a non-const lvalue.
    ///
    ////////////////////////////////////////////////////////////
    template <typename... Args>
        requires(!(sizeof...(Args) == 1 && (... && isSame<RemoveCVRefIndirect<Args>, InPlacePImpl>)))
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    [[nodiscard, gnu::always_inline]] explicit InPlacePImpl(Args&&... args)
    {
        static_assert(sizeof(T) <= BufferSize);
        static_assert(alignof(T) <= alignof(MaxAlignT));

        ZA_PLACEMENT_NEW(m_buffer) T(static_cast<Args&&>(args)...);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Copy-construct by copy-constructing the held implementation
    ///
    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    [[nodiscard, gnu::always_inline]] InPlacePImpl(const InPlacePImpl& rhs)
    {
        ZA_PLACEMENT_NEW(m_buffer) T(*ZA_LAUNDER_CAST(const T*, rhs.m_buffer));
    }


    ////////////////////////////////////////////////////////////
    /// \brief Move-construct by move-constructing the held implementation
    ///
    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    [[nodiscard, gnu::always_inline]] InPlacePImpl(InPlacePImpl&& rhs) noexcept
    {
        ZA_PLACEMENT_NEW(m_buffer) T(static_cast<T&&>(*ZA_LAUNDER_CAST(T*, rhs.m_buffer)));
    }


    ////////////////////////////////////////////////////////////
    /// \brief Copy-assign by delegating to the held implementation's copy-assignment
    ///
    /// Self-assignment must be handled by the inner type if needed.
    ///
    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    [[gnu::always_inline]] InPlacePImpl& operator=(const InPlacePImpl& rhs)
    {
        // Rely on the inner type for self-assignment check.
        *ZA_LAUNDER_CAST(T*, m_buffer) = *ZA_LAUNDER_CAST(const T*, rhs.m_buffer);
        return *this;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Move-assign by delegating to the held implementation's move-assignment
    ///
    /// Self-assignment must be handled by the inner type if needed.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] InPlacePImpl& operator=(InPlacePImpl&& rhs) noexcept
    {
        // Rely on the inner type for self-assignment check.
        *ZA_LAUNDER_CAST(T*, m_buffer) = static_cast<T&&>(*ZA_LAUNDER_CAST(T*, rhs.m_buffer));
        return *this;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Destructor, runs the held implementation's destructor
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] ~InPlacePImpl()
    {
        ZA_LAUNDER_CAST(T*, m_buffer)->~T();
    }
};

} // namespace za
