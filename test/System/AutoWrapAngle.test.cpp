#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Geometry/AutoWrapAngle.hpp"

#include "Zancle/Geometry/Angle.hpp"

#include "Zancle/Math/Constants.hpp"

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
}
