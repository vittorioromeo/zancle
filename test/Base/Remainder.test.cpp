#include "Tst/Tst.hpp"

#include "Zancle/Math/Remainder.hpp"

#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Nextafter.hpp"

#include "Zancle/Base/Memcmp.hpp"

#include <cmath>


TEST_CASE("[Base] Base/Remainder.hpp")
{
    SECTION("remainder is the IEEE remainder, like std::remainder")
    {
        volatile float opaque = 0.f; // defeat constant folding, to test the run-time code paths

        CHECK(za::remainder(opaque + 7.f, 3.f) == 1.f);
        CHECK(za::remainder(opaque + 8.f, 3.f) == -1.f); // `8 / 3` rounds to `3`, unlike `fmod`'s truncation to `2`
        CHECK(za::remainder(opaque + 190.f, 360.f) == -170.f);
        CHECK(za::remainder(opaque - 190.f, 360.f) == 170.f);
        CHECK(za::remainder(opaque + 5.f, 2.f) == 1.f);   // tie: `2.5` rounds to even `2`
        CHECK(za::remainder(opaque + 7.f, 2.f) == -1.f);  // tie: `3.5` rounds to even `4`
        CHECK(za::remainder(opaque + 1e10f, 3.f) == 1.f); // quotient beyond `int`

        CHECK(za::remainder(static_cast<double>(opaque) + 190.0, 360.0) == -170.0);
        CHECK(za::remainder(static_cast<long double>(opaque) + 190.0L, 360.0L) == -170.0L);

        // Bit for bit like `std::remainder`, over a range of magnitudes and signs
        int mismatches = 0;
        for (int i = -2000; i <= 2000; ++i)
        {
            const float a = static_cast<float>(i) * 1.37f + opaque;
            const float b = 0.1f + static_cast<float>(i < 0 ? -i : i) * 0.05f;

            const float expected = std::remainder(a, b);
            const float actual   = za::remainder(a, b);

            mismatches += ZA_MEMCMP(&expected, &actual, sizeof(float)) != 0;
        }

        CHECK(mismatches == 0);
    }

    SECTION("truncatedRemainder")
    {
        STATIC_CHECK(za::truncatedRemainder(7.f, 3.f) == 1.f);
        STATIC_CHECK(za::truncatedRemainder(-7.f, 3.f) == -1.f);
        STATIC_CHECK(za::truncatedRemainder(6.f, 3.f) == 0.f);
        STATIC_CHECK(za::truncatedRemainder(0.f, 3.f) == 0.f);
        STATIC_CHECK(za::truncatedRemainder(1.5f, 3.f) == 1.5f);
    }

    SECTION("positiveRemainder")
    {
        STATIC_CHECK(za::positiveRemainder(7.f, 3.f) == 1.f);
        STATIC_CHECK(za::positiveRemainder(-1.f, 3.f) == 2.f);
        STATIC_CHECK(za::positiveRemainder(-3.f, 3.f) == 0.f);
        STATIC_CHECK(za::positiveRemainder(-7.f, 3.f) == 2.f);
        STATIC_CHECK(za::positiveRemainder(0.f, 3.f) == 0.f);
    }

    SECTION("positiveRemainder never returns the divisor")
    {
        // Regression: `val + b` used to round to exactly `b` for tiny negative `val`
        STATIC_CHECK(za::positiveRemainder(-1e-8f, za::tau) == 0.f);
        STATIC_CHECK(za::positiveRemainder(-1e-7f, za::tau) == 0.f);
        STATIC_CHECK(za::positiveRemainder(-1e-6f, 360.f) == 0.f);

        const float divisors[] = {1.f, 2.f, 3.f, 360.f, za::pi, za::tau};

        int outOfRangeCount = 0;

        for (const float b : divisors)
            for (int i = -20'000; i <= 20'000; ++i)
            {
                const float a = static_cast<float>(i) * 0.001f * b;
                const float r = za::positiveRemainder(a, b);

                outOfRangeCount += (r < 0.f || r >= b);
            }

        CHECK(outOfRangeCount == 0);
    }

    SECTION("positiveRemainder stays in [0, b) for large magnitudes")
    {
        // Regression: when `a / b` rounds towards zero across an integer, the truncated remainder
        // ends up slightly below `-b`, and a single `+ b` correction left the result negative
        STATIC_CHECK(za::positiveRemainder(-2224.24756f, za::tau) >= 0.f);

        volatile float input = -2224.24756f; // defeat constant folding
        CHECK(za::positiveRemainder(input, za::tau) >= 0.f);
        CHECK(za::positiveRemainder(input, za::tau) < za::tau);

        // The problematic inputs are the floats closest to multiples of `b` (247 of them failed here)
        const float divisors[] = {za::tau, 360.f, 0.1f};

        int outOfRangeCount = 0;

        for (const float b : divisors)
            for (int k = -20'000; k <= 20'000; ++k)
            {
                float a = static_cast<float>(k) * b;

                for (int j = 0; j < 4; ++j)
                    a = za::nextafter(a, -1e30f);

                for (int j = 0; j < 9; ++j, a = za::nextafter(a, 1e30f))
                {
                    const float r = za::positiveRemainder(a, b);
                    outOfRangeCount += (r < 0.f || r >= b);
                }
            }

        CHECK(outOfRangeCount == 0);
    }
}
