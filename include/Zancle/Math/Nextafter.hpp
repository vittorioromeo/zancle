#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
#include "Zancle/Math/Priv/Impl.hpp"


////////////////////////////////////////////////////////////
#if ZA_PRIV_HAS_MATH_BUILTIN(nextafter)
    #define ZA_MATH_NEXTAFTER(...)  __builtin_nextafter(__VA_ARGS__)
    #define ZA_MATH_NEXTAFTERF(...) __builtin_nextafterf(__VA_ARGS__)
    #define ZA_MATH_NEXTAFTERL(...) __builtin_nextafterl(__VA_ARGS__)
#else
    #include <cmath> // IWYU pragma: keep

    // `::std::nextafter` has `float` overloads: convert like `__builtin_nextafter`, which only takes `double`
    #define ZA_MATH_NEXTAFTER(x, y) ::std::nextafter(static_cast<double>(x), static_cast<double>(y))
    #define ZA_MATH_NEXTAFTERF(...) ::std::nextafterf(__VA_ARGS__)
    #define ZA_MATH_NEXTAFTERL(...) ::std::nextafterl(__VA_ARGS__)
#endif


////////////////////////////////////////////////////////////
ZA_PRIV_DEFINE_MATH_WRAPPER_2ARG(nextafter, NEXTAFTER)
