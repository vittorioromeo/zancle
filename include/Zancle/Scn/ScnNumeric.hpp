#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Scn/ScnCore.hpp"

#include "Zancle/String/FromChars.hpp"

#include "Zancle/Vocabulary/Radix.hpp"

#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsFloatingPoint.hpp"
#include "Zancle/Trait/IsIntegral.hpp"
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/IsUnsigned.hpp"
#include "Zancle/Trait/MakeUnsigned.hpp"


namespace za::priv
{

////////////////////////////////////////////////////////////
// `priv::isDigit` (from `FromChars.hpp`) covers the decimal case;
// the radix variant below extends it to hex digits with a base check.
//
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline constexpr bool scnIsDigitForRadix(const char c, const unsigned base) noexcept
{
    if (c >= '0' && c <= '9')
        return static_cast<unsigned>(c - '0') < base;

    if (c >= 'a' && c <= 'f')
        return static_cast<unsigned>(c - 'a' + 10) < base;

    if (c >= 'A' && c <= 'F')
        return static_cast<unsigned>(c - 'A' + 10) < base;

    return false;
}


////////////////////////////////////////////////////////////
// Pre: `scnIsDigitForRadix(c, base)` returned `true` for the same args.
//
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline constexpr unsigned scnDigitValue(const char c) noexcept
{
    if (c >= '0' && c <= '9')
        return static_cast<unsigned>(c - '0');

    if (c >= 'a' && c <= 'f')
        return static_cast<unsigned>(c - 'a' + 10);

    return static_cast<unsigned>(c - 'A' + 10);
}

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief `scnArg` for any standard integral type (excluding `bool`).
///
/// Skips leading whitespace, then reads an optional `+` / `-` sign
/// (rejected for unsigned destinations) followed by decimal digits.
/// Stops at the first non-digit byte (which is *not* consumed).
///
/// Returns `false` on EOF / no-digits / overflow. No scratch buffer is used;
/// overflow is checked incrementally while the maximal digit run is consumed.
///
////////////////////////////////////////////////////////////
template <typename T, ScnSource S>
    requires(isIntegral<T> && !ZA_IS_SAME(T, bool))
[[nodiscard]] bool scnArg(S& src, T& out)
{
    scnSkipWhitespace(src);

    auto c = src.peek();
    if (!c)
        return false;

    bool isNegative = false;
    if (*c == '+' || *c == '-')
    {
        isNegative = *c == '-';
        src.consume();
        c = src.peek();
    }

    using UnsignedT = MakeUnsigned<T>;

    UnsignedT result   = 0;
    bool      gotAny   = false;
    bool      overflow = false;

    constexpr auto maxPositive = static_cast<UnsignedT>(priv::maxIntegral<T>());

    const auto limit = [&]
    {
        if constexpr (isUnsigned<T>)
            return maxPositive;
        else
            return isNegative ? static_cast<UnsignedT>(maxPositive + 1u) : maxPositive;
    }();

    while (c && priv::isDigit(*c))
    {
        gotAny = true;

        const auto digit = static_cast<UnsignedT>(*c - '0');

        if (!overflow)
        {
            if (result > limit / 10u || (result == limit / 10u && digit > limit % 10u))
                overflow = true;
            else
                result = static_cast<UnsignedT>(result * 10u + digit);
        }

        src.consume();
        c = src.peek();
    }

    if (!gotAny || overflow)
        return false;

    if constexpr (isUnsigned<T>)
    {
        if (isNegative)
            return false;

        out = static_cast<T>(result);
    }
    else
    {
        out = isNegative ? (result == 0u ? T{0} : static_cast<T>(-static_cast<T>(result - 1u) - T{1}))
                         : static_cast<T>(result);
    }

    return true;
}


////////////////////////////////////////////////////////////
/// \brief `scnArg` for floats. Skips whitespace, then optional sign,
/// integer part, optional `.fraction`. Exponent form (`e` / `E`) is
/// not supported -- matches `fromChars` for float.
///
////////////////////////////////////////////////////////////
template <typename T, ScnSource S>
    requires isFloatingPoint<T>
[[nodiscard]] bool scnArg(S& src, T& out)
{
    scnSkipWhitespace(src);

    // `scnFloatScratchSize` = 40, see comment on the constant.
    char tmp[scnFloatScratchSize];

    SizeT n               = 0u;
    bool  scratchOverflow = false;

    const auto appendScratch = [&](const char x)
    {
        if (n < sizeof(tmp))
            tmp[n++] = x;
        else
            scratchOverflow = true;
    };

    auto c = src.peek();

    if (!c)
        return false;

    if (*c == '+' || *c == '-')
    {
        appendScratch(*c);
        src.consume();

        c = src.peek();
    }

    // Integer part
    while (c && priv::isDigit(*c))
    {
        appendScratch(*c);
        src.consume();

        c = src.peek();
    }

    // Fractional part
    if (c && *c == '.')
    {
        appendScratch(*c);
        src.consume();

        c = src.peek();

        while (c && priv::isDigit(*c))
        {
            appendScratch(*c);
            src.consume();
            c = src.peek();
        }
    }

    if (scratchOverflow)
        return false;

    const auto r = fromChars(tmp, tmp + n, out);
    return r.ec == FromCharsError::None && r.ptr != tmp;
}


////////////////////////////////////////////////////////////
/// \brief `scnArg` for `bool`. Skips whitespace, then accepts:
///   - literal `true` or `false` (case-sensitive), or
///   - a single `'0'` / `'1'` digit.
///
/// Anything else returns `false`. The minimal-match strategy means
/// `1abc` parses as `1` and leaves `abc` on the source.
///
////////////////////////////////////////////////////////////
template <ScnSource S>
[[nodiscard]] bool scnArg(S& src, bool& out)
{
    scnSkipWhitespace(src);

    auto c = src.peek();

    if (!c)
        return false;

    if (*c == '0')
    {
        src.consume();

        out = false;
        return true;
    }

    if (*c == '1')
    {
        src.consume();

        out = true;
        return true;
    }

    // Match against `true` or `false` byte-by-byte. If the prefix matches
    // but the source runs out partway through, the parse fails -- and the
    // matched prefix has been consumed (`out` is left unspecified).
    const auto matchLiteral = [&](const char* const lit, const SizeT len, const bool value) -> bool
    {
        for (SizeT i = 0u; i < len; ++i)
        {
            auto pc = src.peek();

            if (!pc || *pc != lit[i])
                return false;

            src.consume();
        }

        out = value;
        return true;
    };

    if (*c == 't')
        return matchLiteral("true", 4u, true);

    if (*c == 'f')
        return matchLiteral("false", 5u, false);

    return false;
}


////////////////////////////////////////////////////////////
/// \brief Parse an unsigned integer in the given `radix`. Skips
/// whitespace, then consumes the maximal run of digits valid for the
/// chosen radix. No sign accepted -- matches `fromCharsRadix`.
///
/// Returns `false` on EOF, no-digits, or overflow. Consumes the full
/// digit run regardless of overflow (matches the decimal `scnArg`
/// policy: source advances to a stable resume point).
///
/// No scratch buffer is used; overflow is checked incrementally, so
/// arbitrarily long inputs (e.g. many leading zeroes) are accepted as
/// long as the final value fits in `T`.
///
////////////////////////////////////////////////////////////
template <typename T, ScnSource S>
    requires(isIntegral<T> && isUnsigned<T>)
[[nodiscard]] bool scnRadix(S& src, T& out, const Radix radix)
{
    scnSkipWhitespace(src);

    const auto  base  = static_cast<unsigned>(radix);
    constexpr T limit = priv::maxIntegral<T>();

    T    result   = 0;
    bool gotAny   = false;
    bool overflow = false;

    while (auto c = src.peek())
    {
        if (!priv::scnIsDigitForRadix(*c, base))
            break;

        gotAny = true;

        if (!overflow)
        {
            const auto digit = static_cast<T>(priv::scnDigitValue(*c));
            const T    div   = static_cast<T>(limit / base);
            const T    mod   = static_cast<T>(limit % base);

            if (result > div || (result == div && digit > mod))
                overflow = true;
            else
                result = static_cast<T>(result * base + digit);
        }

        src.consume();
    }

    if (!gotAny || overflow)
        return false;

    out = result;
    return true;
}

} // namespace za


////////////////////////////////////////////////////////////
/// \file
/// Built-in `scnArg` overloads for integers, floats, and `bool`,
/// plus `scnRadix<T>` for explicit-radix unsigned parsing.
///
/// Header-only: decimal integer parsing is incremental and has no scratch
/// limit. Float and explicit-radix parsers use bounded scratch buffers; if
/// a digit run exceeds the scratch size, they consume the rest of that run
/// and fail rather than succeeding on a truncated prefix.
///
////////////////////////////////////////////////////////////
