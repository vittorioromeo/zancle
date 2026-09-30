// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/String/FromChars.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/BitCast.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/MulWide.hpp"

#include "Zancle/Trait/IsSame.hpp"


////////////////////////////////////////////////////////////
// Correctly rounded decimal-to-binary floating-point conversion of
// `value == W * 10^e`, where `W` holds the significant decimal digits:
//
// 1. Fast path (Clinger): if `W` and `10^|e|` are exactly representable in
//    `T`, a single IEEE multiplication or division is correctly rounded.
// 2. Otherwise, start from an approximation and compare the value, exactly,
//    against the halfway points between adjacent `T` values, moving to the
//    neighbor until the approximation is the correctly rounded result:
//    - with 128-bit integers, if `W` has at most 19 digits and `|e| <= 19`
//      (e.g. the 17 digits of a shortest round-trip `double`);
//    - otherwise with big integers (multiplications and shifts only).
//
// Keeping `Decimal::maxDigits == 800` digits suffices for the big integers:
// halfway points between doubles have at most 767 significant digits, so
// the dropped digits can only matter when the kept ones exactly equal a
// halfway point, where their presence (`Decimal::truncated`) breaks the tie
// upwards.
////////////////////////////////////////////////////////////


namespace za::priv
{
namespace
{
////////////////////////////////////////////////////////////
template <typename T>
struct FloatFormat;


////////////////////////////////////////////////////////////
template <>
struct FloatFormat<double>
{
    using Bits = U64;

    static constexpr int  mantissaBits  = 52;    // explicit (stored) mantissa bits
    static constexpr int  minExponent   = -1074; // exponent of the least significant bit of subnormals
    static constexpr Bits maxFiniteBits = 0x7F'EF'FF'FF'FF'FF'FF'FFull;
    static constexpr Bits infinityBits  = 0x7F'F0'00'00'00'00'00'00ull;
    static constexpr Bits quietNanBits  = 0x7F'F8'00'00'00'00'00'00ull;
    static constexpr Bits signBit       = Bits{1} << 63;

    static constexpr U64 maxExactInteger     = U64{1} << 53; // fast path: exactly representable integers
    static constexpr int maxExactPowerOf10   = 22;           // fast path: `10^22` is exactly representable
    static constexpr int maxDecimalMagnitude = 309;          // values `>= 10^309` overflow
    static constexpr int minDecimalMagnitude = -323;         // values `< 10^-324` round to zero
};


////////////////////////////////////////////////////////////
template <>
struct FloatFormat<float>
{
    using Bits = U32;

    static constexpr int  mantissaBits  = 23;
    static constexpr int  minExponent   = -149;
    static constexpr Bits maxFiniteBits = 0x7F'7F'FF'FFu;
    static constexpr Bits infinityBits  = 0x7F'80'00'00u;
    static constexpr Bits quietNanBits  = 0x7F'C0'00'00u;
    static constexpr Bits signBit       = Bits{1} << 31;

