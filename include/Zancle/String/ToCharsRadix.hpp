#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Vocabulary/Radix.hpp" // IWYU pragma: export

#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsIntegral.hpp"
#include "Zancle/Trait/MakeUnsigned.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Write `value` into `[first, last)` in the given `radix`.
///
/// The value is interpreted as its raw unsigned bit pattern; no sign is
/// emitted. For signed inputs that's the C `printf("%x" / "%o")` convention --
/// `static_cast<int>(-1)` formats as `"ffffffff"` (or `"FFFFFFFF"` with
/// `upperHex = true`), not `"-1"`.
///
/// `upperHex` is only meaningful at `Radix::Hex`; for `Bin` and `Oct` all
/// digits are single ASCII characters and the flag is ignored.
///
/// \return Pointer one past the last written character, or `nullptr` if the
/// buffer is too small.
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] constexpr char* toCharsRadix(char* const       first,
                                           const char* const last,
                                           const T           value,
                                           const Radix       radix,
                                           const bool        upperHex = false)
    requires isIntegral<T>
{
    // 64 chars covers up to a 64-bit value at `Radix::Bin` (the densest case).
    char  tmp[64];
    char* p = tmp + sizeof(tmp);

    // Cast through the unsigned counterpart so a negative signed value is
    // re-interpreted as its two's-complement bit pattern.
    using UT           = MakeUnsigned<T>;
    auto       bits    = static_cast<UT>(value);
    const auto divisor = static_cast<UT>(radix);

    constexpr char    digitsLo[] = "0123456789abcdef";
    constexpr char    digitsHi[] = "0123456789ABCDEF";
    const char* const lut        = upperHex ? digitsHi : digitsLo;

    do
    {
        *--p = lut[bits % divisor];
        bits = static_cast<UT>(bits / divisor);
    } while (bits != 0u);

    const auto n = static_cast<SizeT>((tmp + sizeof(tmp)) - p);

    if (static_cast<SizeT>(last - first) < n)
        return nullptr;

    for (SizeT i = 0u; i < n; ++i)
        first[i] = p[i];

    return first + n;
}

} // namespace za
