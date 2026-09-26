#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Graphics/CircleShape.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


TEST_CASE("[Graphics] za::CircleShape")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::CircleShape));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::CircleShape));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::CircleShape));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::CircleShape));
    }

    SECTION("Default constructor")
    {
        const za::CircleShape circle{{.radius = 0.f}};
        CHECK(circle.getRadius() == 0.f);
        CHECK(circle.getPointCount() == 30);
        for (za::SizeT i = 0; i < circle.getPointCount(); ++i)
            CHECK(circle.getPoint(i) == za::Vec2f{0, 0});
        CHECK(circle.getGeometricCenter() == za::Vec2f{0, 0});
    }

    SECTION("Radius constructor")
    {
        const za::CircleShape circle{{.radius = 15.f}};
        CHECK(circle.getRadius() == 15.f);
        CHECK(circle.getPointCount() == 30);
        // Winding is CW-visual: points sweep from (radius, 2*radius) to the LEFT first.
        CHECK(circle.getPoint(0) == Approx(za::Vec2f(15.000000000f, 30.000000000f)));
        CHECK(circle.getPoint(1) == Approx(za::Vec2f(11.879970551f, 29.671918869f)));
        CHECK(circle.getPoint(2) == Approx(za::Vec2f(8.897069931f, 28.702350616f)));
        CHECK(circle.getPoint(3) == Approx(za::Vec2f(6.182174683f, 27.134489059f)));
        CHECK(circle.getPoint(4) == Approx(za::Vec2f(3.851520538f, 25.035495758f)));
        CHECK(circle.getPoint(5) == Approx(za::Vec2f(2.009025574f, 22.498964310f)));
        CHECK(circle.getPoint(6) == Approx(za::Vec2f(0.733575821f, 19.633468628f)));
        CHECK(circle.getPoint(7) == Approx(za::Vec2f(0.082050323f, 16.566825867f)));
        CHECK(circle.getPoint(8) == Approx(za::Vec2f(0.082351685f, 13.430310249f)));
        CHECK(circle.getPoint(9) == Approx(za::Vec2f(0.734460831f, 10.363785744f)));
        CHECK(circle.getPoint(10) == Approx(za::Vec2f(2.010465622f, 7.498545170f)));
        CHECK(circle.getPoint(11) == Approx(za::Vec2f(3.853439331f, 4.962360382f)));
        CHECK(circle.getPoint(12) == Approx(za::Vec2f(6.184499741f, 2.863815069f)));
        CHECK(circle.getPoint(13) == Approx(za::Vec2f(8.899694443f, 1.296479702f)));
        CHECK(circle.getPoint(14) == Approx(za::Vec2f(11.882774353f, 0.327479839f)));
        CHECK(circle.getPoint(15) == Approx(za::Vec2f(15.000000000f, 0.000000000f)));
        CHECK(circle.getPoint(16) == Approx(za::Vec2f(18.120029449f, 0.328079760f)));
        CHECK(circle.getPoint(17) == Approx(za::Vec2f(21.102930069f, 1.297650099f)));
        CHECK(circle.getPoint(18) == Approx(za::Vec2f(23.817824841f, 2.865510225f)));
        CHECK(circle.getPoint(19) == Approx(za::Vec2f(26.148494720f, 4.964505196f)));
        CHECK(circle.getPoint(20) == Approx(za::Vec2f(27.990989685f, 7.501035213f)));
        CHECK(circle.getPoint(21) == Approx(za::Vec2f(29.266424179f, 10.366530418f)));
        CHECK(circle.getPoint(22) == Approx(za::Vec2f(29.917949677f, 13.433175087f)));
        CHECK(circle.getPoint(23) == Approx(za::Vec2f(29.917648315f, 16.569688797f)));
        CHECK(circle.getPoint(24) == Approx(za::Vec2f(29.265539169f, 19.636215210f)));
        CHECK(circle.getPoint(25) == Approx(za::Vec2f(27.989534378f, 22.501455307f)));
        CHECK(circle.getPoint(26) == Approx(za::Vec2f(26.146560669f, 25.037639618f)));
        CHECK(circle.getPoint(27) == Approx(za::Vec2f(23.815500259f, 27.136184692f)));
        CHECK(circle.getPoint(28) == Approx(za::Vec2f(21.100305557f, 28.703519821f)));
        CHECK(circle.getPoint(29) == Approx(za::Vec2f(18.117225647f, 29.672519684f)));
        CHECK(circle.getGeometricCenter() == za::Vec2f(15.f, 15.f));
    }

    SECTION("Radius and point count constructor")
    {
        const za::CircleShape circle{{.radius = 5.f, .pointCount = 8}};
        CHECK(circle.getRadius() == 5.f);
        CHECK(circle.getPointCount() == 8);
        CHECK(circle.getPoint(0) == Approx(za::Vec2f(5.000000000f, 10.000000000f)));
        CHECK(circle.getPoint(1) == Approx(za::Vec2f(1.464466095f, 8.535533905f)));
        CHECK(circle.getPoint(2) == Approx(za::Vec2f(0.000000000f, 5.000000000f)));
        CHECK(circle.getPoint(3) == Approx(za::Vec2f(1.464466095f, 1.464466095f)));
        CHECK(circle.getPoint(4) == Approx(za::Vec2f(5.000000000f, 0.000000000f)));
        CHECK(circle.getPoint(5) == Approx(za::Vec2f(8.535533905f, 1.464466095f)));
        CHECK(circle.getPoint(6) == Approx(za::Vec2f(10.000000000f, 4.999999523f)));
        CHECK(circle.getPoint(7) == Approx(za::Vec2f(8.535533905f, 8.535533905f)));
        CHECK(circle.getGeometricCenter() == za::Vec2f(5.f, 5.f));
    }

    SECTION("Set radius")
    {
        za::CircleShape circle{{.radius = 1.f, .pointCount = 6}};
        circle.setRadius(10.f);
        CHECK(circle.getRadius() == 10.f);
        CHECK(circle.getPointCount() == 6);
        CHECK(circle.getPoint(0) == Approx(za::Vec2f(10.000000000f, 20.000000000f)));
        CHECK(circle.getPoint(1) == Approx(za::Vec2f(1.339350224f, 14.999309540f)));
        CHECK(circle.getPoint(2) == Approx(za::Vec2f(1.340311050f, 4.999030113f)));
        CHECK(circle.getPoint(3) == Approx(za::Vec2f(10.000000000f, 0.000000000f)));
        CHECK(circle.getPoint(4) == Approx(za::Vec2f(18.660659790f, 5.000690460f)));
        CHECK(circle.getPoint(5) == Approx(za::Vec2f(18.659690022f, 15.000970840f)));
        CHECK(circle.getGeometricCenter() == za::Vec2f(10.f, 10.f));
    }

    SECTION("Set point count")
    {
        za::CircleShape circle{{.radius = 4.f, .pointCount = 10}};
        circle.setPointCount(4);
        CHECK(circle.getRadius() == 4.f);
        CHECK(circle.getPointCount() == 4);
        CHECK(circle.getPoint(0) == Approx(za::Vec2f(4.000000000f, 8.000000000f)));
        CHECK(circle.getPoint(1) == Approx(za::Vec2f(0.000000000f, 4.000000000f)));
        CHECK(circle.getPoint(2) == Approx(za::Vec2f(4.000000000f, 0.000000000f)));
        CHECK(circle.getPoint(3) == Approx(za::Vec2f(8.000000000f, 3.999999762f)));
        CHECK(circle.getGeometricCenter() == za::Vec2f(4.f, 4.f));
    }

    SECTION("Equilateral triangle")
    {
        za::CircleShape triangle{{.radius = 2.f, .pointCount = 3}};
        CHECK(triangle.getRadius() == 2.f);
        CHECK(triangle.getPointCount() == 3);
        CHECK(triangle.getPoint(0) == Approx(za::Vec2f(2.000000000f, 4.000000000f)));
        CHECK(triangle.getPoint(1) == Approx(za::Vec2f(0.268062115f, 0.999806046f)));
        CHECK(triangle.getPoint(2) == Approx(za::Vec2f(3.732131958f, 1.000138044f)));
        CHECK(triangle.getGeometricCenter() == za::Vec2f(2.f, 2.f));
    }

    SECTION("Geometric center")
    {
        SECTION("2 points")
        {
            CHECK(za::CircleShape{{.radius = 2.f, .pointCount = 2}}.getGeometricCenter() == za::Vec2f(2.f, 2.f));
        }

        SECTION("3 points")
        {
            CHECK(za::CircleShape{{.radius = 4.f, .pointCount = 3}}.getGeometricCenter() == za::Vec2f(4.f, 4.f));
        }
    }
}