    static constexpr U64 maxExactInteger     = U64{1} << 24;
    static constexpr int maxExactPowerOf10   = 10;
    static constexpr int maxDecimalMagnitude = 39;  // values `>= 10^39` overflow
    static constexpr int minDecimalMagnitude = -45; // values `< 10^-46` round to zero
};


////////////////////////////////////////////////////////////
constexpr int maxU64Digits = 19; // any 19-digit number fits in 64 bits


////////////////////////////////////////////////////////////
constexpr U64 powersOf10U64[maxU64Digits + 1] = {
    1ull,
    10ull,
    100ull,
    1000ull,
    10'000ull,
    100'000ull,
    1'000'000ull,
    10'000'000ull,
    100'000'000ull,
    1'000'000'000ull,
    10'000'000'000ull,
    100'000'000'000ull,
    1'000'000'000'000ull,
    10'000'000'000'000ull,
    100'000'000'000'000ull,
    1'000'000'000'000'000ull,
    10'000'000'000'000'000ull,
    100'000'000'000'000'000ull,
    1'000'000'000'000'000'000ull,
    10'000'000'000'000'000'000ull,
};


////////////////////////////////////////////////////////////
constexpr double exactPowersOf10Double[23] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
                                              1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};


////////////////////////////////////////////////////////////
constexpr float exactPowersOf10Float[11] = {1e0f, 1e1f, 1e2f, 1e3f, 1e4f, 1e5f, 1e6f, 1e7f, 1e8f, 1e9f, 1e10f};


////////////////////////////////////////////////////////////
/// \brief Positive finite `T` as `mantissa * 2^exponent`
///
////////////////////////////////////////////////////////////
struct Unpacked
{
    U64 mantissa;
    int exponent;
};


////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] Unpacked unpack(const typename FloatFormat<T>::Bits bits) noexcept
{
    using Format = FloatFormat<T>;
    using Bits   = typename Format::Bits;

    // Subnormals have no implicit bit, and the exponent of the smallest normals
    const Bits exponentField = bits >> Format::mantissaBits;
    const U64  fraction      = bits & ((Bits{1} << Format::mantissaBits) - 1u);

    if (exponentField == 0u)
        return {fraction, Format::minExponent};

    return {fraction | (U64{1} << Format::mantissaBits), Format::minExponent + static_cast<int>(exponentField) - 1};
}


////////////////////////////////////////////////////////////
struct U128
{
    U64 high;
    U64 low;
};


////////////////////////////////////////////////////////////
[[nodiscard]] U128 multiply(const U64 lhs, const U64 rhs) noexcept
{
    U128 result{};
    result.low = mulWide(lhs, rhs, result.high);
    return result;
}


////////////////////////////////////////////////////////////
/// \brief `value << shift`, which must fit in 128 bits
///
////////////////////////////////////////////////////////////
[[nodiscard]] U128 shiftLeft(const U128 value, const int shift) noexcept
{
    ZA_ASSERT(shift >= 0 && shift < 128);

    if (shift == 0)
        return value;

    if (shift < 64)
    {
        ZA_ASSERT((value.high >> (64 - shift)) == 0u);
        return {(value.high << shift) | (value.low >> (64 - shift)), value.low << shift};
    }

    ZA_ASSERT(value.high == 0u && (shift == 64 || (value.low >> (128 - shift)) == 0u));
    return {value.low << (shift - 64), 0u};
}


////////////////////////////////////////////////////////////
/// \brief Sign of `lhs - rhs`
///
////////////////////////////////////////////////////////////
[[nodiscard]] int compare(const U128 lhs, const U128 rhs) noexcept
{
    if (lhs.high != rhs.high)
        return lhs.high < rhs.high ? -1 : 1;

    if (lhs.low != rhs.low)
        return lhs.low < rhs.low ? -1 : 1;

    return 0;
}


////////////////////////////////////////////////////////////
/// \brief Unsigned big integer, just large enough for the halfway comparisons of `BigComparer`
///
/// The largest operands are about `10^800 * 2^1076` and `10^1123 * 2^55`, below `2^3800`.
///
////////////////////////////////////////////////////////////
class BigInt
{
public:
    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init): the limbs beyond `m_size` are never read
    explicit BigInt(const U64 value) noexcept : m_size{value != 0u ? 1 : 0}
    {
        m_limbs[0] = value;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Copies only the limbs in use
    ///
    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init): the limbs beyond `m_size` are never read
    BigInt(const BigInt& rhs) noexcept : m_size{rhs.m_size}
    {
        for (int i = 0; i < m_size; ++i)
            m_limbs[i] = rhs.m_limbs[i];
    }


    ////////////////////////////////////////////////////////////
    BigInt& operator=(const BigInt&) = delete;


    ////////////////////////////////////////////////////////////
    /// \brief `*this = *this * multiplier + addend`
    ///
    ////////////////////////////////////////////////////////////
    void mulAdd(const U64 multiplier, const U64 addend) noexcept
    {
        U64 carry = addend;

        for (int i = 0; i < m_size; ++i)
        {
            // `limb * multiplier + carry <= (2^64 - 1)^2 + 2^64 - 1 < 2^128`
            U64       high = 0u;
            const U64 low  = mulWide(m_limbs[i], multiplier, high);
            const U64 sum  = low + carry;

            carry      = high + (sum < low ? 1u : 0u);
            m_limbs[i] = sum;
        }

        if (carry != 0u)
            pushLimb(carry);
    }


    ////////////////////////////////////////////////////////////
    void mulPow10(int exponent) noexcept
    {
        for (; exponent >= maxU64Digits; exponent -= maxU64Digits)
            mulAdd(powersOf10U64[maxU64Digits], 0u);

        if (exponent > 0)
            mulAdd(powersOf10U64[exponent], 0u);
    }


