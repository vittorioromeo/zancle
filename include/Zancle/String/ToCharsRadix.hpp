#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Vocabulary/Radix.hpp" // IWYU pragma: export

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Clzll.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsIntegral.hpp"
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/MakeUnsigned.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Bits per digit of `radix` (every supported radix is a power of two), or `0` if `radix` is invalid
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] constexpr unsigned int radixDigitBits(const Radix radix) noexcept
{
    switch (radix)
    {
        case Radix::Bin:
            return 1u;
        case Radix::Oct:
            return 3u;
        case Radix::Hex:
            return 4u;
    }

    return 0u; // not one of the enumerators (e.g. `static_cast<Radix>(10)`)
}

} // namespace za::priv


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
/// buffer is too small (or `radix` is not a `Radix` enumerator).
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] constexpr char* toCharsRadix(char* const       first,
                                           const char* const last,
                                           const T           value,
                                           const Radix       radix,
                                           const bool        upperHex = false)
    requires(isIntegral<T> && !isSame<T, bool> && sizeof(T) <= sizeof(unsigned long long)) // no 128-bit integers
{
    const unsigned int digitBits = priv::radixDigitBits(radix);
    ZA_ASSERT(digitBits != 0u && "Invalid radix");

    if (digitBits == 0u) [[unlikely]]
        return nullptr;

    // Cast through the unsigned counterpart so a negative signed value is
    // re-interpreted as its two's-complement bit pattern.
    using UT        = MakeUnsigned<T>;
    const auto bits = static_cast<unsigned long long>(static_cast<UT>(value));

    // Every digit is a group of `digitBits` bits: the digit count follows from the top set bit
    // (`| 1` so that zero is written as a single digit, and `clzll(0)` is avoided)
    const auto significantBits = static_cast<unsigned int>(64 - ZA_CLZLL(bits | 1ull));
    const auto n               = static_cast<SizeT>((significantBits + digitBits - 1u) / digitBits);

    if (static_cast<SizeT>(last - first) < n)
        return nullptr;

    constexpr char    digitsLo[] = "0123456789abcdef";
    constexpr char    digitsHi[] = "0123456789ABCDEF";
    const char* const lut        = upperHex ? digitsHi : digitsLo;

    const unsigned long long digitMask = (1ull << digitBits) - 1u;

    // No divisions: shift and mask, writing straight into the output from its end
    unsigned long long rest = bits;
    for (char* p = first + n; p != first; rest >>= digitBits)
        *--p = lut[rest & digitMask];

    return first + n;
}

} // namespace za
