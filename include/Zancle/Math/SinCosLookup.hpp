#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Fabs.hpp"

#include "Zancle/Base/AssertAndAssume.hpp"
#include "Zancle/Base/IntTypes.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
enum : U32
{
    sinTableBits    = 12u,
    sinTableSize    = 1u << sinTableBits, // 4096 entries per turn (16 KiB)
    sinTableMask    = sinTableSize - 1u,
    sinTableQuarter = sinTableSize / 4u
};


////////////////////////////////////////////////////////////
/// Scales radians to (fractional) table indices. With this exact `float`
/// value, `za::halfPi * k` and `za::pi * k` map to exact integer indices,
/// so multiples of 90 degrees yield exactly `0`, `1`, or `-1`.
///
////////////////////////////////////////////////////////////
inline constexpr float radToSinTableIndex = static_cast<float>(sinTableSize) / tau;


////////////////////////////////////////////////////////////
/// Angle between two consecutive table entries, in radians
///
////////////////////////////////////////////////////////////
inline constexpr float sinTableStep = tau / static_cast<float>(sinTableSize);


////////////////////////////////////////////////////////////
/// Magnitude limit that keeps the scaled angle within `I32` range
///
////////////////////////////////////////////////////////////
inline constexpr float sinLookupMaxRadians = 1'000'000.f;


////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] constexpr double sinTableTaylorSin(const double x) noexcept // `|x| <= pi/4`
{
    const double xSquared = x * x;

    double term   = x;
    double result = x;

    for (int i = 1; i <= 9; ++i)
    {
        term *= -xSquared / static_cast<double>((2 * i) * (2 * i + 1));
        result += term;
    }

    return result;
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] constexpr double sinTableTaylorCos(const double x) noexcept // `|x| <= pi/4`
{
    const double xSquared = x * x;

    double term   = 1.;
    double result = 1.;

    for (int i = 1; i <= 9; ++i)
    {
        term *= -xSquared / static_cast<double>((2 * i - 1) * (2 * i));
        result += term;
    }

    return result;
}


////////////////////////////////////////////////////////////
/// \brief Value of table entry `i`: `sin(i * tau / sinTableSize)` rounded to `float`
///
/// Computed in double precision via quadrant/octant symmetry, so that
/// multiples of 90 degrees are exactly `0`/`1`/`-1` and the table is
/// exactly (anti)symmetric. Used both to fill the table and, during
/// constant evaluation, in place of the table.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] constexpr float sinTableEntry(const U32 i) noexcept
{
    constexpr double step = 6.283185307179586476925286766559 / static_cast<double>(sinTableSize);

    const U32  quadrant = (i / sinTableQuarter) & 3u;
    const U32  r        = i % sinTableQuarter; // angle within quadrant: `r * step`, in `[0, pi/2)`
    const bool useCos   = (quadrant & 1u) != 0u;

    // sin(q * pi/2 + a) is `sin(a)`, `cos(a)`, `-sin(a)`, `-cos(a)` for quadrants 0-3
    const double value = r <= sinTableQuarter / 2u
                             ? (useCos ? sinTableTaylorCos(r * step) : sinTableTaylorSin(r * step))
                             : (useCos ? sinTableTaylorSin((sinTableQuarter - r) * step)
                                       : sinTableTaylorCos((sinTableQuarter - r) * step));

    return static_cast<float>((quadrant & 2u) != 0u ? 0. - value : value); // `0. - value`: avoid `-0.f`
}


////////////////////////////////////////////////////////////
/// One turn of sine values; the cosine of entry `i` is entry `i + sinTableQuarter` (wrapped)
///
////////////////////////////////////////////////////////////
struct alignas(64) SinTable
{
    float data[sinTableSize];
};


////////////////////////////////////////////////////////////
extern const SinTable sinTable;

} // namespace za::priv


