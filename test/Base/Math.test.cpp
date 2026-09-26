#include "Tst/Tst.hpp"

#include "Zancle/Math/Frexp.hpp"
#include "Zancle/Math/Ldexp.hpp"


TEST_CASE("[Base] Base/Math.hpp")
{
    SECTION("Frexp")
    {
        int exponent{};

        CHECK(ZA_MATH_FREXPF(0.f, &exponent) == 0.f);
        CHECK(exponent == 0);

        CHECK(ZA_MATH_FREXP(0., &exponent) == 0.f);
        CHECK(exponent == 0);

        CHECK(ZA_MATH_FREXPL(0.l, &exponent) == 0.f);
        CHECK(exponent == 0);

        CHECK(za::frexp(0.f, &exponent) == 0.f);
        CHECK(exponent == 0);

        CHECK(za::frexp(0., &exponent) == 0.f);
        CHECK(exponent == 0);

        CHECK(za::frexp(0.l, &exponent) == 0.f);
        CHECK(exponent == 0);
    }

    SECTION("Frexp writes exponent")
    {
        // Regression: `za::frexp` used to be `gnu::pure`, so optimized Clang builds dropped the exponent store
        int exponent = -100;
        CHECK(za::frexp(8.f, &exponent) == 0.5f);
        CHECK(exponent == 4);

        exponent = -100;
        CHECK(za::frexp(-3., &exponent) == -0.75);
        CHECK(exponent == 2);

        exponent = -100;
        CHECK(za::frexp(0.25l, &exponent) == 0.5l);
        CHECK(exponent == -1);

        int exponentA = -100;
        int exponentB = -100;
        CHECK(za::frexp(12.f, &exponentA) == za::frexp(12.f, &exponentB));
        CHECK(exponentA == 4);
        CHECK(exponentB == 4);
    }

    SECTION("Ldexp")
    {
        int exponent{};

        CHECK(ZA_MATH_LDEXPF(0.f, exponent) == 0.f);
        CHECK(ZA_MATH_LDEXP(0., exponent) == 0.f);
        CHECK(ZA_MATH_LDEXPL(0.l, exponent) == 0.f);
        CHECK(za::ldexp(0.f, exponent) == 0.f);
        CHECK(za::ldexp(0., exponent) == 0.f);
        CHECK(za::ldexp(0.l, exponent) == 0.f);
    }
}
