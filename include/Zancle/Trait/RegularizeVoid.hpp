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
/// after `regularizeVoid`, the result is never `void`, even if `f`
/// returns `void`. Any other result is returned exactly as `f` returns
/// it (references are forwarded, not copied).
///
////////////////////////////////////////////////////////////
template <typename F>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr decltype(auto) regularizeVoid(F&& f)
{
    // Must check the same (forwarded) invocation that is performed below,
    // as ref-qualified call operators may return different types
    if constexpr (ZA_IS_SAME(decltype(static_cast<F&&>(f)()), void))
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
