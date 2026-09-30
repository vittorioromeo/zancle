#include "Tst/Tst.hpp"

#include "Zancle/Math/SinCosLookup.hpp"

#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Cos.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/Fmax.hpp"
#include "Zancle/Math/Sin.hpp"

#include "Zancle/Base/BitCast.hpp"
#include "Zancle/Base/InitializerList.hpp" // IWYU pragma: keep
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Signbit.hpp"


namespace
{
////////////////////////////////////////////////////////////
[[nodiscard]] constexpr bool isPositiveZero(const float x)
{
    return x == 0.f && !ZA_SIGNBIT(x);
}


////////////////////////////////////////////////////////////
constexpr float sqrtHalf = 0.70710678118f;


////////////////////////////////////////////////////////////
// Samples cover `[-20, 20]`. Each is a single multiplication, so that it cannot be contracted into an FMA
// at runtime and differ from its compile-time counterpart.
constexpr int   sampleCount  = 2001;
constexpr int   sampleOffset = 1000;
constexpr float sampleStep   = 0.02f;


////////////////////////////////////////////////////////////
struct Samples
{
    float sin[sampleCount];
    float cos[sampleCount];
};


////////////////////////////////////////////////////////////
constexpr Samples compileTimeSamples = []
{
    Samples result{};

    for (int i = 0; i < sampleCount; ++i)
    {
        const float x = static_cast<float>(i - sampleOffset) * sampleStep;

        result.sin[i] = za::sinLookup(x);
        result.cos[i] = za::cosLookup(x);
    }

    return result;
}();

} // namespace


////////////////////////////////////////////////////////////
// Multiples of 90 degrees are exact, with no negative zeros
static_assert(isPositiveZero(za::sinLookup(0.f)) && za::cosLookup(0.f) == 1.f);
static_assert(za::sinLookup(za::halfPi) == 1.f && isPositiveZero(za::cosLookup(za::halfPi)));
static_assert(isPositiveZero(za::sinLookup(za::pi)) && za::cosLookup(za::pi) == -1.f);
static_assert(za::sinLookup(za::pi * 1.5f) == -1.f && isPositiveZero(za::cosLookup(za::pi * 1.5f)));
static_assert(za::sinLookup(za::halfPi * 3.f) == -1.f && isPositiveZero(za::cosLookup(za::halfPi * 3.f)));
static_assert(isPositiveZero(za::sinLookup(za::tau)) && za::cosLookup(za::tau) == 1.f);

// Negative angles and angles beyond one turn need no wrapping
static_assert(za::sinLookup(-za::halfPi) == -1.f && isPositiveZero(za::cosLookup(-za::halfPi)));
static_assert(isPositiveZero(za::sinLookup(-za::pi)) && za::cosLookup(-za::pi) == -1.f);
static_assert(isPositiveZero(za::sinLookup(-za::tau)) && za::cosLookup(-za::tau) == 1.f);
static_assert(isPositiveZero(za::sinLookup(za::tau * 10.f)) && za::cosLookup(za::tau * 10.f) == 1.f);

// Odd multiples of 45 degrees are the correctly rounded `float` value of `sqrt(2)/2`
static_assert(za::sinLookup(za::halfPi * 0.5f) == sqrtHalf && za::cosLookup(za::halfPi * 0.5f) == sqrtHalf);
static_assert(za::sinLookup(za::halfPi * 1.5f) == sqrtHalf && za::cosLookup(za::halfPi * 1.5f) == -sqrtHalf);
static_assert(za::sinLookup(za::halfPi * 2.5f) == -sqrtHalf && za::cosLookup(za::halfPi * 2.5f) == -sqrtHalf);
static_assert(za::sinLookup(za::halfPi * 3.5f) == -sqrtHalf && za::cosLookup(za::halfPi * 3.5f) == sqrtHalf);


