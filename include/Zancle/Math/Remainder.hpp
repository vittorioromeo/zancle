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
/// gameplay use cases that need it. `b` must be strictly positive.
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
/// positive. Useful for wrapping angles or texture coordinates.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] constexpr float positiveRemainder(const float a, const float b) noexcept
{
    ZA_ASSERT_AND_ASSUME(b > 0.f);

    const auto val     = a - static_cast<float>(static_cast<int>(a / b)) * b;
    const auto wrapped = val >= 0.f ? val : val + b;

    // `val + b` rounds to exactly `b` when `val` is a tiny negative number (e.g. `-1e-8f`)
    return wrapped < b ? wrapped : 0.f;
}

} // namespace za
