#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
/// \file
///
/// \brief Internal helpers for `za::math::*` math wrappers
///
/// Provides `ZA_PRIV_HAS_MATH_BUILTIN(name)`, which checks
/// whether all three of `__builtin_<name>`, `__builtin_<name>f`, and
/// `__builtin_<name>l` are available. Used by per-function math
/// headers to decide whether to define `ZA_MATH_<NAME>(F|L)`
/// macros that point at the compiler builtins or at `::std::<name>`
/// from `<cmath>`.
///
/// Also provides `ZA_PRIV_DEFINE_MATH_WRAPPER_1ARG(name, NAME)`
/// and its 2-arg counterpart, which generate the type-dispatching
/// `za::<name>` function template that calls the appropriate
/// `ZA_MATH_<NAME>(F|L)` macro for `float`, `double`, or
/// `long double`. Caller must define those macros before invoking.
///
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Trait/IsSame.hpp"


////////////////////////////////////////////////////////////
// Checks the `__builtin_`-prefixed names (the ones actually used), not the plain library names:
// the latter are not builtins under `-fno-builtin`/`-ffreestanding`, while the former still are.
#define ZA_PRIV_HAS_MATH_BUILTIN(name) \
    (__has_builtin(__builtin_##name) && __has_builtin(__builtin_##name##f) && __has_builtin(__builtin_##name##l))


////////////////////////////////////////////////////////////
#define ZA_PRIV_DEFINE_MATH_WRAPPER_1ARG(name, NAME)                                                             \
    namespace za                                                                                                 \
    {                                                                                                            \
                                                                                                                 \
    template <typename T>                                                                                        \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr auto name(const T arg) noexcept \
    {                                                                                                            \
        if constexpr (ZA_IS_SAME(T, float))                                                                      \
            return ZA_MATH_##NAME##F(arg);                                                                       \
        else if constexpr (ZA_IS_SAME(T, double))                                                                \
            return ZA_MATH_##NAME(arg);                                                                          \
        else if constexpr (ZA_IS_SAME(T, long double))                                                           \
            return ZA_MATH_##NAME##L(arg);                                                                       \
        else                                                                                                     \
            static_assert(false);                                                                                \
    }                                                                                                            \
                                                                                                                 \
    } // namespace za

////////////////////////////////////////////////////////////
#define ZA_PRIV_DEFINE_MATH_WRAPPER_2ARG(name, NAME)                                                                            \
    namespace za                                                                                                                \
    {                                                                                                                           \
                                                                                                                                \
    template <typename T>                                                                                                       \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr auto name(const T arg0, const T arg1) noexcept \
    {                                                                                                                           \
        if constexpr (ZA_IS_SAME(T, float))                                                                                     \
            return ZA_MATH_##NAME##F(arg0, arg1);                                                                               \
        else if constexpr (ZA_IS_SAME(T, double))                                                                               \
            return ZA_MATH_##NAME(arg0, arg1);                                                                                  \
        else if constexpr (ZA_IS_SAME(T, long double))                                                                          \
            return ZA_MATH_##NAME##L(arg0, arg1);                                                                               \
        else                                                                                                                    \
            static_assert(false);                                                                                               \
    }                                                                                                                           \
                                                                                                                                \
    } // namespace za
