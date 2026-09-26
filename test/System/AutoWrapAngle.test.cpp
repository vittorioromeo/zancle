#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Geometry/AutoWrapAngle.hpp"

#include "Zancle/Geometry/Angle.hpp"

#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Remainder.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"
#include "Zancle/Trait/IsTriviallyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"


TEST_CASE("[System] za::AutoWrapAngle")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::AutoWrapAngle));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::AutoWrapAngle));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::AutoWrapAngle));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::AutoWrapAngle));

        STATIC_CHECK(!ZA_IS_AGGREGATE(za::AutoWrapAngle));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::AutoWrapAngle));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::AutoWrapAngle));
        STATIC_CHECK(ZA_IS_TRIVIALLY_ASSIGNABLE(za::AutoWrapAngle, za::AutoWrapAngle));
    }

    SECTION("Construction")
    {
        constexpr za::AutoWrapAngle angle;
        STATIC_CHECK(angle.asDegrees() == 0.f);
        STATIC_CHECK(angle.asRadians() == 0.f);
    }

    SECTION("Wrapping")
    {
        STATIC_CHECK(za::AutoWrapAngle{za::degrees(360.f)}.asRadians() == 0.f);
        STATIC_CHECK(za::AutoWrapAngle{za::degrees(180.f)}.asDegrees() == 180.f);
        CHECK(za::AutoWrapAngle{za::degrees(360.f + 180.f)}.asDegrees() == Approx(180.f));
    }

    SECTION("Operators")
    {
        constexpr za::AutoWrapAngle a{za::radians(1.f)};
        constexpr za::AutoWrapAngle b{za::radians(2.f)};

        SECTION("Equality")
        {
            STATIC_CHECK(a == a);
            STATIC_CHECK(a != b);

            // Mixed comparisons are unambiguous, in both orders
            STATIC_CHECK(a == za::radians(1.f));
            STATIC_CHECK(za::radians(1.f) == a);
            STATIC_CHECK(a != za::radians(2.f));
            STATIC_CHECK(za::radians(2.f) != a);

            // The `za::Angle` operand is wrapped before comparing
            STATIC_CHECK(za::AutoWrapAngle{} == za::degrees(360.f));
            STATIC_CHECK(za::degrees(-360.f) == za::AutoWrapAngle{});
        }

        SECTION("Relational")
        {
            STATIC_CHECK(a < b);
            STATIC_CHECK(b > a);
            STATIC_CHECK(a <= a);
            STATIC_CHECK(b >= a);
            STATIC_CHECK(a < za::radians(2.f));
            STATIC_CHECK(za::radians(0.5f) < a);
        }

        SECTION("Arithmetic")
        {
            STATIC_CHECK(a + b == za::radians(3.f));
            STATIC_CHECK(b - a == za::radians(1.f));
            STATIC_CHECK(-a == za::radians(-1.f));
            STATIC_CHECK(a * 2.f == za::radians(2.f));
            STATIC_CHECK(2.f * a == za::radians(2.f));
            STATIC_CHECK(b / 2.f == za::radians(1.f));
            STATIC_CHECK(b / a == 2.f);
            STATIC_CHECK(za::AutoWrapAngle{za::radians(3.f)} % b == za::radians(1.f));
        }
    }

    SECTION("Wrapping on modification")
    {
        za::AutoWrapAngle angle{za::degrees(350.f)};

        angle += za::degrees(20.f);
        CHECK(angle.asDegrees() == Approx(10.f));

        angle -= za::degrees(30.f);
        CHECK(angle.asDegrees() == Approx(340.f));

        angle *= 2.f;
        CHECK(angle.asDegrees() == Approx(320.f));

        angle = za::degrees(-90.f);
        CHECK(angle.asDegrees() == Approx(270.f));
    }

    SECTION("Repeated increments are not lost")
    {
        // Above `2^18` radians, adding `0.01f` to an unwrapped float accumulator
        // rounds to no change at all, so the angle would stop rotating
        za::AutoWrapAngle angle{za::radians(300'000.f)};

        CHECK(angle.asRadians() >= 0.f);
        CHECK(angle.asRadians() < za::tau);

        const float before = angle.asRadians();
        angle += za::radians(0.01f);
        CHECK(angle.asRadians() != before);
    }

    SECTION("Wrapping matches `positiveRemainder` bit for bit")
    {
        // `wrapUnsigned` returns already-wrapped angles unchanged without dividing; that fast path
        // must produce exactly what the general `positiveRemainder` path would
        const auto bits = [](const float x) { return __builtin_bit_cast(unsigned int, x); };

        const auto matches = [&](const float x)
        { return bits(za::AutoWrapAngle{za::radians(x)}.asRadians()) == bits(za::positiveRemainder(x, za::tau)); };

        STATIC_CHECK(za::radians(1.f).wrapUnsigned().asRadians() == 1.f);
        STATIC_CHECK(za::radians(za::tau).wrapUnsigned().asRadians() == 0.f);
        STATIC_CHECK(za::radians(-1e-8f).wrapUnsigned().asRadians() == 0.f);

        const float edgeCases[] =
            {0.f,
             -0.f,
             1e-30f,
             -1e-30f,
             -1e-8f,
             __builtin_nextafterf(za::tau, 0.f),
             za::tau,
             __builtin_nextafterf(za::tau, 10.f),
             -za::tau,
             za::pi,
             -za::pi,
             1000.f,
             -1000.f};

        int mismatchCount = 0;

        for (const float x : edgeCases)
            mismatchCount += !matches(x);

        for (int i = -300'000; i <= 300'000; ++i)
            mismatchCount += !matches(static_cast<float>(i) * 0.0001f);

        CHECK(mismatchCount == 0);
    }
}
