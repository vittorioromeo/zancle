#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsSame.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Sentinel returned when a type is not found in a parameter pack
///
////////////////////////////////////////////////////////////
enum : SizeT
{
    badTypePackIndex = static_cast<SizeT>(-1)
};


////////////////////////////////////////////////////////////
/// \brief Find the zero-based index of `T` in `Ts...`
///
/// Returns the position of the first match, or `badTypePackIndex` if
/// `T` does not appear in the pack. Used by `Variant` to map an
/// alternative type to its discriminator.
///
////////////////////////////////////////////////////////////
template <typename T, typename... Ts>
[[nodiscard]] consteval SizeT getTypePackIndex() noexcept
{
    constexpr bool matches[]{ZA_IS_SAME(T, Ts)..., false}; // trailing `false` avoids zero-sized array

    for (SizeT i = 0u; i < sizeof...(Ts); ++i)
        if (matches[i])
            return i;

    return badTypePackIndex;
}


} // namespace za
