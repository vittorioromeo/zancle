#include "Tst/Tst.hpp"

#include "Zancle/Mixin/GlobalAnchorPointMixin.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/Vec2.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"
#include "Zancle/Trait/IsStandardLayout.hpp"
#include "Zancle/Trait/IsTrivial.hpp"
#include "Zancle/Trait/IsTriviallyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"


namespace
{
////////////////////////////////////////////////////////////
constexpr za::Rect2f testRect{{53.f, 88.f}, {512.f, 5839.f}};


////////////////////////////////////////////////////////////
// Public `Vec2f position` data member
struct TestLayoutObject : za::GlobalAnchorPointMixin
{
    constexpr TestLayoutObject() = default;

    [[nodiscard]] constexpr za::Rect2f getLocalBounds() const
    {
        return {{0.f, 0.f}, {512.f, 5839.f}};
    }

    [[nodiscard]] constexpr za::Rect2f getGlobalBounds() const
    {
        const auto localBounds = getLocalBounds();
        return {position + localBounds.position, localBounds.size};
    }

    za::Vec2f position{42.f, 55.f};
};


////////////////////////////////////////////////////////////
// Private position, exposed through `getPosition`/`setPosition`
class AccessorLayoutObject : public za::GlobalAnchorPointMixin
{
public:
    [[nodiscard]] constexpr za::Vec2f getPosition() const
    {
        return m_position;
    }

    constexpr void setPosition(const za::Vec2f position)
    {
        m_position = position;
    }

    [[nodiscard]] constexpr za::Rect2f getGlobalBounds() const
    {
        return {m_position + za::Vec2f{-8.f, 4.f}, {64.f, 32.f}};
    }

private:
    za::Vec2f m_position{10.f, 20.f};
};


////////////////////////////////////////////////////////////
// Integral position through `getPosition`/`setPosition`, like `za::WindowBase`
struct IntegralLayoutObject : za::GlobalAnchorPointMixin
{
    [[nodiscard]] constexpr za::Vec2i getPosition() const
    {
        return position;
    }

    constexpr void setPosition(const za::Vec2i newPosition)
    {
        position = newPosition;
    }

    [[nodiscard]] constexpr za::Rect2f getGlobalBounds() const
    {
        return {position.toVec2f(), size};
    }

    za::Vec2i position{0, 0};
    za::Vec2f size{101.f, 51.f};
};


////////////////////////////////////////////////////////////
// Bounds but no way to move: the setters must not be callable
class UnmovableLayoutObject : public za::GlobalAnchorPointMixin
{
public:
    [[nodiscard]] constexpr za::Rect2f getGlobalBounds() const
    {
        return {position, {10.f, 10.f}};
    }

private:
    za::Vec2f position{}; // NOLINT(readability-identifier-naming): named like a movable object's, but inaccessible
};


////////////////////////////////////////////////////////////
// Concepts, so that invalid calls yield `false` instead of a hard error
template <typename T>
concept CanSetGlobalCenter = requires(T& o) { o.setGlobalCenter(za::Vec2f{}); };

template <typename T>
concept CanSetGlobalAnchorPoint = requires(T& o) { o.setGlobalAnchorPoint(za::Vec2f{}, za::Vec2f{}); };

template <typename T>
concept CanSetGlobalLeft = requires(T& o) { o.setGlobalLeft(0.f); };

template <typename T>
concept CanSetGlobalCenterY = requires(T& o) { o.setGlobalCenterY(0.f); };

template <typename T>
concept CanGetGlobalCenter = requires(const T& o) { o.getGlobalCenter(); };


////////////////////////////////////////////////////////////
[[nodiscard]] consteval bool doSetAnchorPointTest(za::Vec2f factors)
{
    constexpr za::Vec2f newPos{24.f, 24.f};

    TestLayoutObject testObject;
    testObject.setGlobalAnchorPoint(factors, newPos);
    return testObject.position == newPos - za::Vec2f{testRect.size.x * factors.x, testRect.size.y * factors.y};
}


////////////////////////////////////////////////////////////
// Every named setter moves the matching anchor exactly to the requested point, without resizing
template <typename T>
[[nodiscard]] constexpr bool namedSettersRoundTrip(const za::Vec2f target)
{
    bool ok = true;

#define ZA_TEST_ROUND_TRIP(name)                               \
    {                                                          \
        T          object;                                     \
        const auto sizeBefore = object.getGlobalBounds().size; \
        object.setGlobal##name(target);                        \
        ok &= object.getGlobal##name() == target;              \
        ok &= object.getGlobalBounds().size == sizeBefore;     \
    }

    ZA_TEST_ROUND_TRIP(TopLeft);
    ZA_TEST_ROUND_TRIP(TopCenter);
    ZA_TEST_ROUND_TRIP(TopRight);
    ZA_TEST_ROUND_TRIP(CenterLeft);
    ZA_TEST_ROUND_TRIP(Center);
    ZA_TEST_ROUND_TRIP(CenterRight);
    ZA_TEST_ROUND_TRIP(BottomLeft);
    ZA_TEST_ROUND_TRIP(BottomCenter);
    ZA_TEST_ROUND_TRIP(BottomRight);

#undef ZA_TEST_ROUND_TRIP

    return ok;
}


////////////////////////////////////////////////////////////
// Every coordinate setter moves the object along one axis only
template <typename T>
[[nodiscard]] constexpr bool coordinateSettersRoundTrip(const float target)
{
    bool ok = true;

#define ZA_TEST_ROUND_TRIP(name, otherAxisGetter)               \
    {                                                           \
        T           object;                                     \
        const float otherAxisBefore = object.otherAxisGetter(); \
        object.setGlobal##name(target);                         \
        ok &= object.getGlobal##name() == target;               \
        ok &= object.otherAxisGetter() == otherAxisBefore;      \
    }

    ZA_TEST_ROUND_TRIP(Left, getGlobalTop);
    ZA_TEST_ROUND_TRIP(Right, getGlobalTop);
    ZA_TEST_ROUND_TRIP(CenterX, getGlobalTop);
    ZA_TEST_ROUND_TRIP(Top, getGlobalLeft);
    ZA_TEST_ROUND_TRIP(Bottom, getGlobalLeft);
    ZA_TEST_ROUND_TRIP(CenterY, getGlobalLeft);

#undef ZA_TEST_ROUND_TRIP

    return ok;
}

} // namespace


