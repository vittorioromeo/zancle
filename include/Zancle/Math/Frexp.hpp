#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
#include "Zancle/Math/Priv/Impl.hpp"

#include "Zancle/Trait/IsSame.hpp"


////////////////////////////////////////////////////////////
#if ZA_PRIV_HAS_MATH_BUILTIN(frexp)
    #define ZA_MATH_FREXP(...)  __builtin_frexp(__VA_ARGS__)
    #define ZA_MATH_FREXPF(...) __builtin_frexpf(__VA_ARGS__)
    #define ZA_MATH_FREXPL(...) __builtin_frexpl(__VA_ARGS__)
#else
    #include <cmath> // IWYU pragma: keep

    #define ZA_MATH_FREXP(...)  ::std::frexp(__VA_ARGS__)
    #define ZA_MATH_FREXPF(...) ::std::frexpf(__VA_ARGS__)
    #define ZA_MATH_FREXPL(...) ::std::frexpl(__VA_ARGS__)
#endif


////////////////////////////////////////////////////////////
namespace za
{

// Not `gnu::pure`: the exponent is written through `exp`, and Clang would drop that store
template <typename T>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr auto frexp(const T arg0, int* exp) noexcept
{
    if constexpr (ZA_IS_SAME(T, float))
        return ZA_MATH_FREXPF(arg0, exp);
    else if constexpr (ZA_IS_SAME(T, double))
        return ZA_MATH_FREXP(arg0, exp);
    else if constexpr (ZA_IS_SAME(T, long double))
        return ZA_MATH_FREXPL(arg0, exp);
    else
        static_assert(false);
}

} // namespace za
