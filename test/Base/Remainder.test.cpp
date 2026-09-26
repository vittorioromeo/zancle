#include "Tst/Tst.hpp"

#include "Zancle/Math/Remainder.hpp"

#include "Zancle/Math/Constants.hpp"


TEST_CASE("[Base] Base/Remainder.hpp")
{
    SECTION("remainder")
    {
        STATIC_CHECK(za::remainder(7.f, 3.f) == 1.f);
        STATIC_CHECK(za::remainder(-7.f, 3.f) == -1.f);
        STATIC_CHECK(za::remainder(6.f, 3.f) == 0.f);
        STATIC_CHECK(za::remainder(0.f, 3.f) == 0.f);
        STATIC_CHECK(za::remainder(1.5f, 3.f) == 1.5f);
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
                    a = __builtin_nextafterf(a, -1e30f);

                for (int j = 0; j < 9; ++j, a = __builtin_nextafterf(a, 1e30f))
                {
                    const float r = za::positiveRemainder(a, b);
                    outOfRangeCount += (r < 0.f || r >= b);
                }
            }

        CHECK(outOfRangeCount == 0);
    }
}
