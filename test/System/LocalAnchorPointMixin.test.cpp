#include "Tst/Tst.hpp"

#include "Zancle/Mixin/LocalAnchorPointMixin.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/Vec2.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsStandardLayout.hpp"
#include "Zancle/Trait/IsTrivial.hpp"


namespace
{
////////////////////////////////////////////////////////////
// Local bounds that do not start at the origin (e.g. text glyphs with a bearing)
struct TestLocalObject : za::LocalAnchorPointMixin
{
    [[nodiscard]] constexpr za::Rect2f getLocalBounds() const
    {
        return {{-10.f, 20.f}, {64.f, 32.f}};
    }
};


////////////////////////////////////////////////////////////
// No setters: local bounds are defined by the object's contents
template <typename T>
concept HasLocalSetters = requires(T& o) { o.setLocalCenter(za::Vec2f{}); };

} // namespace


TEST_CASE("[System] za::LocalAnchorPointMixin")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_TRIVIAL(za::LocalAnchorPointMixin));
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(za::LocalAnchorPointMixin));
        STATIC_CHECK(ZA_IS_AGGREGATE(za::LocalAnchorPointMixin));

        STATIC_CHECK(!HasLocalSetters<TestLocalObject>);
    }

    SECTION("Named anchor points")
    {
        constexpr TestLocalObject o;

        STATIC_CHECK(o.getLocalTopLeft() == za::Vec2f{-10.f, 20.f});
        STATIC_CHECK(o.getLocalTopCenter() == za::Vec2f{22.f, 20.f});
        STATIC_CHECK(o.getLocalTopRight() == za::Vec2f{54.f, 20.f});
        STATIC_CHECK(o.getLocalCenterLeft() == za::Vec2f{-10.f, 36.f});
        STATIC_CHECK(o.getLocalCenter() == za::Vec2f{22.f, 36.f});
        STATIC_CHECK(o.getLocalCenterRight() == za::Vec2f{54.f, 36.f});
        STATIC_CHECK(o.getLocalBottomLeft() == za::Vec2f{-10.f, 52.f});
        STATIC_CHECK(o.getLocalBottomCenter() == za::Vec2f{22.f, 52.f});
        STATIC_CHECK(o.getLocalBottomRight() == za::Vec2f{54.f, 52.f});
    }

    SECTION("Arbitrary anchor points")
    {
        constexpr TestLocalObject o;

        STATIC_CHECK(o.getLocalAnchorPoint({0.f, 0.f}) == o.getLocalTopLeft());
        STATIC_CHECK(o.getLocalAnchorPoint({0.5f, 0.5f}) == o.getLocalCenter());
        STATIC_CHECK(o.getLocalAnchorPoint({1.f, 1.f}) == o.getLocalBottomRight());
        STATIC_CHECK(o.getLocalAnchorPoint({0.25f, 0.75f}) == za::Vec2f{6.f, 44.f});
    }

    SECTION("Coordinates and size")
    {
        constexpr TestLocalObject o;

        STATIC_CHECK(o.getLocalLeft() == -10.f);
        STATIC_CHECK(o.getLocalRight() == 54.f);
        STATIC_CHECK(o.getLocalTop() == 20.f);
        STATIC_CHECK(o.getLocalBottom() == 52.f);
        STATIC_CHECK(o.getLocalCenterX() == 22.f);
        STATIC_CHECK(o.getLocalCenterY() == 36.f);
        STATIC_CHECK(o.getLocalWidth() == 64.f);
        STATIC_CHECK(o.getLocalHeight() == 32.f);

        STATIC_CHECK(o.getLocalCenterX() == o.getLocalCenter().x);
        STATIC_CHECK(o.getLocalCenterY() == o.getLocalCenter().y);
    }
}