    ////////////////////////////////////////////////////////////
    void mulPow2(const int exponent) noexcept
    {
        if (m_size == 0 || exponent == 0)
            return;

        const int wordShift = exponent / 64;
        const int bitShift  = exponent % 64;

        if (bitShift != 0)
        {
            U64 carry = 0u;

            for (int i = 0; i < m_size; ++i)
            {
                const U64 next = m_limbs[i] >> (64 - bitShift);
                m_limbs[i]     = (m_limbs[i] << bitShift) | carry;
                carry          = next;
            }

            if (carry != 0u)
                pushLimb(carry);
        }

        if (wordShift != 0)
        {
            ZA_ASSERT(m_size + wordShift <= maxLimbs);

            for (int i = m_size - 1; i >= 0; --i)
                m_limbs[i + wordShift] = m_limbs[i];

            for (int i = 0; i < wordShift; ++i)
                m_limbs[i] = 0u;

            m_size += wordShift;
        }
    }


    ////////////////////////////////////////////////////////////
    /// \brief Sign of `lhs - rhs`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] friend int compare(const BigInt& lhs, const BigInt& rhs) noexcept
    {
        if (lhs.m_size != rhs.m_size)
            return lhs.m_size < rhs.m_size ? -1 : 1;

        for (int i = lhs.m_size - 1; i >= 0; --i)
            if (lhs.m_limbs[i] != rhs.m_limbs[i])
                return lhs.m_limbs[i] < rhs.m_limbs[i] ? -1 : 1;

        return 0;
    }

private:
    ////////////////////////////////////////////////////////////
    static constexpr int maxLimbs = 72; // 4608 bits

    ////////////////////////////////////////////////////////////
    void pushLimb(const U64 limb) noexcept
    {
        ZA_ASSERT(m_size < maxLimbs);
        m_limbs[m_size++] = limb;
    }

    ////////////////////////////////////////////////////////////
    // Little-endian limbs; the most significant one (if any) is nonzero
    U64 m_limbs[maxLimbs];
    int m_size;
};


////////////////////////////////////////////////////////////
/// \brief Significant decimal digits of a finite, nonzero input: value `== 0.digits * 10^magnitude`
///
////////////////////////////////////////////////////////////
struct Decimal
{
    static constexpr int maxDigits = 800;

    U8   digits[maxDigits]; // most significant first, without leading or trailing zeros
    int  count     = 0;
    I64  magnitude = 0;
    bool truncated = false; // nonzero digits beyond `maxDigits` were dropped (the value is slightly larger)
};


////////////////////////////////////////////////////////////
/// \brief Exact comparisons of `digits * 10^exponent` (`digits < 2^64`, `|exponent| <= 19`) against halfway points
///
////////////////////////////////////////////////////////////
template <typename T>
struct SmallComparer
{
    ////////////////////////////////////////////////////////////
    /// \brief Sign of `value - halfway`, where `halfway` is the midpoint between the (positive) `T`
    ///        with representation `bits` and the next one
    ///
    /// `halfway == (2 * mantissa + 1) * 2^(binaryExponent - 1)`. Both sides are
    /// scaled to integers, and always fit in 128 bits: `bits` is within a few
    /// ulps of the value, so `halfway` is about the value, and (with `k == |exponent|`):
    /// - `exponent >= 0`: `digits * 10^k < 10^38 < 2^127`, and the scaled
    ///   `halfway` is about the same, or `2 * mantissa < 2^55` when scaled by `2^(1 - binaryExponent)`.
    /// - `exponent < 0`: `(2 * mantissa + 1) * 10^k < 2^54 * 10^19 < 2^118`, and the scaled
    ///   `digits` is about the same, or `2 * digits < 2^65` when `halfway` is scaled by `2^(binaryExponent - 1)`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] int operator()(const typename FloatFormat<T>::Bits bits) const noexcept
    {
        const auto [mantissa, binaryExponent] = unpack<T>(bits);

        U128 lhs = multiply(digits, exponent > 0 ? powersOf10U64[exponent] : 1u);
        U128 rhs = multiply(2u * mantissa + 1u, exponent < 0 ? powersOf10U64[-exponent] : 1u);

        if (binaryExponent - 1 >= 0)
            rhs = shiftLeft(rhs, binaryExponent - 1);
        else
            lhs = shiftLeft(lhs, 1 - binaryExponent);

        return compare(lhs, rhs);
    }

