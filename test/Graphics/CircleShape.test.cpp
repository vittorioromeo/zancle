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
        CHECK(circle.getPoint(1) == Approx(za::Vec2f(11.881324638f, 29.672214011f)));
        CHECK(circle.getPoint(2) == Approx(za::Vec2f(8.898950354f, 28.703181865f)));
        CHECK(circle.getPoint(3) == Approx(za::Vec2f(6.183221216f, 27.135254916f)));
        CHECK(circle.getPoint(4) == Approx(za::Vec2f(3.852827618f, 25.036959095f)));
        CHECK(circle.getPoint(5) == Approx(za::Vec2f(2.009618943f, 22.500000000f)));
        CHECK(circle.getPoint(6) == Approx(za::Vec2f(0.734152256f, 19.635254916f)));
        CHECK(circle.getPoint(7) == Approx(za::Vec2f(0.082171569f, 16.567926949f)));
        CHECK(circle.getPoint(8) == Approx(za::Vec2f(0.082171569f, 13.432073051f)));
        CHECK(circle.getPoint(9) == Approx(za::Vec2f(0.734152256f, 10.364745084f)));
        CHECK(circle.getPoint(10) == Approx(za::Vec2f(2.009618943f, 7.500000000f)));
        CHECK(circle.getPoint(11) == Approx(za::Vec2f(3.852827618f, 4.963040905f)));
        CHECK(circle.getPoint(12) == Approx(za::Vec2f(6.183221216f, 2.864745084f)));
        CHECK(circle.getPoint(13) == Approx(za::Vec2f(8.898950354f, 1.296818135f)));
        CHECK(circle.getPoint(14) == Approx(za::Vec2f(11.881324638f, 0.327785989f)));
        CHECK(circle.getPoint(15) == Approx(za::Vec2f(15.000000000f, 0.000000000f)));
        CHECK(circle.getPoint(16) == Approx(za::Vec2f(18.118675362f, 0.327785989f)));
        CHECK(circle.getPoint(17) == Approx(za::Vec2f(21.101049646f, 1.296818135f)));
        CHECK(circle.getPoint(18) == Approx(za::Vec2f(23.816778784f, 2.864745084f)));
        CHECK(circle.getPoint(19) == Approx(za::Vec2f(26.147172382f, 4.963040905f)));
        CHECK(circle.getPoint(20) == Approx(za::Vec2f(27.990381057f, 7.500000000f)));
        CHECK(circle.getPoint(21) == Approx(za::Vec2f(29.265847744f, 10.364745084f)));
        CHECK(circle.getPoint(22) == Approx(za::Vec2f(29.917828431f, 13.432073051f)));
        CHECK(circle.getPoint(23) == Approx(za::Vec2f(29.917828431f, 16.567926949f)));
        CHECK(circle.getPoint(24) == Approx(za::Vec2f(29.265847744f, 19.635254916f)));
        CHECK(circle.getPoint(25) == Approx(za::Vec2f(27.990381057f, 22.500000000f)));
        CHECK(circle.getPoint(26) == Approx(za::Vec2f(26.147172382f, 25.036959095f)));
        CHECK(circle.getPoint(27) == Approx(za::Vec2f(23.816778784f, 27.135254916f)));
        CHECK(circle.getPoint(28) == Approx(za::Vec2f(21.101049646f, 28.703181865f)));
        CHECK(circle.getPoint(29) == Approx(za::Vec2f(18.118675362f, 29.672214011f)));
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
        CHECK(circle.getPoint(1) == Approx(za::Vec2f(1.339745962f, 15.000000000f)));
        CHECK(circle.getPoint(2) == Approx(za::Vec2f(1.339745962f, 5.000000000f)));
        CHECK(circle.getPoint(3) == Approx(za::Vec2f(10.000000000f, 0.000000000f)));
        CHECK(circle.getPoint(4) == Approx(za::Vec2f(18.660254038f, 5.000000000f)));
        CHECK(circle.getPoint(5) == Approx(za::Vec2f(18.660254038f, 15.000000000f)));
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
