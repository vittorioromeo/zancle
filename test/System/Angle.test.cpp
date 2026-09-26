#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Geometry/Angle.hpp"

#include "Zancle/Math/Constants.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsTriviallyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyConstructible.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyMoveAssignable.hpp"
#include "Zancle/Trait/IsTriviallyMoveConstructible.hpp"


TEST_CASE("[System] za::Angle")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Angle));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Angle));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Angle));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Angle));

        STATIC_CHECK(ZA_IS_AGGREGATE(za::Angle));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Angle));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Angle));
        STATIC_CHECK(ZA_IS_TRIVIALLY_ASSIGNABLE(za::Angle, za::Angle));
    }

    SECTION("Construction")
    {
        constexpr za::Angle angle;
        STATIC_CHECK(angle.asDegrees() == 0.f);
        STATIC_CHECK(angle.asRadians() == 0.f);
    }

    SECTION("wrapSigned()")
    {
        STATIC_CHECK(za::Angle::Zero.wrapSigned() == za::Angle::Zero);
        CHECK(za::degrees(0).wrapSigned() == Approx(za::degrees(0)));
        CHECK(za::degrees(1).wrapSigned() == Approx(za::degrees(1)));
        CHECK(za::degrees(-1).wrapSigned() == Approx(za::degrees(-1)));
        CHECK(za::degrees(90).wrapSigned() == Approx(za::degrees(90)));
        CHECK(za::degrees(-90).wrapSigned() == Approx(za::degrees(-90)));
        CHECK(za::degrees(180).wrapSigned() == Approx(za::degrees(-180)));
        CHECK(za::degrees(-180).wrapSigned() == Approx(za::degrees(-180)));
        CHECK(za::degrees(360).wrapSigned() == Approx(za::degrees(0)));
        CHECK(za::degrees(-360).wrapSigned() == Approx(za::degrees(0)));
        CHECK(za::degrees(720).wrapSigned() == Approx(za::degrees(0)));
        CHECK(za::degrees(-720).wrapSigned() == Approx(za::degrees(0)));
    }

    SECTION("wrapUnsigned()")
    {
        STATIC_CHECK(za::Angle::Zero.wrapUnsigned() == za::Angle::Zero);
        CHECK(za::degrees(0).wrapUnsigned() == Approx(za::degrees(0)));
        CHECK(za::degrees(1).wrapUnsigned() == Approx(za::degrees(1)));
        CHECK(za::degrees(-1).wrapUnsigned() == Approx(za::degrees(359)));
        CHECK(za::degrees(90).wrapUnsigned() == Approx(za::degrees(90)));
        CHECK(za::degrees(-90).wrapUnsigned() == Approx(za::degrees(270)));
        CHECK(za::degrees(180).wrapUnsigned() == Approx(za::degrees(180)));
        CHECK(za::degrees(-180).wrapUnsigned() == Approx(za::degrees(180)));
        CHECK(za::degrees(360).wrapUnsigned() == Approx(za::degrees(0)));
        CHECK(za::degrees(-360).wrapUnsigned() == Approx(za::degrees(0)));
        CHECK(za::degrees(720).wrapUnsigned() == Approx(za::degrees(0)));
        CHECK(za::degrees(-720).wrapUnsigned() == Approx(za::degrees(0)));

        // Tiny negative angles must wrap into `[0, 2pi)`, not to exactly `2pi`
        CHECK(za::radians(-1e-8f).wrapUnsigned().asRadians() < za::tau);
        CHECK(za::radians(-1e-8f).wrapUnsigned().asRadians() >= 0.f);
    }

    SECTION("rotatedTowards()")
    {
        constexpr float pi  = za::pi;
        constexpr float tau = za::tau;

        SECTION("Current angle already equals target")
        {
            za::Angle current = za::radians(2.5f);
            za::Angle target  = za::radians(2.5f);
            REQUIRE(current.rotatedTowards(target, 0.1f).asRadians() == Approx(2.5f));
        }

        SECTION("Exact rotation by speed amount")
        {
            za::Angle current = za::radians(0.f);
            za::Angle target  = za::radians(pi / 2.f);
            REQUIRE(current.rotatedTowards(target, pi / 2).asRadians() == Approx(pi / 2));
        }

        SECTION("Speed larger than needed clamps to target")
        {
            za::Angle current = za::radians(pi / 4.f);
            za::Angle target  = za::radians(3 * pi / 4.f);
            REQUIRE(current.rotatedTowards(target, pi).asRadians() == Approx(3 * pi / 4));
        }

        SECTION("Clockwise rotation")
        {
            za::Angle current = za::radians(pi / 2.f);
            za::Angle target  = za::radians(0.f);
            za::Angle result  = current.rotatedTowards(target, pi / 4);
            REQUIRE(result.asRadians() == Approx(pi / 4));
        }

        SECTION("Counter-clockwise rotation")
        {
            za::Angle current = za::radians(0.f);
            za::Angle target  = za::radians(pi / 2.f);
            za::Angle result  = current.rotatedTowards(target, pi / 4);
            REQUIRE(result.asRadians() == Approx(pi / 4));
        }

        SECTION("Crossing tau boundary clockwise")
        {
            za::Angle current = za::radians(tau - 0.1f);
            za::Angle target  = za::radians(0.1f);
            za::Angle result  = current.rotatedTowards(target, 0.2f);
            REQUIRE(result.asRadians() == Approx(target.asRadians()));
        }

        SECTION("Crossing tau boundary counter-clockwise")
        {
            za::Angle current = za::radians(0.1f);
            za::Angle target  = za::radians(tau - 0.1f);
            za::Angle result  = current.rotatedTowards(target, 0.2f);
            REQUIRE(result.asRadians() == Approx(target.asRadians()));
        }

        SECTION("Half-circle rotation (pi difference)")
        {
            za::Angle current = za::radians(0.f);
            za::Angle target  = za::radians(pi);
            za::Angle result  = current.rotatedTowards(target, pi / 2);
            REQUIRE(result.asRadians() == Approx(pi / 2));
        }

        SECTION("Full circle wrap with negative intermediate")
        {
            za::Angle current = za::radians(0.1f);
            za::Angle target  = za::radians(tau - 0.1f);
            za::Angle result  = current.rotatedTowards(target, 0.05f);
            REQUIRE(result.asRadians() == Approx(0.05f));
        }

        SECTION("Rotation with large speed across boundary")
        {
            za::Angle current = za::radians(3 * pi / 2.f);
            za::Angle target  = za::radians(pi / 2.f);
            za::Angle result  = current.rotatedTowards(target, pi);
            REQUIRE(result.asRadians() == Approx(pi / 2));
        }

        SECTION("Normalization check")
        {
            za::Angle current = za::radians(tau + 0.5f); // Should be normalized to 0.5
            za::Angle target  = za::radians(0.7f);
            za::Angle result  = current.rotatedTowards(target, 0.3f);
            REQUIRE(result.asRadians() == Approx(0.7f));
        }

        SECTION("Snapping to an out-of-range target is normalized")
        {
            const za::Angle negative = za::radians(0.f).rotatedTowards(za::radians(-0.1f), 1.f);
            CHECK(negative.asRadians() == Approx(tau - 0.1f));

            const za::Angle large = za::radians(0.f).rotatedTowards(za::radians(tau + 0.1f), 1.f);
            CHECK(large.asRadians() == Approx(0.1f));
        }
    }

    SECTION("degrees()")
    {
        constexpr za::Angle angle = za::degrees(15);
        STATIC_CHECK(angle == za::degrees(15));
        CHECK(angle.asRadians() == Approx(0.26179939f));

        constexpr za::Angle bigAngle = za::degrees(1000);
        STATIC_CHECK(bigAngle == za::degrees(1000));
        CHECK(bigAngle.asRadians() == Approx(17.453293f));

        constexpr za::Angle bigNegativeAngle = za::degrees(-4321);
        STATIC_CHECK(bigNegativeAngle == za::degrees(-4321));
        CHECK(bigNegativeAngle.asRadians() == Approx(-75.415677f));
    }

    SECTION("radians()")
    {
        constexpr za::Angle angle = za::radians(1);
        CHECK(angle.asDegrees() == Approx(57.2957795f));
        CHECK(angle.asRadians() == Approx(1.f));

        constexpr za::Angle bigAngle = za::radians(72);
        CHECK(bigAngle.asDegrees() == Approx(4125.29612f));
        CHECK(bigAngle.asRadians() == Approx(72.f));

        constexpr za::Angle bigNegativeAngle = za::radians(-200);
        CHECK(bigNegativeAngle.asDegrees() == Approx(-11459.1559f));
        CHECK(bigNegativeAngle.asRadians() == Approx(-200.f));
    }

    SECTION("Constants")
    {
        STATIC_CHECK(za::Angle::Zero.asDegrees() == 0.f);
        STATIC_CHECK(za::Angle::Zero.asRadians() == 0.f);
    }

    SECTION("Operators")
    {
        SECTION("operator==")
        {
            STATIC_CHECK(za::Angle() == za::Angle());
            STATIC_CHECK(za::Angle() == za::Angle::Zero);
            STATIC_CHECK(za::Angle() == za::degrees(0));
            STATIC_CHECK(za::Angle() == za::radians(0));
            STATIC_CHECK(za::degrees(0) == za::radians(0));
            STATIC_CHECK(za::degrees(15) == za::degrees(15));
            STATIC_CHECK(za::radians(1) == za::radians(1));
            STATIC_CHECK(za::degrees(360) == za::degrees(360));
            STATIC_CHECK(za::degrees(720) == za::degrees(720));

            STATIC_CHECK(za::Angle::Quarter == za::degrees(90));
            STATIC_CHECK(za::Angle::Half == za::degrees(180));
            STATIC_CHECK(za::Angle::Full == za::degrees(360));
        }

        SECTION("operator!=")
        {
            STATIC_CHECK(za::Angle() != za::radians(2));
            STATIC_CHECK(za::degrees(1) != za::radians(1));
            STATIC_CHECK(za::radians(0) != za::radians(0.1f));
        }

        SECTION("operator<")
        {
            STATIC_CHECK(za::radians(0) < za::degrees(0.1f));
            STATIC_CHECK(za::degrees(0) < za::radians(0.1f));
            STATIC_CHECK(za::radians(-0.1f) < za::radians(0));
            STATIC_CHECK(za::degrees(-0.1f) < za::degrees(0));
        }

        SECTION("operator>")
        {
            STATIC_CHECK(za::radians(0.1f) > za::degrees(0));
            STATIC_CHECK(za::degrees(0.1f) > za::radians(0));
            STATIC_CHECK(za::radians(0) > za::radians(-0.1f));
            STATIC_CHECK(za::degrees(0) > za::degrees(-0.1f));
        }

        SECTION("operator<=")
        {
            STATIC_CHECK(za::radians(0) <= za::degrees(0.1f));
            STATIC_CHECK(za::degrees(0) <= za::radians(0.1f));
            STATIC_CHECK(za::radians(-0.1f) <= za::radians(0));
            STATIC_CHECK(za::degrees(-0.1f) <= za::degrees(0));

            STATIC_CHECK(za::Angle() <= za::Angle());
            STATIC_CHECK(za::Angle() <= za::Angle::Zero);
            STATIC_CHECK(za::Angle() <= za::degrees(0));
            STATIC_CHECK(za::Angle() <= za::radians(0));
            STATIC_CHECK(za::degrees(0) <= za::radians(0));
            STATIC_CHECK(za::degrees(15) <= za::degrees(15));
            STATIC_CHECK(za::radians(1) <= za::radians(1));
            STATIC_CHECK(za::degrees(360) <= za::degrees(360));
            STATIC_CHECK(za::degrees(720) <= za::degrees(720));
        }

        SECTION("operator>=")
        {
            STATIC_CHECK(za::radians(0.1f) >= za::degrees(0));
            STATIC_CHECK(za::degrees(0.1f) >= za::radians(0));
            STATIC_CHECK(za::radians(0) >= za::radians(-0.1f));
            STATIC_CHECK(za::degrees(0) >= za::degrees(-0.1f));

            STATIC_CHECK(za::Angle() >= za::Angle());
            STATIC_CHECK(za::Angle() >= za::Angle::Zero);
            STATIC_CHECK(za::Angle() >= za::degrees(0));
            STATIC_CHECK(za::Angle() >= za::radians(0));
            STATIC_CHECK(za::degrees(0) >= za::radians(0));
            STATIC_CHECK(za::degrees(15) >= za::degrees(15));
            STATIC_CHECK(za::radians(1) >= za::radians(1));
            STATIC_CHECK(za::degrees(360) >= za::degrees(360));
            STATIC_CHECK(za::degrees(720) >= za::degrees(720));
        }

        SECTION("Unary operator-")
        {
            STATIC_CHECK(-za::Angle() == za::Angle());
            STATIC_CHECK(-za::radians(-1) == za::radians(1));
            STATIC_CHECK(-za::degrees(15) == za::degrees(-15));
            STATIC_CHECK(-za::radians(1) == za::radians(-1));
        }

        SECTION("operator+")
        {
            STATIC_CHECK(za::Angle() + za::Angle() == za::Angle());
            STATIC_CHECK(za::Angle::Zero + za::radians(0.5f) == za::radians(0.5f));
            STATIC_CHECK(za::radians(6) + za::radians(0.5f) == za::radians(6.5f));
            STATIC_CHECK(za::radians(10) + za::radians(0.5f) == za::radians(10.5f));
            STATIC_CHECK(za::degrees(360) + za::degrees(360) == za::degrees(720));
        }

        SECTION("operator+=")
        {
            za::Angle angle = za::degrees(-15);
            angle += za::degrees(15);
            CHECK(angle == Approx(za::degrees(0)));
            angle += za::radians(10);
            CHECK(angle == za::radians(10));
        }

        SECTION("operator-")
        {
            STATIC_CHECK(za::Angle() - za::Angle() == za::Angle());
            STATIC_CHECK(za::radians(1) - za::radians(0.5f) == za::radians(0.5f));
            STATIC_CHECK(za::Angle::Zero - za::radians(0.5f) == za::radians(-0.5f));
            STATIC_CHECK(za::degrees(900) - za::degrees(1) == za::degrees(899));
        }

        SECTION("operator-=")
        {
            za::Angle angle = za::degrees(15);
            angle -= za::degrees(15);
            CHECK(angle == Approx(za::degrees(0)));
            angle -= za::radians(10);
            CHECK(angle == za::radians(-10));
        }

        SECTION("operator*")
        {
            STATIC_CHECK(za::radians(0) * 10 == za::Angle::Zero);
            CHECK(za::degrees(10) * 2.5f == Approx(za::degrees(25)));
            CHECK(za::degrees(100) * 10.f == Approx(za::degrees(1000)));

            STATIC_CHECK(10 * za::radians(0) == za::Angle::Zero);
            CHECK(2.5f * za::degrees(10) == Approx(za::degrees(25)));
            CHECK(10.f * za::degrees(100) == Approx(za::degrees(1000)));
        }

        SECTION("operator*=")
        {
            za::Angle angle = za::degrees(1);
            angle *= 10;
            CHECK(angle == Approx(za::degrees(10)));
        }

        SECTION("operator/")
        {
            STATIC_CHECK(za::Angle::Zero / 10 == za::Angle::Zero);
            STATIC_CHECK(za::radians(12) / 3 == za::radians(4));
            CHECK(za::degrees(10) / 2.5f == Approx(za::degrees(4)));

            STATIC_CHECK(za::Angle::Zero / za::degrees(1) == 0.f);
            STATIC_CHECK(za::degrees(10) / za::degrees(10) == 1.f);
            CHECK(za::radians(10) / za::radians(2) == Approx(5.f));
        }

        SECTION("operator/=")
        {
            za::Angle angle = za::degrees(60);
            angle /= 5;
            CHECK(angle == Approx(za::degrees(12)));
        }

        SECTION("operator%")
        {
            STATIC_CHECK(za::Angle::Zero % za::radians(0.5f) == za::Angle::Zero);
            STATIC_CHECK(za::radians(10) % za::radians(1) == za::radians(0));
            CHECK(za::degrees(90) % za::degrees(30) == Approx(za::degrees(0)));
            CHECK(za::degrees(90) % za::degrees(40) == Approx(za::degrees(10)));
            CHECK(za::degrees(-90) % za::degrees(30) == Approx(za::degrees(0)));
            CHECK(za::degrees(-90) % za::degrees(40) == Approx(za::degrees(30)));
        }

        SECTION("operator%=")
        {
            za::Angle angle = za::degrees(59);
            angle %= za::degrees(10);
            CHECK(angle == Approx(za::degrees(9)));
        }

        SECTION("operator _deg")
        {
            using namespace za::Literals;
            STATIC_CHECK(0.0_deg == za::Angle::Zero);
            STATIC_CHECK(1.0_deg == za::degrees(1));
            STATIC_CHECK(-1.0_deg == za::degrees(-1));
            STATIC_CHECK(3.14_deg == za::degrees(3.14f));
            STATIC_CHECK(-3.14_deg == za::degrees(-3.14f));

            STATIC_CHECK(0_deg == za::Angle::Zero);
            STATIC_CHECK(1_deg == za::degrees(1));
            STATIC_CHECK(-1_deg == za::degrees(-1));
            STATIC_CHECK(100_deg == za::degrees(100));
            STATIC_CHECK(-100_deg == za::degrees(-100));
        }

        SECTION("operator _rad")
        {
            using namespace za::Literals;
            STATIC_CHECK(0.0_rad == za::Angle::Zero);
            STATIC_CHECK(1.0_rad == za::radians(1));
            STATIC_CHECK(-1.0_rad == za::radians(-1));
            STATIC_CHECK(3.14_rad == za::radians(3.14f));
            STATIC_CHECK(-3.14_rad == za::radians(-3.14f));

            STATIC_CHECK(0_rad == za::Angle::Zero);
            STATIC_CHECK(1_rad == za::radians(1));
            STATIC_CHECK(-1_rad == za::radians(-1));
            STATIC_CHECK(100_rad == za::radians(100));
            STATIC_CHECK(-100_rad == za::radians(-100));
        }
    }
}