    U64 digits;
    int exponent;
};


////////////////////////////////////////////////////////////
/// \brief Exact comparisons of a `Decimal` against halfway points, with big integers
///
////////////////////////////////////////////////////////////
template <typename T>
class BigComparer
{
public:
    ////////////////////////////////////////////////////////////
    explicit BigComparer(const Decimal& decimal) noexcept :
        m_scaledDigits{0u},
        m_scaledOne{1u},
        m_truncated{decimal.truncated}
    {
        // `m_scaledDigits / m_scaledOne == digits * 10^exponent`
        for (int i = 0; i < decimal.count; i += maxU64Digits)
        {
            const int chunk = decimal.count - i < maxU64Digits ? decimal.count - i : maxU64Digits;

            U64 chunkValue = 0u;
            for (int j = 0; j < chunk; ++j)
                chunkValue = chunkValue * 10u + decimal.digits[i + j];

            m_scaledDigits.mulAdd(powersOf10U64[chunk], chunkValue);
        }

        const auto exponent = static_cast<int>(decimal.magnitude) - decimal.count;

        if (exponent > 0)
            m_scaledDigits.mulPow10(exponent);
        else
            m_scaledOne.mulPow10(-exponent);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Sign of `value - halfway`, as `SmallComparer::operator()`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] int operator()(const typename FloatFormat<T>::Bits bits) const noexcept
    {
        const auto [mantissa, binaryExponent] = unpack<T>(bits);

        BigInt lhs = m_scaledDigits;
        BigInt rhs = m_scaledOne;
        rhs.mulAdd(2u * mantissa + 1u, 0u);

        if (binaryExponent - 1 >= 0)
            rhs.mulPow2(binaryExponent - 1);
        else
            lhs.mulPow2(1 - binaryExponent);

        const int result = compare(lhs, rhs);

        // The dropped digits make the value slightly larger than `digits * 10^exponent`
        return result == 0 && m_truncated ? 1 : result;
    }

private:
    BigInt m_scaledDigits;
    BigInt m_scaledOne;
    bool   m_truncated;
};


////////////////////////////////////////////////////////////
/// \brief Correct `bits` (an approximation of the positive value, a few ulps off at most)
///        into the correctly rounded result, using `comparer`'s exact halfway comparisons
///
/// \return `ResultOutOfRange` if the value rounds to infinity or to zero
///
////////////////////////////////////////////////////////////
template <typename T, typename Comparer>
[[nodiscard]] FromCharsError roundCorrectly(typename FloatFormat<T>::Bits& bits, const Comparer& comparer) noexcept
{
    using Format = FloatFormat<T>;

    if (bits > Format::maxFiniteBits) // infinity
        bits = Format::maxFiniteBits;

    // Terminates: every step moves `bits` one ulp towards the correctly rounded result
    while (true)
    {
        // Above the midpoint with the next value (or exactly on it, with an odd mantissa): round up
        const int vsUpper = comparer(bits);
        if (vsUpper > 0 || (vsUpper == 0 && (bits & 1u) != 0u))
        {
            if (bits == Format::maxFiniteBits)
                return FromCharsError::ResultOutOfRange; // rounds to infinity

            ++bits;
            continue;
        }

        // Below the midpoint with the previous value (or exactly on it, with an odd mantissa): round down
        if (bits != 0u)
        {
            const int vsLower = comparer(static_cast<typename Format::Bits>(bits - 1u));
            if (vsLower < 0 || (vsLower == 0 && (bits & 1u) != 0u))
            {
                --bits;
                continue;
            }
        }

        break;
    }

    // A nonzero value rounding to zero
    return bits == 0u ? FromCharsError::ResultOutOfRange : FromCharsError::None;
}


////////////////////////////////////////////////////////////
/// \brief `digits * 10^exponent`, approximately (a few ulps), as a starting point for `roundCorrectly`
///
////////////////////////////////////////////////////////////
[[nodiscard]] double approximate(const U64 digits, I64 exponent) noexcept
{
    auto result = static_cast<double>(digits);

    for (; exponent >= 22; exponent -= 22)
        result *= 1e22;

    for (; exponent <= -22; exponent += 22)
        result /= 1e22;

    return exponent >= 0 ? result * exactPowersOf10Double[exponent] : result / exactPowersOf10Double[-exponent];
}


