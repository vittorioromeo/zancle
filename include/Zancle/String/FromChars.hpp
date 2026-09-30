#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/String/FromCharsResult.hpp" // IWYU pragma: export

#include "Zancle/Base/IsInf.hpp"

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

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Parse an integer from `[first, last)` into `value`
///
/// Stricter than `strtol`: only base-10 digits and an optional leading
/// sign are accepted. Detects overflow before it happens and signals
/// it via `FromCharsError::ResultOutOfRange`.
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] FromCharsResult fromChars(const char* first, const char* const last, T& value)
    requires(isIntegral<T> && !isSame<T, bool>)
{
    if (first == last)
        return {first, FromCharsError::InvalidArgument};

    bool isNegative = false;
    if (*first == '-')
    {
        isNegative = true;
        ++first;
    }
    else if (*first == '+')
    {
        ++first;
    }

    if constexpr (ZA_IS_UNSIGNED(T))
    {
        if (isNegative)
            return {first, FromCharsError::InvalidArgument};
    }

    if (first == last || !priv::isDigit(*first))
    {
        return {first, FromCharsError::InvalidArgument};
    }

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

    while (first != last && priv::isDigit(*first))
    {
        const auto digit = static_cast<UnsignedT>(*first - '0');

        // Check for overflow before multiplication
        if (result > limitDiv10 || (result == limitDiv10 && digit > limitMod10))
            return {first, FromCharsError::ResultOutOfRange};

        result = static_cast<UnsignedT>(result * 10u + digit);
        ++first;
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

    return {first, FromCharsError::None}; // Success
}


////////////////////////////////////////////////////////////
/// \brief Parse a floating-point number from `[first, last)` into `value`
///
/// Accepts an optional sign, an integer part, and an optional fractional
/// part. Exponents (`e`/`E`) are not currently supported. Uses
/// `long double` internally to retain precision before narrowing the
/// result back into `T`.
///
/// Values beyond the finite range of `T`, and nonzero values that would
/// round to zero, are rejected with `FromCharsError::ResultOutOfRange`
/// (like `std::from_chars`): `value` is left untouched.
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] FromCharsResult fromChars(const char* first, const char* const last, T& value)
    requires isFloatingPoint<T>
{
    if (first == last)
        return {first, FromCharsError::InvalidArgument};

    const char* initialFirst = first;

    bool isNegative = false;

    if (*first == '-')
    {
        isNegative = true;
        ++first;
    }
    else if (*first == '+')
    {
        ++first;
    }

    // Use long double for intermediate calculations to maximize precision.
    long double result          = 0.0l;
    bool        anyDigitsParsed = false;
    bool        anyNonZeroDigit = false;

    // Parse whole number part
    while (first != last && priv::isDigit(*first))
    {
        anyNonZeroDigit |= *first != '0';
        result = result * 10.0l + (*first - '0');
        ++first;
        anyDigitsParsed = true;
    }

    // Parse fractional part
    if (first != last && *first == '.')
    {
        ++first;
        long double power = 0.1l;
        while (first != last && priv::isDigit(*first))
        {
            anyNonZeroDigit |= *first != '0';
            result += (*first - '0') * power;
            power /= 10.0l;
            ++first;
            anyDigitsParsed = true;
        }
    }

    // If no digits were parsed at all, it's an error.
    if (!anyDigitsParsed)
        return {initialFirst, FromCharsError::InvalidArgument};

    // Out of range: too large for `T` (converting it would be UB), or a nonzero value that underflows to zero
    constexpr long double maxFinite = ZA_IS_SAME(T, float)    ? static_cast<long double>(__FLT_MAX__)
                                      : ZA_IS_SAME(T, double) ? static_cast<long double>(__DBL_MAX__)
                                                              : __LDBL_MAX__;

    if (ZA_ISINF(result) || result > maxFinite || (anyNonZeroDigit && static_cast<T>(result) == T{0}))
        return {first, FromCharsError::ResultOutOfRange};

    if (isNegative)
        result = -result;

    value = static_cast<T>(result);

    return {first, FromCharsError::None}; // Success
}

} // namespace za