TEST_CASE("[Base] Base/SinCosLookup.hpp")
{
    SECTION("Table entries are correctly rounded")
    {
        int incorrectCount = 0;

        for (za::U32 i = 0u; i < za::priv::sinTableSize; ++i)
        {
            if (i % za::priv::sinTableQuarter == 0u)
                continue; // `sin` of the `double` nearest to `pi` is not exactly zero: checked below

            const double angle = static_cast<double>(i) * 6.283185307179586476925286766559 /
                                 static_cast<double>(za::priv::sinTableSize);

            incorrectCount += za::priv::sinTable.data[i] != static_cast<float>(za::sin(angle));
        }

        CHECK(incorrectCount == 0);

        CHECK(isPositiveZero(za::priv::sinTable.data[0u]));
        CHECK(za::priv::sinTable.data[za::priv::sinTableQuarter] == 1.f);
        CHECK(isPositiveZero(za::priv::sinTable.data[za::priv::sinTableQuarter * 2u]));
        CHECK(za::priv::sinTable.data[za::priv::sinTableQuarter * 3u] == -1.f);
    }

    SECTION("Runtime results match compile-time results")
    {
        // Exact unless the compiler contracts the correction into FMA instructions at runtime
        constexpr float tolerance = 2.5e-7f;

        int mismatchCount = 0;

        for (int i = 0; i < sampleCount; ++i)
        {
            volatile float input = static_cast<float>(i - sampleOffset) * sampleStep;
            const float    x     = input;

            const auto [sine, cosine] = za::sinCosLookup(x);

            mismatchCount += za::fabs(sine - compileTimeSamples.sin[i]) > tolerance;
            mismatchCount += za::fabs(cosine - compileTimeSamples.cos[i]) > tolerance;
            mismatchCount += za::sinLookup(x) != sine || za::cosLookup(x) != cosine;
        }

        CHECK(mismatchCount == 0);
    }

    SECTION("Salient angles are exact at runtime")
    {
        const struct
        {
            float x, sin, cos;
        } cases[] = {
            {0.f, 0.f, 1.f},
            {za::halfPi, 1.f, 0.f},
            {za::pi, 0.f, -1.f},
            {za::pi * 1.5f, -1.f, 0.f},
            {za::tau, 0.f, 1.f},
            {-za::halfPi, -1.f, 0.f},
            {-za::pi, 0.f, -1.f},
            {-za::tau, 0.f, 1.f},
            {za::tau * 10.f, 0.f, 1.f},
            {za::halfPi * 0.5f, sqrtHalf, sqrtHalf},
            {za::halfPi * 1.5f, sqrtHalf, -sqrtHalf},
            {za::halfPi * 2.5f, -sqrtHalf, -sqrtHalf},
            {za::halfPi * 3.5f, -sqrtHalf, sqrtHalf},
        };

        for (const auto& c : cases)
        {
            volatile float input      = c.x;
            const auto [sine, cosine] = za::sinCosLookup(input);

            CHECK(sine == c.sin);
            CHECK(cosine == c.cos);
            CHECK(!ZA_SIGNBIT(sine == 0.f ? sine : 1.f));
            CHECK(!ZA_SIGNBIT(cosine == 0.f ? cosine : 1.f));
        }
    }

    SECTION("Accuracy")
    {
        const auto maxError = [](const float range, const int steps)
        {
            double result = 0.;

            for (int i = 0; i <= steps; ++i)
            {
                volatile float input = -range + static_cast<float>(i) * (2.f * range / static_cast<float>(steps));
                const float    x     = input;

                const auto [sine, cosine] = za::sinCosLookup(x);

                result = za::fmax(result, za::fabs(static_cast<double>(sine) - za::sin(static_cast<double>(x))));
                result = za::fmax(result, za::fabs(static_cast<double>(cosine) - za::cos(static_cast<double>(x))));
            }

            return result;
        };

        CHECK(maxError(za::tau, 200'000) < 1.5e-6);
        CHECK(maxError(2.f * za::tau, 200'000) < 2e-6);
        CHECK(maxError(100.f, 200'000) < 1.5e-5);
    }

    SECTION("Results never exceed `sinCosLookupMaxMagnitude`")
    {
        // Within a table cell, the table entries are fixed and the correction `d` grows with the angle,
        // so `s + d * c` (and `c - d * s`) is monotonic there, rounding included: the largest magnitude
        // of each cell is reached at one of its ends. Checking the floats around every cell edge thus
        // covers every angle, without walking billions of floats.
        const auto magnitudeAt = [](const float x)
        {
            const auto [sine, cosine] = za::sinCosLookup(x);

            return za::fmax(za::fmax(za::fabs(sine), za::fabs(cosine)),
                            za::fmax(za::fabs(za::sinLookup(x)), za::fabs(za::cosLookup(x))));
        };

        // `radius` consecutive floats on each side of `center` (stepping through bit patterns, which
        // are ordered by magnitude), for a nonzero `center`
        const auto maxMagnitudeAround = [&](const float center, const za::U32 radius)
        {
            const auto centerBits = ZA_BIT_CAST(za::U32, center);

            float result = 0.f;
            for (za::U32 bits = centerBits - radius; bits != centerBits + radius + 1u; ++bits)
                result = za::fmax(result, magnitudeAt(ZA_BIT_CAST(float, bits)));

            return result;
        };

        // The floats around every cell edge of `turns` whole turns, from `firstAngle`
        const auto maxMagnitudeOverCellEdges = [&](const double firstAngle, const int turns)
        {
            constexpr double step = 6.283185307179586476925286766559 / static_cast<double>(za::priv::sinTableSize);

            float result = 0.f;
            for (int i = 0; i <= turns * static_cast<int>(za::priv::sinTableSize); ++i)
            {
                const auto edge = static_cast<float>(firstAngle + i * step);

                if (edge != 0.f)
                    result = za::fmax(result, maxMagnitudeAround(edge, 64u));
            }

            return result;
        };

        // Every cell in `[-2*Pi, 2*Pi]`
        const float nearZero = maxMagnitudeOverCellEdges(-6.283185307179586476925286766559, 2); // -2*Pi
        CHECK(nearZero > 1.f);                                                                  // documented overshoot
        CHECK(nearZero <= za::sinCosLookupMaxMagnitude);

        // One whole turn of cells at larger angles, where floats are sparser
        for (const double angle : {1'000.0, 10'000.0, 99'000.0})
        {
            CHECK(maxMagnitudeOverCellEdges(angle, 1) <= za::sinCosLookupMaxMagnitude);
            CHECK(maxMagnitudeOverCellEdges(-angle, 1) <= za::sinCosLookupMaxMagnitude);
        }

        // Coarse sweep of `[-1e5, 1e5]`, as a sanity check of the reasoning above
        float sweep = 0.f;
        for (const float sign : {1.f, -1.f})
            for (za::U32 bits = 0u; bits < ZA_BIT_CAST(za::U32, 100'000.f); bits += 997u)
                sweep = za::fmax(sweep, magnitudeAt(sign * ZA_BIT_CAST(float, bits)));

        CHECK(sweep <= za::sinCosLookupMaxMagnitude);
    }
}