////////////////////////////////////////////////////////////
/// \brief Correctly rounded `T` nearest to `digits * 10^exponent` (nonzero, at most 19 digits)
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] FromCharsError smallDecimalToFloat(U64 digits, int exponent, typename FloatFormat<T>::Bits& bits) noexcept
{
    using Format = FloatFormat<T>;
    using Bits   = typename Format::Bits;

    // Fast path: exactly representable digits and power of ten, so that a single operation is correctly rounded.
    // Excess powers of ten move into the digits while they stay exact (e.g. `1e25` as `1000 * 1e22`).
    {
        U64 fastDigits   = digits;
        int fastExponent = exponent;

        while (fastExponent > Format::maxExactPowerOf10 && fastDigits <= Format::maxExactInteger / 10u)
        {
            fastDigits *= 10u;
            --fastExponent;
        }

        if (fastDigits <= Format::maxExactInteger && fastExponent >= -Format::maxExactPowerOf10 &&
            fastExponent <= Format::maxExactPowerOf10)
        {
            const int k = fastExponent >= 0 ? fastExponent : -fastExponent;
            T         result;

            if constexpr (ZA_IS_SAME(T, float))
            {
                const auto value = static_cast<float>(fastDigits);
                result = fastExponent >= 0 ? value * exactPowersOf10Float[k] : value / exactPowersOf10Float[k];
            }
            else
            {
                const auto value = static_cast<double>(fastDigits);
                result = fastExponent >= 0 ? value * exactPowersOf10Double[k] : value / exactPowersOf10Double[k];
            }

            bits = ZA_BIT_CAST(Bits, result);
            return FromCharsError::None;
        }
    }

    bits = ZA_BIT_CAST(Bits, static_cast<T>(approximate(digits, exponent)));

    if (exponent >= -maxU64Digits && exponent <= maxU64Digits)
        return roundCorrectly<T>(bits, SmallComparer<T>{digits, exponent});

    // Rare: e.g. `1.2345e-300`
    Decimal decimal;

    for (U64 rest = digits; rest != 0u; rest /= 10u)
        ++decimal.count;

    for (int i = decimal.count - 1; i >= 0; --i, digits /= 10u)
        decimal.digits[i] = static_cast<U8>(digits % 10u);

    decimal.magnitude = decimal.count + exponent;
    return roundCorrectly<T>(bits, BigComparer<T>{decimal});
}


