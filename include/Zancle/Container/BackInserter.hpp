#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/PtrDiffT.hpp"

#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/RemoveCVRef.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Output iterator that appends to a container via `pushBack`
///
/// Lightweight counterpart of `std::back_insert_iterator` for Zancle
/// containers: every assignment `*it = value` forwards `value` to
/// `container.pushBack(...)`. Works with any type providing a
/// `pushBack` member (e.g. `za::Vector`, `za::Utf8String`), without
/// requiring a `value_type` typedef.
///
////////////////////////////////////////////////////////////
template <typename T>
class BackInserter
{
private:
    ////////////////////////////////////////////////////////////
    T* m_container;


public:
    ////////////////////////////////////////////////////////////
    using container_type  = T;
    using value_type      = void;
    using difference_type = PtrDiffT;


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] explicit BackInserter(T& container) noexcept : m_container(&container)
    {
    }


    ////////////////////////////////////////////////////////////
    /// \brief Append `value` to the container via `pushBack`
    ///
    /// Constrained so that it never hijacks copy/move assignment of
    /// the `BackInserter` itself.
    ///
    ////////////////////////////////////////////////////////////
    template <typename U>
        requires(!za::isSame<za::RemoveCVRefIndirect<U>, BackInserter>)
    [[gnu::always_inline]] BackInserter& operator=(U&& value)
    {
        m_container->pushBack(static_cast<U&&>(value));
        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] BackInserter& operator*() noexcept
    {
        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::pure]] BackInserter& operator++() noexcept
    {
        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] BackInserter operator++(int) noexcept
    {
        return *this;
    }
};

} // namespace za
