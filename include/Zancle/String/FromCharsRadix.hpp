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
/// Hex parsing accepts both lowercase (`a`..`f`) and uppercase (`A`..`F`)
/// digits; the choice is irrelevant for octal and binary.
///
/// Constrained to **unsigned** types so the "treat as bit pattern" model is
/// unambiguous. To parse a signed value as decimal use the existing
/// `fromChars` overload.
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] constexpr FromCharsResult fromCharsRadix(const char* first, const char* const last, T& value, const Radix radix)
    requires(isIntegral<T> && isUnsigned<T> && !isSame<T, bool>)
{
    const unsigned int digitBits = priv::radixDigitBits(radix);
    ZA_ASSERT(digitBits != 0u && "Invalid radix");

    if (first == last || digitBits == 0u)
        return {first, FromCharsError::InvalidArgument};

    const unsigned int base = 1u << digitBits;

    // Shifting in another digit overflows iff any of the top `digitBits` bits is already set
    const unsigned int overflowShift = sizeof(T) * 8u - digitBits;

    T    result   = 0;
    bool anyDigit = false;

    while (first != last)
    {
        const char c = *first;

        T digit = 0;
        if (c >= '0' && c <= '9')
            digit = static_cast<T>(c - '0');
        else if (c >= 'a' && c <= 'f')
            digit = static_cast<T>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F')
            digit = static_cast<T>(c - 'A' + 10);
        else
            break; // First non-digit character ends the parse.

        // Reject digits outside the chosen radix (e.g. '8' under `Radix::Oct`,
        // 'a' under `Radix::Bin`). This is a parse stop, not an error: matches
        // `fromChars` decimal semantics on the first non-digit byte.
        if (digit >= base)
            break;

        // Overflow check, mirroring the decimal `fromChars` (no divisions: every radix is a power of two)
        if ((result >> overflowShift) != 0u)
            return {first, FromCharsError::ResultOutOfRange};

        result = static_cast<T>((result << digitBits) | digit);
        ++first;
        anyDigit = true;
    }

    if (!anyDigit)
        return {first, FromCharsError::InvalidArgument};

    value = result;
    return {first, FromCharsError::None};
}

} // namespace za
