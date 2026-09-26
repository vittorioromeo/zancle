#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
#include "Zancle/Math/Priv/Impl.hpp"


////////////////////////////////////////////////////////////
#if ZA_PRIV_HAS_MATH_BUILTIN(cosh)
    #define ZA_MATH_COSH(...)  __builtin_cosh(__VA_ARGS__)
    #define ZA_MATH_COSHF(...) __builtin_coshf(__VA_ARGS__)
    #define ZA_MATH_COSHL(...) __builtin_coshl(__VA_ARGS__)
#else
    #include <cmath> // IWYU pragma: keep

    // `::std::cosh` has `float` overloads: convert like `__builtin_cosh`, which only takes `double`
    #define ZA_MATH_COSH(x)    ::std::cosh(static_cast<double>(x))
    #define ZA_MATH_COSHF(...) ::std::coshf(__VA_ARGS__)
    #define ZA_MATH_COSHL(...) ::std::coshl(__VA_ARGS__)
#endif


////////////////////////////////////////////////////////////
ZA_PRIV_DEFINE_MATH_WRAPPER_1ARG(cosh, COSH)
