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
}
