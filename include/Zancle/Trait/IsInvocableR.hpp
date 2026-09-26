#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Trait/IsConvertible.hpp"
#include "Zancle/Trait/IsSame.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief `true` if `F` can be called with `Args...` and the result converts to `R`
///
/// Any result is accepted if `R` is `void`. Unlike `std::is_invocable_r_v`,
/// only plain call syntax is supported (no pointers to members), which
/// keeps it cheap to compile.
///
////////////////////////////////////////////////////////////
template <typename F, typename R, typename... Args>
inline constexpr bool isInvocableR = requires(F&& f, Args&&... args) {
    static_cast<F&&>(f)(static_cast<Args&&>(args)...);
    requires isSame<R, void> || isConvertible<decltype(static_cast<F&&>(f)(static_cast<Args&&>(args)...)), R>;
};

} // namespace za
