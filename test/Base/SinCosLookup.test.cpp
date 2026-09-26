#include "Tst/Tst.hpp"

#include "Zancle/Math/SinCosLookup.hpp"

#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Cos.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/Fmax.hpp"
#include "Zancle/Math/Sin.hpp"


namespace
{
////////////////////////////////////////////////////////////
[[nodiscard]] constexpr bool isPositiveZero(const float x)
{
    return x == 0.f && !__builtin_signbit(x);
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
            CHECK(!__builtin_signbit(sine == 0.f ? sine : 1.f));
            CHECK(!__builtin_signbit(cosine == 0.f ? cosine : 1.f));
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
}
