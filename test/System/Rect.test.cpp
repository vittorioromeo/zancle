#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/RectUtils.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsStandardLayout.hpp"
#include "Zancle/Trait/IsTrivial.hpp"
#include "Zancle/Trait/IsTriviallyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyConstructible.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyMoveAssignable.hpp"
#include "Zancle/Trait/IsTriviallyMoveConstructible.hpp"

#include "Zancle/Base/Assert.hpp"

TEMPLATE_TEST_CASE("[System] za::Rect2", "", int, float)
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Rect2<TestType>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Rect2<TestType>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Rect2<TestType>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Rect2<TestType>));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Rect2<TestType>)); // because of member initializers
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(za::Rect2<TestType>));
        STATIC_CHECK(ZA_IS_AGGREGATE(za::Rect2<TestType>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Rect2<TestType>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Rect2<TestType>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_ASSIGNABLE(za::Rect2<TestType>, za::Rect2<TestType>));
    }

    SECTION("Construction")
    {
        SECTION("Default constructor")
        {
            constexpr za::Rect2<TestType> rectangle{};
            STATIC_CHECK(rectangle.position == za::Vec2<TestType>());
            STATIC_CHECK(rectangle.size == za::Vec2<TestType>());
        }

        SECTION("(Vec2, Vec2) constructor")
        {
            constexpr za::Vec2<TestType>  position(1, 2);
            constexpr za::Vec2<TestType>  dimension(3, 4);
            constexpr za::Rect2<TestType> rectangle(position, dimension);

            STATIC_CHECK(rectangle.position == position);
            STATIC_CHECK(rectangle.size == dimension);
        }

        SECTION("Conversion constructor")
        {
            constexpr za::Rect2f sourceRectangle{{1.f, 2.f}, {3.f, 4.f}};
            constexpr auto       rectangle = sourceRectangle.toRect2i();

            STATIC_CHECK(rectangle.position == za::Vec2i{1, 2});
            STATIC_CHECK(rectangle.size == za::Vec2i{3, 4});
        }
    }

    SECTION("contains(Vec2)")
    {
        constexpr za::Rect2<TestType> rectangle({0, 0}, {10, 10});

        STATIC_CHECK(rectangle.contains(za::Vec2<TestType>(0, 0)) == true);
        STATIC_CHECK(rectangle.contains(za::Vec2<TestType>(9, 0)) == true);
        STATIC_CHECK(rectangle.contains(za::Vec2<TestType>(0, 9)) == true);
        STATIC_CHECK(rectangle.contains(za::Vec2<TestType>(9, 9)) == true);
        STATIC_CHECK(rectangle.contains(za::Vec2<TestType>(9, 10)) == false);
        STATIC_CHECK(rectangle.contains(za::Vec2<TestType>(10, 9)) == false);
        STATIC_CHECK(rectangle.contains(za::Vec2<TestType>(10, 10)) == false);
        STATIC_CHECK(rectangle.contains(za::Vec2<TestType>(15, 15)) == false);
    }

    SECTION("findIntersection()")
    {
        constexpr za::Rect2<TestType> rectangle({0, 0}, {10, 10});
        constexpr za::Rect2<TestType> intersectingRectangle({5, 5}, {10, 10});

        const auto intersectionResult = za::findIntersection(rectangle, intersectingRectangle);
        REQUIRE(intersectionResult.hasValue());
        ZA_ASSERT(*intersectionResult == za::Rect2<TestType>({5, 5}, {5, 5}));

        constexpr za::Rect2<TestType> nonIntersectingRectangle({-5, -5}, {5, 5});
        CHECK_FALSE(za::findIntersection(rectangle, nonIntersectingRectangle).hasValue());
    }

    SECTION("intersects()")
    {
        constexpr za::Rect2<TestType> rectangle({0, 0}, {10, 10});

        STATIC_CHECK(rectangle.intersects(za::Rect2<TestType>({5, 5}, {10, 10})));
        STATIC_CHECK(rectangle.intersects(za::Rect2<TestType>({2, 2}, {1, 1})));     // fully inside
        STATIC_CHECK(rectangle.intersects(za::Rect2<TestType>({-5, -5}, {20, 20}))); // fully around
        STATIC_CHECK(!rectangle.intersects(za::Rect2<TestType>({-5, -5}, {5, 5})));  // touching corner
        STATIC_CHECK(!rectangle.intersects(za::Rect2<TestType>({10, 0}, {5, 10})));  // touching edge
        STATIC_CHECK(!rectangle.intersects(za::Rect2<TestType>({0, 20}, {10, 10}))); // disjoint on Y only

        // Negative sizes are normalized, like in `findIntersection`
        STATIC_CHECK(rectangle.intersects(za::Rect2<TestType>({15, 15}, {-10, -10})));
        STATIC_CHECK(za::Rect2<TestType>({10, 10}, {-10, -10}).intersects(za::Rect2<TestType>({5, 5}, {10, 10})));
        STATIC_CHECK(!rectangle.intersects(za::Rect2<TestType>({20, 20}, {-10, -10})));

        // Agrees with `findIntersection`
        const za::Rect2<TestType> others[] = {
            {{5, 5}, {10, 10}},
            {{-5, -5}, {5, 5}},
            {{15, 15}, {-10, -10}},
            {{10, 0}, {5, 10}},
            {{9, 9}, {1, 1}},
        };

        for (const auto& other : others)
        {
            CHECK(rectangle.intersects(other) == za::findIntersection(rectangle, other).hasValue());
            CHECK(other.intersects(rectangle) == rectangle.intersects(other));
        }
    }

    SECTION("getCenter()")
    {
        STATIC_CHECK(za::Rect2<TestType>({}, {}).getCenter() == za::Vec2<TestType>());
        STATIC_CHECK(za::Rect2<TestType>({1, 2}, {4, 6}).getCenter() == za::Vec2<TestType>(3, 5));
    }

    SECTION("Operators")
    {
        SECTION("operator==")
        {
            STATIC_CHECK(za::Rect2<TestType>() == za::Rect2<TestType>());
            STATIC_CHECK(za::Rect2<TestType>({1, 3}, {2, 5}) == za::Rect2<TestType>({1, 3}, {2, 5}));

            STATIC_CHECK_FALSE(za::Rect2<TestType>({1, 0}, {0, 0}) == za::Rect2<TestType>({0, 0}, {0, 0}));
            STATIC_CHECK_FALSE(za::Rect2<TestType>({0, 1}, {0, 0}) == za::Rect2<TestType>({0, 0}, {0, 0}));
            STATIC_CHECK_FALSE(za::Rect2<TestType>({0, 0}, {1, 0}) == za::Rect2<TestType>({0, 0}, {0, 0}));
            STATIC_CHECK_FALSE(za::Rect2<TestType>({0, 0}, {0, 1}) == za::Rect2<TestType>({0, 0}, {0, 0}));
        }

        SECTION("operator!=")
        {
            STATIC_CHECK(za::Rect2<TestType>({1, 0}, {0, 0}) != za::Rect2<TestType>({0, 0}, {0, 0}));
            STATIC_CHECK(za::Rect2<TestType>({0, 1}, {0, 0}) != za::Rect2<TestType>({0, 0}, {0, 0}));
            STATIC_CHECK(za::Rect2<TestType>({0, 0}, {1, 0}) != za::Rect2<TestType>({0, 0}, {0, 0}));
            STATIC_CHECK(za::Rect2<TestType>({0, 0}, {0, 1}) != za::Rect2<TestType>({0, 0}, {0, 0}));

            STATIC_CHECK_FALSE(za::Rect2<TestType>() != za::Rect2<TestType>());
            STATIC_CHECK_FALSE(za::Rect2<TestType>({1, 3}, {2, 5}) != za::Rect2<TestType>({1, 3}, {2, 5}));
        }
    }

    SECTION("Get anchor point")
    {
        constexpr za::Rect2<TestType> r({0, 0}, {1024, 1024});

        STATIC_CHECK(r.getAnchorPoint({0.f, 0.f}) == za::Vec2<TestType>{0, 0});
        STATIC_CHECK(r.getAnchorPoint({0.5f, 0.f}) == za::Vec2<TestType>{512, 0});
        STATIC_CHECK(r.getAnchorPoint({1.f, 0.f}) == za::Vec2<TestType>{1024, 0});
        STATIC_CHECK(r.getAnchorPoint({0.f, 0.5f}) == za::Vec2<TestType>{0, 512});
        STATIC_CHECK(r.getAnchorPoint({0.5f, 0.5f}) == za::Vec2<TestType>{512, 512});
        STATIC_CHECK(r.getAnchorPoint({1.f, 0.5f}) == za::Vec2<TestType>{1024, 512});
        STATIC_CHECK(r.getAnchorPoint({0.f, 1.f}) == za::Vec2<TestType>{0, 1024});
        STATIC_CHECK(r.getAnchorPoint({0.5f, 1.f}) == za::Vec2<TestType>{512, 1024});
        STATIC_CHECK(r.getAnchorPoint({1.f, 1.f}) == za::Vec2<TestType>{1024, 1024});
    }

    SECTION("Get anchor point offset")
    {
        constexpr za::Rect2<TestType> r({0, 0}, {1000, 1000});

        STATIC_CHECK(r.getAnchorPointOffset({0.f, 0.f}) == za::Vec2<TestType>{0, 0});
        STATIC_CHECK(r.getAnchorPointOffset({0.5f, 0.f}) == za::Vec2<TestType>{-500, 0});
        STATIC_CHECK(r.getAnchorPointOffset({1.f, 0.f}) == za::Vec2<TestType>{-1000, 0});

        STATIC_CHECK(r.getAnchorPointOffset({0.f, 0.5f}) == za::Vec2<TestType>{0, -500});
        STATIC_CHECK(r.getAnchorPointOffset({0.5f, 0.5f}) == za::Vec2<TestType>{-500, -500});
        STATIC_CHECK(r.getAnchorPointOffset({1.f, 0.5f}) == za::Vec2<TestType>{-1000, -500});

        STATIC_CHECK(r.getAnchorPointOffset({0.f, 1.f}) == za::Vec2<TestType>{0, -1000});
        STATIC_CHECK(r.getAnchorPointOffset({0.5f, 1.f}) == za::Vec2<TestType>{-500, -1000});
        STATIC_CHECK(r.getAnchorPointOffset({1.f, 1.f}) == za::Vec2<TestType>{-1000, -1000});
    }

    SECTION("Set anchor point")
    {
        const auto doTest = [](za::Vec2f factors, za::Vec2<TestType> expected)
        {
            za::Rect2<TestType> r({0, 0}, {1000, 1000});
            r.setAnchorPoint(factors, {500, 500});
            CHECK(r.position == expected);
        };

        doTest({0.f, 0.f}, {500, 500});
        doTest({0.5f, 0.f}, {0, 500});
        doTest({1.f, 0.f}, {-500, 500});

        doTest({0.f, 0.5f}, {500, 0});
        doTest({0.5f, 0.5f}, {0, 0});
        doTest({1.f, 0.5f}, {-500, 0});

        doTest({0.f, 1.f}, {500, -500});
        doTest({0.5f, 1.f}, {0, -500});
        doTest({1.f, 1.f}, {-500, -500});
    }

    SECTION("Named anchor points match `getAnchorPoint`")
    {
        // Odd sizes exercise the truncation of integral halves
        constexpr za::Rect2<TestType> r({1, 2}, {7, 9});

        STATIC_CHECK(r.getTopLeft() == r.getAnchorPoint({0.f, 0.f}));
        STATIC_CHECK(r.getTopCenter() == r.getAnchorPoint({0.5f, 0.f}));
        STATIC_CHECK(r.getTopRight() == r.getAnchorPoint({1.f, 0.f}));
        STATIC_CHECK(r.getCenterLeft() == r.getAnchorPoint({0.f, 0.5f}));
        STATIC_CHECK(r.getCenter() == r.getAnchorPoint({0.5f, 0.5f}));
        STATIC_CHECK(r.getCenterRight() == r.getAnchorPoint({1.f, 0.5f}));
        STATIC_CHECK(r.getBottomLeft() == r.getAnchorPoint({0.f, 1.f}));
        STATIC_CHECK(r.getBottomCenter() == r.getAnchorPoint({0.5f, 1.f}));
        STATIC_CHECK(r.getBottomRight() == r.getAnchorPoint({1.f, 1.f}));

        STATIC_CHECK(r.getTopLeft() == za::Vec2<TestType>{r.getLeft(), r.getTop()});
        STATIC_CHECK(r.getBottomRight() == za::Vec2<TestType>{r.getRight(), r.getBottom()});
    }

    SECTION("Named anchor setters round-trip")
    {
        constexpr za::Vec2<TestType> target{100, 200};

#define ZA_PRIV_CHECK_ANCHOR_ROUNDTRIP(setter, getter, factors) \
    do                                                          \
    {                                                           \
        za::Rect2<TestType> r({1, 2}, {7, 9});                  \
        r.setter(target);                                       \
        CHECK(r.getter() == target);                            \
        CHECK(r.size == za::Vec2<TestType>{7, 9});              \
                                                                \
        za::Rect2<TestType> r2({1, 2}, {7, 9});                 \
        r2.setAnchorPoint(factors, target);                     \
        CHECK(r2.position == r.position);                       \
    } while (false)

        ZA_PRIV_CHECK_ANCHOR_ROUNDTRIP(setTopLeft, getTopLeft, za::Vec2f(0.f, 0.f));
        ZA_PRIV_CHECK_ANCHOR_ROUNDTRIP(setTopCenter, getTopCenter, za::Vec2f(0.5f, 0.f));
        ZA_PRIV_CHECK_ANCHOR_ROUNDTRIP(setTopRight, getTopRight, za::Vec2f(1.f, 0.f));
        ZA_PRIV_CHECK_ANCHOR_ROUNDTRIP(setCenterLeft, getCenterLeft, za::Vec2f(0.f, 0.5f));
        ZA_PRIV_CHECK_ANCHOR_ROUNDTRIP(setCenter, getCenter, za::Vec2f(0.5f, 0.5f));
        ZA_PRIV_CHECK_ANCHOR_ROUNDTRIP(setCenterRight, getCenterRight, za::Vec2f(1.f, 0.5f));
        ZA_PRIV_CHECK_ANCHOR_ROUNDTRIP(setBottomLeft, getBottomLeft, za::Vec2f(0.f, 1.f));
        ZA_PRIV_CHECK_ANCHOR_ROUNDTRIP(setBottomCenter, getBottomCenter, za::Vec2f(0.5f, 1.f));
        ZA_PRIV_CHECK_ANCHOR_ROUNDTRIP(setBottomRight, getBottomRight, za::Vec2f(1.f, 1.f));

#undef ZA_PRIV_CHECK_ANCHOR_ROUNDTRIP
    }
}


