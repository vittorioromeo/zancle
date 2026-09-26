#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
#include "Zancle/Math/Priv/Impl.hpp"

#include "Zancle/Trait/IsSame.hpp"


////////////////////////////////////////////////////////////
#if ZA_PRIV_HAS_MATH_BUILTIN(ldexp)
    #define ZA_MATH_LDEXP(...)  __builtin_ldexp(__VA_ARGS__)
    #define ZA_MATH_LDEXPF(...) __builtin_ldexpf(__VA_ARGS__)
    #define ZA_MATH_LDEXPL(...) __builtin_ldexpl(__VA_ARGS__)
#else
    #include <cmath> // IWYU pragma: keep

    #define ZA_MATH_LDEXP(...)  ::std::ldexp(__VA_ARGS__)
    #define ZA_MATH_LDEXPF(...) ::std::ldexpf(__VA_ARGS__)
    #define ZA_MATH_LDEXPL(...) ::std::ldexpl(__VA_ARGS__)
#endif


////////////////////////////////////////////////////////////
namespace za
{

template <typename T>
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr auto ldexp(const T arg0, int exp) noexcept
{
    if constexpr (ZA_IS_SAME(T, float))
        return ZA_MATH_LDEXPF(arg0, exp);
    else if constexpr (ZA_IS_SAME(T, double))
        return ZA_MATH_LDEXP(arg0, exp);
    else if constexpr (ZA_IS_SAME(T, long double))
        return ZA_MATH_LDEXPL(arg0, exp);
    else
        static_assert(false);
}

} // namespace za
