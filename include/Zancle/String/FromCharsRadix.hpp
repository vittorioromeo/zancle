#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/String/FromCharsResult.hpp" // IWYU pragma: export
#include "Zancle/String/ToCharsRadix.hpp"    // `priv::radixDigitBits`

#include "Zancle/Vocabulary/Radix.hpp"

#include "Zancle/Base/Assert.hpp"

#include "Zancle/Trait/IsIntegral.hpp"
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/IsUnsigned.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Value of the hexadecimal digit `c`, or `16` if `c` is not a hexadecimal digit
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline constexpr unsigned int hexDigitValue(const char c) noexcept
{
    if (c >= '0' && c <= '9')
        return static_cast<unsigned int>(c - '0');

    if (c >= 'a' && c <= 'f')
        return static_cast<unsigned int>(c - 'a' + 10);

    if (c >= 'A' && c <= 'F')
        return static_cast<unsigned int>(c - 'A' + 10);

    return 16u;
}

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Parse an unsigned integer from `[first, last)` in the given `radix`.
///
/// Symmetric counterpart to `toCharsRadix`: rejects a leading sign (a hex /
/// oct / binary literal has no sign in C-protocol contexts), stops at the
/// first character that is not a valid digit for the chosen radix, and
/// signals overflow via `FromCharsError::ResultOutOfRange`.
///
/// On `FromCharsError::ResultOutOfRange`, the returned pointer is past all
/// the digits (not just the ones that fit), and `value` is left untouched.
/// On `FromCharsError::InvalidArgument` (no digits), the returned pointer
/// is `first`: nothing is consumed.
///
/// Hex parsing accepts both lowercase (`a`..`f`) and uppercase (`A`..`F`)
/// digits; the choice is irrelevant for octal and binary.
///
/// Constrained to **unsigned** types so the "treat as bit pattern" model is
/// unambiguous. To parse a signed value as decimal use the existing
/// `fromChars` overload.
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] constexpr FromCharsResult fromCharsRadix(const char* const first, const char* const last, T& value, const Radix radix)
    requires(isIntegral<T> && isUnsigned<T> && !isSame<T, bool>)
{
    const unsigned int digitBits = priv::radixDigitBits(radix);
    ZA_ASSERT(digitBits != 0u && "Invalid radix");

    if (first == last || digitBits == 0u)
        return {first, FromCharsError::InvalidArgument};

    const unsigned int base = 1u << digitBits;

    // Shifting in another digit overflows iff any of the top `digitBits` bits is already set
    const unsigned int overflowShift = sizeof(T) * 8u - digitBits;

    const char* p      = first;
    T           result = 0;

    // The first character that is not a digit of the chosen radix ends the parse (e.g. '8' under
    // `Radix::Oct`, 'a' under `Radix::Bin`), matching the decimal `fromChars`
    for (; p != last; ++p)
    {
        const unsigned int digit = priv::hexDigitValue(*p);
        if (digit >= base)
            break;

        // Overflow check, mirroring the decimal `fromChars` (no divisions: every radix is a power of two)
        if ((result >> overflowShift) != 0u)
        {
            // Consume the rest of the number, so that parsing can resume after it
            while (p != last && priv::hexDigitValue(*p) < base)
                ++p;

            return {p, FromCharsError::ResultOutOfRange};
        }

        result = static_cast<T>((result << digitBits) | static_cast<T>(digit));
    }

    if (p == first)
        return {first, FromCharsError::InvalidArgument};

    value = result;
    return {p, FromCharsError::None};
}

} // namespace za
