#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Trait/IsSame.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Empty placeholder type returned in place of `void`
///
////////////////////////////////////////////////////////////
struct RegularizeVoidDummy
{
};


////////////////////////////////////////////////////////////
/// \brief Invoke `f()` and replace a `void` return with `RegularizeVoidDummy`
///
/// Useful in generic code that wants to treat any callable uniformly:
/// after `regularizeVoid`, the result is always a regular value type
/// even if `f` returns `void`.
///
////////////////////////////////////////////////////////////
template <typename F>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr auto regularizeVoid(F&& f)
{
    if constexpr (ZA_IS_SAME(decltype(f()), void))
    {
        static_cast<F&&>(f)();
        return RegularizeVoidDummy{};
    }
    else
    {
        return static_cast<F&&>(f)();
    }
}

} // namespace za
