#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/AssertAndAssume.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Truncated `a % b` for floats (sign follows `a`)
///
/// Lighter and faster than `std::fmod`: uses `int` truncation rather
/// than the IEEE-754 remainder operation, which is sufficient for the
/// gameplay use cases that need it. `b` must be strictly positive, and
/// `|a / b|` must be less than `2^31` (as the quotient is truncated via `int`).
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] constexpr float remainder(const float a, const float b) noexcept
{
    ZA_ASSERT_AND_ASSUME(b > 0.f);

    return a - static_cast<float>(static_cast<int>(a / b)) * b;
}


////////////////////////////////////////////////////////////
/// \brief Like `remainder` but always returns a value in `[0, b)`
///
/// Equivalent to `((a % b) + b) % b` for floats. `b` must be strictly
/// positive, and `|a / b|` must be less than `2^31` (as the quotient is
/// truncated via `int`). Useful for wrapping angles or texture coordinates.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] constexpr float positiveRemainder(const float a, const float b) noexcept
{
    ZA_ASSERT_AND_ASSUME(b > 0.f);

    const auto val = a - static_cast<float>(static_cast<int>(a / b)) * b;

    // Arithmetic rather than a ternary, so that it compiles to branchless code (the sign of `val`
    // is unpredictable, and a mispredicted branch costs far more than the multiplication)
    const auto wrapped = val + b * static_cast<float>(val < 0.f);

    // Rounding can leave `wrapped` just outside `[0, b)`: `val + b` rounds to exactly `b` when `val`
    // is a tiny negative number (e.g. `-1e-8f`), and when `a / b` rounds towards zero across an
    // integer, `val` is slightly below `-b` (e.g. `a = -2224.24756f` and `b = tau` give a negative
    // `wrapped` of `-1.76e-5f`). The correct result is within rounding error of `0` (modulo `b`) in both cases.
    return (wrapped >= 0.f && wrapped < b) ? wrapped : 0.f;
}

} // namespace za