TEST_CASE("[System] za::Rect2 with unsigned coordinates")
{
    constexpr za::Rect2u rectangle({10u, 10u}, {10u, 10u});

    STATIC_CHECK(rectangle.contains({10u, 10u}));
    STATIC_CHECK(rectangle.contains({19u, 19u}));
    STATIC_CHECK(!rectangle.contains({20u, 10u}));
    STATIC_CHECK(!rectangle.contains({9u, 10u}));

    STATIC_CHECK(rectangle.intersects(za::Rect2u({15u, 15u}, {10u, 10u})));
    STATIC_CHECK(!rectangle.intersects(za::Rect2u({20u, 10u}, {10u, 10u})));
    STATIC_CHECK(!rectangle.intersects(za::Rect2u({0u, 0u}, {10u, 10u})));

    const auto intersection = za::findIntersection(rectangle, za::Rect2u({15u, 5u}, {10u, 10u}));
    REQUIRE(intersection.hasValue());
    CHECK(*intersection == za::Rect2u({15u, 10u}, {5u, 5u}));
}


TEST_CASE("[System] za::Rect2 anchor point precision")
{
    SECTION("Rect2<double> does not round-trip through float")
    {
        constexpr za::Rect2<double> r({0.0, 0.0}, {1.0 / 3.0, 100'000'000.1});

        STATIC_CHECK(r.getCenter() == za::Vec2<double>{r.size.x / 2.0, r.size.y / 2.0});
        STATIC_CHECK(r.getAnchorPoint({0.5f, 0.5f}) == r.getCenter());
        STATIC_CHECK(r.getAnchorPointOffset({1.f, 1.f}) == -r.size);
    }

    SECTION("Large integral sizes are exact")
    {
        // Not representable exactly as `float`
        constexpr za::Rect2uz r({0u, 0u}, {67'108'868u, 16'777'217u});

        STATIC_CHECK(r.getBottomRight() == za::Vec2uz{r.getRight(), r.getBottom()});
        STATIC_CHECK(r.getCenter() == za::Vec2uz{33'554'434u, 8'388'608u});
    }
}