////////////////////////////////////////////////////////////
/// \brief Whether `[p, last)` starts with `lowercaseWord`, ignoring case
///
////////////////////////////////////////////////////////////
[[nodiscard]] bool startsWithNoCase(const char* p, const char* const last, const char* lowercaseWord) noexcept
{
    for (; *lowercaseWord != '\0'; ++p, ++lowercaseWord)
        if (p == last || (*p | 0x20) != *lowercaseWord) // `| 0x20` lowercases ASCII letters
            return false;

    return true;
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool isNanSequenceChar(const char c) noexcept
{
    return isDigit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}


////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] FromCharsResult fromCharsImpl(const char* const first, const char* const last, T& value)
{
    using Format = FloatFormat<T>;
    using Bits   = typename Format::Bits;

    const char* p = first;

    // Sign (unlike `std::from_chars`, a leading '+' is accepted)
    bool negative = false;
    if (p != last && (*p == '-' || *p == '+'))
    {
        negative = *p == '-';
        ++p;
    }

    const Bits sign = negative ? Format::signBit : Bits{0};

    // Infinity and NaN
    if (p != last && ((*p | 0x20) == 'i' || (*p | 0x20) == 'n'))
    {
        if (startsWithNoCase(p, last, "inf"))
        {
            p += startsWithNoCase(p, last, "infinity") ? 8 : 3;
            value = ZA_BIT_CAST(T, static_cast<Bits>(Format::infinityBits | sign));
            return {p, FromCharsError::None};
        }

        if (startsWithNoCase(p, last, "nan"))
        {
            p += 3;

            // Optional `(n-char-sequence)`
            if (p != last && *p == '(')
            {
                const char* q = p + 1;
                while (q != last && isNanSequenceChar(*q))
                    ++q;

                if (q != last && *q == ')')
                    p = q + 1;
            }

            value = ZA_BIT_CAST(T, static_cast<Bits>(Format::quietNanBits | sign));
            return {p, FromCharsError::None};
        }

        return {first, FromCharsError::InvalidArgument};
    }

    // Integer and fractional digits. The first 19 significant digits (leading zeros excluded) are
    // accumulated into `digits`, and `magnitude` tracks the position of the decimal point relative
    // to the first significant digit: the value is `0.d1d2d3... * 10^magnitude`
    const char* const digitsBegin = p;

    U64  digits      = 0u;
    int  significant = 0; // number of significant digits, possibly more than those in `digits`
    bool anyDigit    = false;

    const auto accumulate = [&](const char c)
    {
        if (significant < maxU64Digits)
            digits = digits * 10u + static_cast<U64>(c - '0');

        ++significant;
    };

    for (; p != last && isDigit(*p); ++p)
    {
        anyDigit = true;

        if (significant != 0 || *p != '0')
            accumulate(*p);
    }

    I64 magnitude = significant;

    if (p != last && *p == '.')
    {
        for (++p; p != last && isDigit(*p); ++p)
        {
            anyDigit = true;

            if (significant == 0 && *p == '0')
                --magnitude; // leading zero after the point
            else
                accumulate(*p);
        }
    }

    if (!anyDigit)
        return {first, FromCharsError::InvalidArgument}; // e.g. empty, "-", or "."

    const char* const digitsEnd = p;

    // Exponent, only consumed if digits follow (e.g. "3em" parses as 3)
    if (p != last && (*p | 0x20) == 'e')
    {
        const char* q            = p + 1;
        bool        exponentSign = false;

        if (q != last && (*q == '+' || *q == '-'))
        {
            exponentSign = *q == '-';
            ++q;
        }

        if (q != last && isDigit(*q))
        {
            I64 exponent = 0;

            for (; q != last && isDigit(*q); ++q)
                if (exponent < 1'000'000) // saturate: far beyond any representable magnitude
                    exponent = exponent * 10 + (*q - '0');

            magnitude += exponentSign ? -exponent : exponent;
            p = q;
        }
    }

    const char* const end = p;

    if (significant == 0) // zero, keeping the sign
    {
        value = ZA_BIT_CAST(T, sign);
        return {end, FromCharsError::None};
    }

    // Quick range checks: the value is in `[10^(magnitude - 1), 10^magnitude)`
    if (magnitude > Format::maxDecimalMagnitude)
        return {end, FromCharsError::ResultOutOfRange};

    if (magnitude < Format::minDecimalMagnitude) // below half of the smallest subnormal: rounds to zero
        return {end, FromCharsError::ResultOutOfRange};

    Bits           bits  = 0u;
    FromCharsError error = FromCharsError::None;

    if (significant <= maxU64Digits)
    {
        error = smallDecimalToFloat<T>(digits, static_cast<int>(magnitude) - significant, bits);
    }
    else
    {
        // Long inputs: all the significant digits (up to `Decimal::maxDigits`), without trailing zeros
        Decimal decimal;
        decimal.magnitude = magnitude;

        bool leading = true;
        for (const char* q = digitsBegin; q != digitsEnd; ++q)
        {
            if (*q == '.' || (leading && *q == '0'))
                continue;

            leading = false;

            if (decimal.count < Decimal::maxDigits)
                decimal.digits[decimal.count++] = static_cast<U8>(*q - '0');
            else
                decimal.truncated |= *q != '0';
        }

        while (decimal.digits[decimal.count - 1] == 0u)
            --decimal.count;

        bits  = ZA_BIT_CAST(Bits, static_cast<T>(approximate(digits, magnitude - maxU64Digits)));
        error = roundCorrectly<T>(bits, BigComparer<T>{decimal});
    }

    if (error != FromCharsError::None)
        return {end, error};

    value = ZA_BIT_CAST(T, static_cast<Bits>(bits | sign));
    return {end, FromCharsError::None};
}

} // namespace


////////////////////////////////////////////////////////////
FromCharsResult fromCharsFloat(const char* const first, const char* const last, float& value)
{
    return fromCharsImpl(first, last, value);
}


////////////////////////////////////////////////////////////
FromCharsResult fromCharsDouble(const char* const first, const char* const last, double& value)
{
    return fromCharsImpl(first, last, value);
}

} // namespace za::priv