TEST_CASE("[System] za::GlobalAnchorPointMixin")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::GlobalAnchorPointMixin));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::GlobalAnchorPointMixin));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::GlobalAnchorPointMixin));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::GlobalAnchorPointMixin));

        STATIC_CHECK(ZA_IS_TRIVIAL(za::GlobalAnchorPointMixin));
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(za::GlobalAnchorPointMixin));
        STATIC_CHECK(ZA_IS_AGGREGATE(za::GlobalAnchorPointMixin));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::GlobalAnchorPointMixin));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::GlobalAnchorPointMixin));
        STATIC_CHECK(ZA_IS_TRIVIALLY_ASSIGNABLE(za::GlobalAnchorPointMixin, za::GlobalAnchorPointMixin));
    }

    SECTION("getAnchorPoint")
    {
        constexpr TestLayoutObject testObject;

        STATIC_CHECK(testObject.getGlobalAnchorPoint({0.f, 0.f}) == testObject.getGlobalTopLeft());
        STATIC_CHECK(testObject.getGlobalAnchorPoint({0.5f, 0.f}) == testObject.getGlobalTopCenter());
        STATIC_CHECK(testObject.getGlobalAnchorPoint({1.f, 0.f}) == testObject.getGlobalTopRight());
        STATIC_CHECK(testObject.getGlobalAnchorPoint({0.f, 0.5f}) == testObject.getGlobalCenterLeft());
        STATIC_CHECK(testObject.getGlobalAnchorPoint({0.5f, 0.5f}) == testObject.getGlobalCenter());
        STATIC_CHECK(testObject.getGlobalAnchorPoint({1.f, 0.5f}) == testObject.getGlobalCenterRight());
        STATIC_CHECK(testObject.getGlobalAnchorPoint({0.f, 1.f}) == testObject.getGlobalBottomLeft());
        STATIC_CHECK(testObject.getGlobalAnchorPoint({0.5f, 1.f}) == testObject.getGlobalBottomCenter());
        STATIC_CHECK(testObject.getGlobalAnchorPoint({1.f, 1.f}) == testObject.getGlobalBottomRight());
    }

    SECTION("setAnchorPoint")
    {
        STATIC_CHECK(doSetAnchorPointTest({0.f, 0.f}));
        STATIC_CHECK(doSetAnchorPointTest({0.5f, 0.f}));
        STATIC_CHECK(doSetAnchorPointTest({1.f, 0.f}));
        STATIC_CHECK(doSetAnchorPointTest({0.f, 0.5f}));
        STATIC_CHECK(doSetAnchorPointTest({0.5f, 0.5f}));
        STATIC_CHECK(doSetAnchorPointTest({1.f, 0.5f}));
        STATIC_CHECK(doSetAnchorPointTest({0.f, 1.f}));
        STATIC_CHECK(doSetAnchorPointTest({0.5f, 1.f}));
        STATIC_CHECK(doSetAnchorPointTest({1.f, 1.f}));
    }

    SECTION("Coordinate and size getters")
    {
        constexpr TestLayoutObject testObject; // bounds: {42, 55}, {512, 5839}

        STATIC_CHECK(testObject.getGlobalLeft() == 42.f);
        STATIC_CHECK(testObject.getGlobalRight() == 42.f + 512.f);
        STATIC_CHECK(testObject.getGlobalTop() == 55.f);
        STATIC_CHECK(testObject.getGlobalBottom() == 55.f + 5839.f);
        STATIC_CHECK(testObject.getGlobalCenterX() == 42.f + 256.f);
        STATIC_CHECK(testObject.getGlobalCenterY() == 55.f + 2919.5f);
        STATIC_CHECK(testObject.getGlobalWidth() == 512.f);
        STATIC_CHECK(testObject.getGlobalHeight() == 5839.f);

        STATIC_CHECK(testObject.getGlobalCenterX() == testObject.getGlobalCenter().x);
        STATIC_CHECK(testObject.getGlobalCenterY() == testObject.getGlobalCenter().y);
    }

    SECTION("Named getters use the exact rect anchors")
    {
        // The generic formula `position + size * factors` gives `inf * 0 = NaN` for the top-left anchor
        struct UnboundedLayoutObject : za::GlobalAnchorPointMixin
        {
            [[nodiscard]] constexpr za::Rect2f getGlobalBounds() const
            {
                return {{3.f, 4.f}, {__builtin_huge_valf(), __builtin_huge_valf()}};
            }
        };

        constexpr UnboundedLayoutObject o;
        STATIC_CHECK(o.getGlobalTopLeft() == za::Vec2f{3.f, 4.f});
        STATIC_CHECK(o.getGlobalLeft() == 3.f);
        STATIC_CHECK(o.getGlobalTop() == 4.f);
    }

    SECTION("Named setters (position data member)")
    {
        STATIC_CHECK(namedSettersRoundTrip<TestLayoutObject>({24.f, -24.f}));
        STATIC_CHECK(namedSettersRoundTrip<TestLayoutObject>({-1000.f, 3000.f}));
    }

    SECTION("Named setters (getPosition/setPosition)")
    {
        STATIC_CHECK(namedSettersRoundTrip<AccessorLayoutObject>({24.f, -24.f}));
        STATIC_CHECK(namedSettersRoundTrip<AccessorLayoutObject>({-1000.f, 3000.f}));
    }

    SECTION("Coordinate setters")
    {
        STATIC_CHECK(coordinateSettersRoundTrip<TestLayoutObject>(24.f));
        STATIC_CHECK(coordinateSettersRoundTrip<TestLayoutObject>(-1000.f));
        STATIC_CHECK(coordinateSettersRoundTrip<AccessorLayoutObject>(24.f));
        STATIC_CHECK(coordinateSettersRoundTrip<AccessorLayoutObject>(-1000.f));
    }

    SECTION("Integral positions are rounded to nearest, whatever their sign")
    {
        // The exact position would be `target - size / 2`, i.e. `target - {50.5, 25.5}`
        IntegralLayoutObject object;

        object.setGlobalCenter({0.f, 0.f}); // exactly {-50.5, -25.5}: halves round upwards
        CHECK(object.position == za::Vec2i{-50, -25});

        object.setGlobalCenter({1000.f, 1000.f}); // exactly {949.5, 974.5}
        CHECK(object.position == za::Vec2i{950, 975});

        object.size = {101.4f, 51.f};
        object.setGlobalCenter({0.f, 0.f}); // exactly {-50.7, -25.5}: truncation would give -50
        CHECK(object.position.x == -51);

        object.setGlobalCenter({1000.f, 1000.f}); // exactly {949.3, 974.5}
        CHECK(object.position.x == 949);

        object.setGlobalLeft(-7.f);
        CHECK(object.position.x == -7);

        object.setGlobalRight(-7.f); // exactly -108.4
        CHECK(object.position.x == -108);
    }

    SECTION("Setters require a way to move the object")
    {
        STATIC_CHECK(CanSetGlobalCenter<TestLayoutObject>);
        STATIC_CHECK(CanSetGlobalCenter<AccessorLayoutObject>);
        STATIC_CHECK(CanSetGlobalCenter<IntegralLayoutObject>);

        // Used to compile and silently do nothing
        STATIC_CHECK(!CanSetGlobalCenter<UnmovableLayoutObject>);
        STATIC_CHECK(!CanSetGlobalAnchorPoint<UnmovableLayoutObject>);
        STATIC_CHECK(!CanSetGlobalLeft<UnmovableLayoutObject>);
        STATIC_CHECK(!CanSetGlobalCenterY<UnmovableLayoutObject>);

        // Getters only need the bounds
        STATIC_CHECK(CanGetGlobalCenter<UnmovableLayoutObject>);

        // Const objects cannot be moved
        STATIC_CHECK(!CanSetGlobalCenter<const TestLayoutObject>);
        STATIC_CHECK(!CanSetGlobalCenter<const AccessorLayoutObject>);
    }
}