////////////////////////////////////////////////////////////
// Macros rather than functions, to minimize stack traffic in unoptimized builds.
//
// `ZA_PRIV_SIN_TABLE_COORDS` declares `sinIndex`, the wrapped table index of `radians` truncated towards
// zero, `cosIndex`, the wrapped index of the matching cosine entry (a quarter turn later), and `delta`,
// the remaining angle in `(-sinTableStep, sinTableStep)`. Truncation (rather than
// rounding to nearest) is cheaper and needs no special handling for negative angles: the first-order
// correction is valid on either side of the entry.
#define ZA_PRIV_SIN_TABLE_COORDS(radians, sinIndex, cosIndex, delta)                                                   \
    const float     zaPrivScaled    = (radians) * ::za::priv::radToSinTableIndex;                                      \
    const ::za::I32 zaPrivTruncated = static_cast<::za::I32>(zaPrivScaled);                                            \
    const float     delta           = (zaPrivScaled - static_cast<float>(zaPrivTruncated)) * ::za::priv::sinTableStep; \
    const ::za::U32 sinIndex        = static_cast<::za::U32>(zaPrivTruncated) & ::za::priv::sinTableMask;              \
    const ::za::U32 cosIndex        = (sinIndex + ::za::priv::sinTableQuarter) & ::za::priv::sinTableMask

#define ZA_PRIV_SIN_TABLE_AT(i) \
    (__builtin_is_constant_evaluated() ? ::za::priv::sinTableEntry(i) : ::za::priv::sinTable.data[i])


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Lookup-table sine of `radians` (faster than `std::sin`, less precise)
///
/// Reads an entry of a 4096-entry sine table and applies a first-order
/// correction, using the same table for the derivative. The maximum
/// absolute error is about `1.3e-6` in `[-2*Pi, 2*Pi]` and `1.5e-6` in
/// `[-4*Pi, 4*Pi]`; like any `float` function, precision degrades as
/// `|radians|` grows. Multiples of `Pi/2` (e.g. `za::halfPi * 3.f`) yield
/// exact results. Constant-evaluated calls return the same values as
/// runtime calls, up to last-bit differences when the compiler contracts
/// the correction into FMA instructions (e.g. on ARM64).
///
/// \param radians Angle in radians, any sign. `|radians|` must be less than `1e6`.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr float sinLookup(const float radians) noexcept
{
    ZA_ASSERT_AND_ASSUME(ZA_MATH_FABSF(radians) < priv::sinLookupMaxRadians); // Also rejects NaN

    ZA_PRIV_SIN_TABLE_COORDS(radians, is, ic, d);

    return ZA_PRIV_SIN_TABLE_AT(is) + d * ZA_PRIV_SIN_TABLE_AT(ic);
}


////////////////////////////////////////////////////////////
/// \brief Lookup-table cosine of `radians` (faster than `std::cos`, less precise)
///
/// See `sinLookup` for precision and range.
///
/// \param radians Angle in radians, any sign. `|radians|` must be less than `1e6`.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr float cosLookup(const float radians) noexcept
{
    ZA_ASSERT_AND_ASSUME(ZA_MATH_FABSF(radians) < priv::sinLookupMaxRadians); // Also rejects NaN

    ZA_PRIV_SIN_TABLE_COORDS(radians, is, ic, d);

    return ZA_PRIV_SIN_TABLE_AT(ic) - d * ZA_PRIV_SIN_TABLE_AT(is);
}


////////////////////////////////////////////////////////////
/// \brief Combined sine+cosine lookup for `radians` (faster than separate calls)
///
/// See `sinLookup` for precision and range.
///
/// \param radians Angle in radians, any sign. `|radians|` must be less than `1e6`.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr auto sinCosLookup(const float radians) noexcept
{
    ZA_ASSERT_AND_ASSUME(ZA_MATH_FABSF(radians) < priv::sinLookupMaxRadians); // Also rejects NaN

    struct Result
    {
        float sin, cos;
    };

    ZA_PRIV_SIN_TABLE_COORDS(radians, is, ic, d);

    const float s = ZA_PRIV_SIN_TABLE_AT(is);
    const float c = ZA_PRIV_SIN_TABLE_AT(ic);

    return Result{s + d * c, c - d * s};
}

} // namespace za


////////////////////////////////////////////////////////////
#undef ZA_PRIV_SIN_TABLE_AT
#undef ZA_PRIV_SIN_TABLE_COORDS
