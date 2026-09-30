#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Config.hpp"

#include "Zancle/String/FromCharsResult.hpp" // IWYU pragma: export

#include "Zancle/Trait/IsFloatingPoint.hpp"
#include "Zancle/Trait/IsIntegral.hpp"
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/IsUnsigned.hpp"
#include "Zancle/Trait/MakeUnsigned.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
[[nodiscard]] inline constexpr bool isDigit(const char c)
{
    return c >= '0' && c <= '9';
}


////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] inline consteval T maxIntegral() noexcept
{
    if constexpr (ZA_IS_UNSIGNED(T))
    {
        // For unsigned types, max is all bits set to 1.
        return static_cast<T>(~T(0));
    }
    else
    {
        // For signed types, max is the unsigned max shifted right by one bit.
        // This is a portable way to get the max signed value (e.g., 0111...111).
        using UnsignedT = ZA_MAKE_UNSIGNED(T);
        return static_cast<T>((static_cast<UnsignedT>(~UnsignedT(0))) >> 1);
    }
}


////////////////////////////////////////////////////////////
/// \brief Floating-point parsing (see `za::fromChars`), defined in `FromChars.cpp`
///
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_SYSTEM_API FromCharsResult fromCharsFloat(const char* first, const char* last, float& value);
[[nodiscard]] ZA_SYSTEM_API FromCharsResult fromCharsDouble(const char* first, const char* last, double& value);

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Parse an integer from `[first, last)` into `value`
///
/// Mirrors `std::from_chars`: only base-10 digits and an optional leading
/// sign are accepted (unlike `std::from_chars`, a leading `'+'` is also
/// accepted). Overflow is detected before it happens.
///
/// On `FromCharsError::ResultOutOfRange`, the returned pointer is past all
/// the digits (not just the ones that fit), and `value` is left untouched.
/// On `FromCharsError::InvalidArgument` (no digits), the returned pointer
/// is `first`: nothing is consumed.
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] FromCharsResult fromChars(const char* const first, const char* const last, T& value)
    requires(isIntegral<T> && !isSame<T, bool>)
{
    const char* p = first;

    if (p == last)
        return {first, FromCharsError::InvalidArgument};

    bool isNegative = false;
    if (*p == '-')
    {
        isNegative = true;
        ++p;
    }
    else if (*p == '+')
    {
        ++p;
    }

    if constexpr (ZA_IS_UNSIGNED(T))
    {
        if (isNegative)
            return {first, FromCharsError::InvalidArgument};
    }

    if (p == last || !priv::isDigit(*p))
        return {first, FromCharsError::InvalidArgument};

    using UnsignedT = MakeUnsigned<T>;

    UnsignedT result = 0;

    constexpr auto maxPositive = static_cast<UnsignedT>(priv::maxIntegral<T>());
    const auto     limit       = [&]
    {
        if constexpr (ZA_IS_UNSIGNED(T))
        {
            return maxPositive;
        }
        else
        {
            return isNegative ? static_cast<UnsignedT>(maxPositive + 1u) : maxPositive;
        }
    }();

    // Hoisted out of the loop: `limit` depends on the sign, so these would otherwise be two
    // runtime divisions per digit (at `-O0` in particular)
    const auto limitDiv10 = static_cast<UnsignedT>(limit / 10u);
    const auto limitMod10 = static_cast<UnsignedT>(limit % 10u);

    while (p != last && priv::isDigit(*p))
    {
        const auto digit = static_cast<UnsignedT>(*p - '0');

        // Check for overflow before multiplication
        if (result > limitDiv10 || (result == limitDiv10 && digit > limitMod10))
        {
            // Consume the rest of the number, so that parsing can resume after it
            while (p != last && priv::isDigit(*p))
                ++p;

            return {p, FromCharsError::ResultOutOfRange};
        }

        result = static_cast<UnsignedT>(result * 10u + digit);
        ++p;
    }

    if constexpr (!ZA_IS_UNSIGNED(T))
    {
        // Negate without overflow: `result` may hold `|T_MIN|`, which doesn't fit in `T`,
        // so the naive `-static_cast<T>(result)` would be UB. `-(result - 1) - 1` keeps
        // every intermediate within `T`'s range (`|T_MIN| - 1 == T_MAX`).
        value = isNegative ? (result == 0u ? T{0} : static_cast<T>(-static_cast<T>(result - 1u) - T{1}))
                           : static_cast<T>(result);
    }
    else
    {
        value = static_cast<T>(result);
    }

    return {p, FromCharsError::None}; // Success
}


////////////////////////////////////////////////////////////
/// \brief Parse a floating-point number from `[first, last)` into `value`
///
/// Mirrors `std::from_chars` (`chars_format::general`): the result is the
/// correctly rounded (nearest, ties to even) value of the input, whatever
/// its number of digits. Accepts:
///
/// - An optional sign (unlike `std::from_chars`, a leading `'+'` is also accepted).
/// - Digits with an optional decimal point (`"12"`, `"1.5"`, `"5."`, `".5"`).
/// - An optional exponent (`"1e5"`, `"2.5E-3"`), consumed only if digits follow
///   (`"3em"` parses as `3`, stopping at `'e'`).
/// - `"inf"`, `"infinity"`, `"nan"`, and `"nan(chars)"`, case-insensitively.
///
/// Values beyond the finite range of `T`, and nonzero values that round to
/// zero, yield `FromCharsError::ResultOutOfRange`, with the returned pointer
/// past the whole number. Without digits, `FromCharsError::InvalidArgument`
/// is returned with `first`. On errors, `value` is left untouched.
///
/// `long double` values are parsed as `double`.
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] FromCharsResult fromChars(const char* const first, const char* const last, T& value)
    requires isFloatingPoint<T>
{
    if constexpr (ZA_IS_SAME(T, float))
    {
        return priv::fromCharsFloat(first, last, value);
    }
    else if constexpr (ZA_IS_SAME(T, double))
    {
        return priv::fromCharsDouble(first, last, value);
    }
    else
    {
        double     parsed = 0.0;
        const auto result = priv::fromCharsDouble(first, last, parsed);

        if (result.ec == FromCharsError::None)
            value = static_cast<T>(parsed);

        return result;
    }
}

} // namespace za
