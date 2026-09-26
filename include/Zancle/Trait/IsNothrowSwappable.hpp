#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"

#include "Zancle/Base/DeclVal.hpp"
#include "Zancle/Base/Swap.hpp" // IWYU pragma: keep


namespace za::priv::swap_adl
{
////////////////////////////////////////////////////////////
// Mirrors the dispatch of `SwapFn::operator()`. Lives in this namespace so that
// unqualified `swap` also sees the deleted generic `swap`, which makes ambiguous
// ADL candidates (e.g. `std::swap`) fall through to the move-based path.
template <typename T>
[[nodiscard]] consteval bool isNothrowSwappableImpl()
{
    if constexpr (requires(T& a, T& b) { a.swap(b); })
        return noexcept(declVal<T&>().swap(declVal<T&>()));
    else if constexpr (requires(T& a, T& b) { swap(a, b); })
        return noexcept(swap(declVal<T&>(), declVal<T&>()));
    else
        return ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(T) && ZA_IS_NOTHROW_MOVE_ASSIGNABLE(T);
}


////////////////////////////////////////////////////////////
template <typename T>
inline constexpr bool isNothrowSwappable = isNothrowSwappableImpl<T>();


////////////////////////////////////////////////////////////
template <typename T, SwapFn::SizeT N>
inline constexpr bool isNothrowSwappable<T[N]> = isNothrowSwappable<T>;

} // namespace za::priv::swap_adl


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Whether `genericSwap` on two `T` lvalues cannot throw
///
/// Follows the same dispatch as `genericSwap` (member `swap`, then
/// unambiguous ADL `swap`, then move-construct + move-assign), but
/// reports the `noexcept`-ness of the selected operation. This is
/// needed because `genericSwap` itself is unconditionally `noexcept`.
///
////////////////////////////////////////////////////////////
template <typename T>
inline constexpr bool isNothrowSwappable = priv::swap_adl::isNothrowSwappable<T>;

} // namespace za


////////////////////////////////////////////////////////////
#define ZA_IS_NOTHROW_SWAPPABLE(...) ::za::isNothrowSwappable<__VA_ARGS__>
