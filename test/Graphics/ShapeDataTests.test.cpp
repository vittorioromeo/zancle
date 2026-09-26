#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Graphics/ArrowShapeData.hpp"
#include "Zancle/Graphics/ChevronShapeData.hpp"
#include "Zancle/Graphics/CircleShapeData.hpp"
#include "Zancle/Graphics/CogShapeData.hpp"
#include "Zancle/Graphics/CrossShapeData.hpp"
#include "Zancle/Graphics/CurvedArrowShapeData.hpp"
#include "Zancle/Graphics/EllipseShapeData.hpp"
#include "Zancle/Graphics/HeartShapeData.hpp"
#include "Zancle/Graphics/PieSliceShapeData.hpp"
#include "Zancle/Graphics/RectangleShapeData.hpp"
#include "Zancle/Graphics/RingPieSliceShapeData.hpp"
#include "Zancle/Graphics/RingShapeData.hpp"
#include "Zancle/Graphics/RoundedRectangleShapeData.hpp"
#include "Zancle/Graphics/ShapeUtils.hpp"
#include "Zancle/Graphics/StarShapeData.hpp"
#include "Zancle/Graphics/TrapezoidShapeData.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/Vec2.hpp"

#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/Remainder.hpp"
#include "Zancle/Math/SinCosLookup.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


namespace
{
////////////////////////////////////////////////////////////
// Helper: assert the four corners of `a` approximately equal the four corners of `b`
// (comparing the min/max corners suffices since Rect2f stores position + size).
////////////////////////////////////////////////////////////
[[nodiscard]] bool rectsApproxEqual(const za::Rect2f& a, const za::Rect2f& b) noexcept
{
    return a.position == Approx(b.position) && a.size == Approx(b.size);
}


////////////////////////////////////////////////////////////
// Numerical polygon centroid via shoelace -- ground truth for shape-data `getCentroid`
// formulas. The polygon is specified by `count` vertices obtained from `pointFn(i)`.
////////////////////////////////////////////////////////////
template <typename PointFn>
[[nodiscard]] za::Vec2f referencePolygonCentroid(const unsigned int count, PointFn&& pointFn) noexcept
{
    float     area2 = 0.f;
    za::Vec2f acc{};

    za::Vec2f prev = pointFn(0u);
    for (unsigned int i = 0u; i < count; ++i)
    {
        const za::Vec2f curr  = pointFn((i + 1u) % count);
        const float     cross = prev.x * curr.y - curr.x * prev.y;
        area2 += cross;
        acc.x += (prev.x + curr.x) * cross;
        acc.y += (prev.y + curr.y) * cross;
        prev = curr;
    }

    if (area2 == 0.f)
        return {};

    return {acc.x / (3.f * area2), acc.y / (3.f * area2)};
}
} // namespace

TEST_CASE("[Graphics] za::ShapeData type traits")
{
#define ZA_PRIV_CHECK_TRAITS(T)                            \
    STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::T));         \
    STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::T));            \
    STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::T)); \
    STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::T))

    ZA_PRIV_CHECK_TRAITS(ArrowShapeData);
    ZA_PRIV_CHECK_TRAITS(ChevronShapeData);
    ZA_PRIV_CHECK_TRAITS(CircleShapeData);
    ZA_PRIV_CHECK_TRAITS(CogShapeData);
    ZA_PRIV_CHECK_TRAITS(CrossShapeData);
    ZA_PRIV_CHECK_TRAITS(CurvedArrowShapeData);
    ZA_PRIV_CHECK_TRAITS(EllipseShapeData);
    ZA_PRIV_CHECK_TRAITS(HeartShapeData);
    ZA_PRIV_CHECK_TRAITS(PieSliceShapeData);
    ZA_PRIV_CHECK_TRAITS(RectangleShapeData);
    ZA_PRIV_CHECK_TRAITS(RingPieSliceShapeData);
    ZA_PRIV_CHECK_TRAITS(RingShapeData);
    ZA_PRIV_CHECK_TRAITS(RoundedRectangleShapeData);
    ZA_PRIV_CHECK_TRAITS(StarShapeData);
    ZA_PRIV_CHECK_TRAITS(TrapezoidShapeData);

#undef ZA_PRIV_CHECK_TRAITS
}

TEST_CASE("[Graphics] za::CircleShapeData")
{
    SECTION("Default local bounds")
    {
        const za::CircleShapeData sd{};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {0.f, 0.f}}));
    }

    SECTION("Local bounds match 2*radius square")
    {
        const za::CircleShapeData sd{.radius = 10.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {20.f, 20.f}}));
    }

    SECTION("Global bounds: identity transform equals local bounds")
    {
        const za::CircleShapeData sd{.radius = 10.f};
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), sd.getLocalBounds()));
    }

    SECTION("Global bounds: translated by position only")
    {
        const za::CircleShapeData sd{.position = {100.f, 50.f}, .radius = 10.f};
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {{100.f, 50.f}, {20.f, 20.f}}));
    }

    SECTION("Global bounds: rotation-invariant under uniform scale")
    {
        const za::CircleShapeData unrotated{.radius = 10.f};
        const za::CircleShapeData rotated{.rotation = za::degrees(45.f), .radius = 10.f};
        // Without origin, rotation is about (0, 0) so the bbox position moves, but its size stays.
        CHECK(rotated.getGlobalBounds().size == Approx(unrotated.getGlobalBounds().size));
    }

    SECTION("Global bounds: rotation about center keeps bounds identical")
    {
        const za::CircleShapeData sd{.origin = {10.f, 10.f}, .rotation = za::degrees(33.f), .radius = 10.f};
        // Rotating a disk about its own center is a no-op.
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {{-10.f, -10.f}, {20.f, 20.f}}));
    }

    SECTION("Anchor points")
    {
        const za::CircleShapeData sd{.radius = 10.f};
        CHECK(sd.getLocalCenter() == Approx(za::Vec2f{10.f, 10.f}));
        CHECK(sd.getLocalTopLeft() == Approx(za::Vec2f{0.f, 0.f}));
        CHECK(sd.getLocalBottomRight() == Approx(za::Vec2f{20.f, 20.f}));
    }

    SECTION("Centroid: coincides with disk center")
    {
        const za::CircleShapeData sd{.radius = 10.f};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{10.f, 10.f}));
    }
}

TEST_CASE("[Graphics] za::EllipseShapeData")
{
    SECTION("Local bounds reflect both radii")
    {
        const za::EllipseShapeData sd{.horizontalRadius = 20.f, .verticalRadius = 10.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {40.f, 20.f}}));
    }

    SECTION("Global bounds: 90 deg rotation about center swaps dimensions")
    {
        const za::EllipseShapeData sd{.origin           = {20.f, 10.f},
                                      .rotation         = za::degrees(90.f),
                                      .horizontalRadius = 20.f,
                                      .verticalRadius   = 10.f};
        // After a 90 deg rotation, the horizontal and vertical extents swap.
        const auto gb = sd.getGlobalBounds();
        CHECK(gb.size == Approx(za::Vec2f{20.f, 40.f}));
    }

    SECTION("Global bounds: uniform scale scales bounds uniformly")
    {
        const za::EllipseShapeData sd{.scale = {2.f, 2.f}, .horizontalRadius = 10.f, .verticalRadius = 5.f};
        CHECK(sd.getGlobalBounds().size == Approx(za::Vec2f{40.f, 20.f}));
    }

    SECTION("Centroid: coincides with ellipse center")
    {
        const za::EllipseShapeData sd{.horizontalRadius = 20.f, .verticalRadius = 10.f};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{20.f, 10.f}));
    }
}

TEST_CASE("[Graphics] za::RingShapeData")
{
    SECTION("Local bounds ignore inner radius")
    {
        const za::RingShapeData sd{.outerRadius = 30.f, .innerRadius = 10.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {60.f, 60.f}}));
    }

    SECTION("Global bounds: tight under rotation")
    {
        const za::RingShapeData sd{.origin = {30.f, 30.f}, .rotation = za::degrees(45.f), .outerRadius = 30.f, .innerRadius = 15.f};
        // Ring is rotation-invariant around its own center: bounds should stay a 60x60 square.
        CHECK(sd.getGlobalBounds().size == Approx(za::Vec2f{60.f, 60.f}));
    }

    SECTION("Invalid inner radius >= outer radius yields an empty bounding box")
    {
        const za::RingShapeData sd{.outerRadius = 10.f, .innerRadius = 20.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {}));
    }

    SECTION("Non-positive outer radius yields an empty bounding box")
    {
        const za::RingShapeData sd{.outerRadius = 0.f, .innerRadius = 0.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {}));
    }

    SECTION("Centroid: coincides with ring center")
    {
        const za::RingShapeData sd{.outerRadius = 30.f, .innerRadius = 10.f};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{30.f, 30.f}));
    }
}

TEST_CASE("[Graphics] za::RectangleShapeData")
{
    SECTION("Local bounds")
    {
        const za::RectangleShapeData sd{.size = {40.f, 20.f}};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {40.f, 20.f}}));
    }

    SECTION("Global bounds with translation")
    {
        const za::RectangleShapeData sd{.position = {5.f, 7.f}, .size = {40.f, 20.f}};
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {{5.f, 7.f}, {40.f, 20.f}}));
    }

    SECTION("Global bounds: 90 deg rotation about center swaps dimensions exactly")
    {
        const za::RectangleShapeData sd{.origin = {20.f, 10.f}, .rotation = za::degrees(90.f), .size = {40.f, 20.f}};
        CHECK(sd.getGlobalBounds().size == Approx(za::Vec2f{20.f, 40.f}));
    }

    SECTION("Global bounds: 45 deg rotated rectangle is tight")
    {
        const za::RectangleShapeData sd{.origin = {10.f, 10.f}, .rotation = za::degrees(45.f), .size = {20.f, 20.f}};
        // A 20x20 square rotated 45 deg has an AABB of 20*sqrt(2) ~ 28.2843 on each side.
        const float expected = 20.f * 1.41421356f;
        CHECK(sd.getGlobalBounds().size.x == Approx(expected));
        CHECK(sd.getGlobalBounds().size.y == Approx(expected));
    }

    SECTION("Scale doubles both extents")
    {
        const za::RectangleShapeData sd{.scale = {2.f, 3.f}, .size = {10.f, 10.f}};
        CHECK(sd.getGlobalBounds().size == Approx(za::Vec2f{20.f, 30.f}));
    }

    SECTION("Centroid: coincides with rectangle center")
    {
        const za::RectangleShapeData sd{.size = {40.f, 20.f}};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{20.f, 10.f}));
    }
}

TEST_CASE("[Graphics] za::RoundedRectangleShapeData")
{
    SECTION("Local bounds")
    {
        const za::RoundedRectangleShapeData sd{.size = {40.f, 20.f}, .cornerRadius = 5.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {40.f, 20.f}}));
    }

    SECTION("Global bounds with identity transform")
    {
        const za::RoundedRectangleShapeData sd{.size = {40.f, 20.f}, .cornerRadius = 5.f, .cornerPointCount = 32u};
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), sd.getLocalBounds()));
    }

    SECTION("Centroid: coincides with rectangle center")
    {
        const za::RoundedRectangleShapeData sd{.size = {40.f, 20.f}, .cornerRadius = 5.f};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{20.f, 10.f}));
    }
}

TEST_CASE("[Graphics] za::StarShapeData")
{
    SECTION("Local bounds are 2 * outerRadius square")
    {
        const za::StarShapeData sd{.outerRadius = 30.f, .innerRadius = 15.f, .pointCount = 5u};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {60.f, 60.f}}));
    }

    SECTION("Global bounds never exceed the outer disk bbox")
    {
        const za::StarShapeData sd{.origin      = {30.f, 30.f},
                                   .rotation    = za::degrees(17.f),
                                   .outerRadius = 30.f,
                                   .innerRadius = 15.f,
                                   .pointCount  = 5u};
        const auto              gb = sd.getGlobalBounds();
        // Tight global bounds cannot exceed the disk enclosing the star tips.
        CHECK(gb.size.x <= 60.f + 1e-3f);
        CHECK(gb.size.y <= 60.f + 1e-3f);
    }

    SECTION("Centroid: coincides with star center")
    {
        const za::StarShapeData sd{.outerRadius = 30.f, .innerRadius = 15.f, .pointCount = 5u};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{30.f, 30.f}));
    }
}

TEST_CASE("[Graphics] za::CogShapeData")
{
    SECTION("Local bounds are 2 * outerRadius square")
    {
        const za::CogShapeData sd{.outerRadius = 40.f, .innerRadius = 20.f, .toothCount = 8u};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {80.f, 80.f}}));
    }

    SECTION("Global bounds never exceed the outer disk bbox under rotation")
    {
        const za::CogShapeData sd{.origin          = {40.f, 40.f},
                                  .rotation        = za::degrees(23.f),
                                  .outerRadius     = 40.f,
                                  .innerRadius     = 20.f,
                                  .toothCount      = 12u,
                                  .toothWidthRatio = 0.5f};
        const auto             gb = sd.getGlobalBounds();
        CHECK(gb.size.x <= 80.f + 1e-3f);
        CHECK(gb.size.y <= 80.f + 1e-3f);
    }

    SECTION("Centroid: coincides with cog center")
    {
        const za::CogShapeData sd{.outerRadius = 40.f, .innerRadius = 20.f, .toothCount = 8u};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{40.f, 40.f}));
    }
}

TEST_CASE("[Graphics] za::PieSliceShapeData")
{
    SECTION("Degenerate sweep yields a point at the hub")
    {
        const za::PieSliceShapeData sd{.radius = 10.f, .startAngle = za::degrees(0.f), .sweepAngle = za::degrees(0.f)};
        // Nothing is drawn, so bounds are empty.
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {}));
    }

    SECTION("Full circle sweep produces full-disk local bounds")
    {
        const za::PieSliceShapeData sd{.radius = 10.f, .startAngle = za::degrees(0.f), .sweepAngle = za::radians(za::tau)};
        // With sweep == tau, every cardinal is in range -- bbox fills the enclosing square.
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {20.f, 20.f}}));
    }

    SECTION("Narrow sweep does not reach the far side of the disk")
    {
        const za::PieSliceShapeData sd{.radius = 10.f, .startAngle = za::degrees(0.f), .sweepAngle = za::degrees(10.f)};
        // A thin wedge starting at angle 0 cannot span both axes.
        const auto lb = sd.getLocalBounds();
        CHECK(lb.size.x <= 20.f);
        CHECK(lb.size.y <= 20.f);
        CHECK((lb.size.x < 20.f || lb.size.y < 20.f));
    }

    SECTION("Centroid: full-circle sweep collapses to the hub")
    {
        // With d = (4*R*sin(pi)) / (3*tau) = 0, the centroid sits exactly on the hub.
        const za::PieSliceShapeData sd{.radius = 10.f, .sweepAngle = za::radians(za::tau)};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{10.f, 10.f}));
    }

    SECTION("Centroid: degenerate sweep falls back to the hub")
    {
        const za::PieSliceShapeData sd{.radius = 10.f, .sweepAngle = za::degrees(0.f)};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{10.f, 10.f}));
    }

    SECTION("Centroid: agrees with polygon shoelace reference on a 90 deg wedge")
    {
        const za::PieSliceShapeData sd{.radius     = 10.f,
                                       .startAngle = za::degrees(0.f),
                                       .sweepAngle = za::degrees(90.f),
                                       .pointCount = 256u};
        // Numerical reference on the tessellated polygon (dense enough to closely
        // approximate the true sector).
        const auto reference = referencePolygonCentroid(sd.pointCount,
                                                        [&](const unsigned int i) noexcept
        {
            if (i == 0u)
                return za::Vec2f{sd.radius, sd.radius};

            const float step  = sd.sweepAngle.asRadians() / static_cast<float>(sd.pointCount - 2u);
            const float angle = sd.startAngle.asRadians() + static_cast<float>(i - 1u) * step;
            const auto  sc    = za::sinCosLookup(za::positiveRemainder(angle, za::tau));
            return za::Vec2f{sd.radius - sd.radius * sc.sin, sd.radius + sd.radius * sc.cos};
        });

        // Tolerance reflects the finite tessellation of the reference polygon.
        CHECK(za::fabs(sd.getCentroid().x - reference.x) < 0.05f);
        CHECK(za::fabs(sd.getCentroid().y - reference.y) < 0.05f);
    }

    SECTION("Negative sweep is equivalent to positive sweep with shifted start")
    {
        // `startAngle + sweep` ending at the same angle in both cases should produce
        // identical bounds and centroid (the shape is the same, traversed in reverse).
        const za::PieSliceShapeData forward{.radius     = 10.f,
                                            .startAngle = za::degrees(30.f),
                                            .sweepAngle = za::degrees(120.f),
                                            .pointCount = 64u};

        const za::PieSliceShapeData backward{.radius     = 10.f,
                                             .startAngle = za::degrees(150.f),
                                             .sweepAngle = za::degrees(-120.f),
                                             .pointCount = 64u};

        CHECK(backward.hasVisibleGeometry());
        CHECK(rectsApproxEqual(forward.getLocalBounds(), backward.getLocalBounds()));
        CHECK(forward.getCentroid() == Approx(backward.getCentroid()));
    }
}

TEST_CASE("[Graphics] za::RingPieSliceShapeData")
{
    SECTION("Full sweep produces full outer-ring bbox")
    {
        const za::RingPieSliceShapeData sd{.outerRadius = 30.f,
                                           .innerRadius = 15.f,
                                           .startAngle  = za::degrees(0.f),
                                           .sweepAngle  = za::radians(za::tau)};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {60.f, 60.f}}));
    }

    SECTION("Degenerate sweep yields an empty bounding box (nothing drawn)")
    {
        const za::RingPieSliceShapeData sd{.outerRadius = 30.f, .innerRadius = 15.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {}));
    }

    SECTION("Invalid inner radius >= outer radius yields an empty bounding box")
    {
        // `add(...)` early-returns for this case; bounds should agree.
        const za::RingPieSliceShapeData sd{.outerRadius = 10.f,
                                           .innerRadius = 20.f,
                                           .startAngle  = za::degrees(0.f),
                                           .sweepAngle  = za::degrees(90.f)};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {}));
    }

    SECTION("Non-positive outer radius yields an empty bounding box")
    {
        const za::RingPieSliceShapeData sd{.outerRadius = 0.f, .innerRadius = 0.f, .sweepAngle = za::degrees(90.f)};
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {}));
    }

    SECTION("Centroid: full-circle sweep collapses to the ring center")
    {
        const za::RingPieSliceShapeData sd{.outerRadius = 30.f, .innerRadius = 15.f, .sweepAngle = za::radians(za::tau)};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{30.f, 30.f}));
    }

    SECTION("Centroid: degenerate sweep falls back to the ring center")
    {
        const za::RingPieSliceShapeData sd{.outerRadius = 30.f, .innerRadius = 15.f};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{30.f, 30.f}));
    }

    SECTION("Centroid: agrees with polygon shoelace reference on a 90 deg ring wedge")
    {
        const za::RingPieSliceShapeData sd{.outerRadius = 30.f,
                                           .innerRadius = 15.f,
                                           .startAngle  = za::degrees(0.f),
                                           .sweepAngle  = za::degrees(90.f),
                                           .pointCount  = 256u};

        // Simple closed polygon for the sector: outer arc forward, then inner arc reversed.
        const auto reference = referencePolygonCentroid(2u * sd.pointCount,
                                                        [&](const unsigned int i) noexcept
        {
            const float        step      = sd.sweepAngle.asRadians() / static_cast<float>(sd.pointCount - 1u);
            const bool         outerSide = (i < sd.pointCount);
            const unsigned int local     = outerSide ? i : (sd.pointCount - 1u - (i - sd.pointCount));
            const float        radius    = outerSide ? sd.outerRadius : sd.innerRadius;
            const float        angle     = sd.startAngle.asRadians() + static_cast<float>(local) * step;
            const auto         sc        = za::sinCosLookup(za::positiveRemainder(angle, za::tau));
            return za::Vec2f{sd.outerRadius + radius * sc.cos, sd.outerRadius + radius * sc.sin};
        });

        CHECK(za::fabs(sd.getCentroid().x - reference.x) < 0.1f);
        CHECK(za::fabs(sd.getCentroid().y - reference.y) < 0.1f);
    }

    SECTION("Negative sweep is equivalent to positive sweep with shifted start")
    {
        const za::RingPieSliceShapeData forward{.outerRadius = 30.f,
                                                .innerRadius = 15.f,
                                                .startAngle  = za::degrees(30.f),
                                                .sweepAngle  = za::degrees(120.f),
                                                .pointCount  = 64u};

        const za::RingPieSliceShapeData backward{.outerRadius = 30.f,
                                                 .innerRadius = 15.f,
                                                 .startAngle  = za::degrees(150.f),
                                                 .sweepAngle  = za::degrees(-120.f),
                                                 .pointCount  = 64u};

        CHECK(backward.hasVisibleGeometry());
        CHECK(rectsApproxEqual(forward.getLocalBounds(), backward.getLocalBounds()));
        CHECK(forward.getCentroid() == Approx(backward.getCentroid()));
    }
}

TEST_CASE("[Graphics] za::CurvedArrowShapeData")
{
    SECTION("Zero-sweep arrow yields an empty bounding box (nothing drawn)")
    {
        const za::CurvedArrowShapeData
            sd{.outerRadius = 20.f, .innerRadius = 10.f, .sweepAngle = za::degrees(0.f), .headLength = 5.f, .headWidth = 8.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {}));
    }

    SECTION("Invalid inner >= outer radius yields an empty bounding box")
    {
        const za::CurvedArrowShapeData sd{.outerRadius = 10.f,
                                          .innerRadius = 20.f,
                                          .sweepAngle  = za::degrees(90.f),
                                          .headLength  = 5.f,
                                          .headWidth   = 8.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {}));
    }

    SECTION("Local bounds respect sweep")
    {
        const za::CurvedArrowShapeData narrow{.outerRadius = 30.f,
                                              .innerRadius = 15.f,
                                              .startAngle  = za::degrees(0.f),
                                              .sweepAngle  = za::degrees(30.f),
                                              .headLength  = 5.f,
                                              .headWidth   = 8.f};
        const za::CurvedArrowShapeData wide{.outerRadius = 30.f,
                                            .innerRadius = 15.f,
                                            .startAngle  = za::degrees(0.f),
                                            .sweepAngle  = za::degrees(180.f),
                                            .headLength  = 5.f,
                                            .headWidth   = 8.f};
        // Wider arc should have at least as much extent in some axis as a narrower one.
        CHECK(wide.getLocalBounds().size.x + wide.getLocalBounds().size.y >=
              narrow.getLocalBounds().size.x + narrow.getLocalBounds().size.y);
    }

    SECTION("Centroid: degenerate shape falls back to the ring center")
    {
        const za::CurvedArrowShapeData
            sd{.outerRadius = 20.f, .innerRadius = 10.f, .sweepAngle = za::degrees(0.f), .headLength = 5.f, .headWidth = 8.f};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{20.f, 20.f}));
    }

    SECTION("Centroid: lies inside the shape's bounding box")
    {
        const za::CurvedArrowShapeData sd{.outerRadius = 30.f,
                                          .innerRadius = 15.f,
                                          .startAngle  = za::degrees(0.f),
                                          .sweepAngle  = za::degrees(120.f),
                                          .headLength  = 12.f,
                                          .headWidth   = 18.f};
        const auto                     c  = sd.getCentroid();
        const auto                     lb = sd.getLocalBounds();
        CHECK(c.x >= lb.position.x - 1e-3f);
        CHECK(c.y >= lb.position.y - 1e-3f);
        CHECK(c.x <= lb.position.x + lb.size.x + 1e-3f);
        CHECK(c.y <= lb.position.y + lb.size.y + 1e-3f);
    }
}

TEST_CASE("[Graphics] za::ArrowShapeData")
{
    SECTION("Local bounds track max(shaftWidth, headWidth)")
    {
        const za::ArrowShapeData sd{.shaftLength = 50.f, .shaftWidth = 10.f, .headLength = 20.f, .headWidth = 30.f};
        // Local bbox: x in [0, shaftLength + headLength], y centered on the shaft axis.
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, -15.f}, {70.f, 30.f}}));
    }

    SECTION("Global bounds: 90 deg rotation swaps extents")
    {
        const za::ArrowShapeData sd{.origin      = {35.f, 0.f},
                                    .rotation    = za::degrees(90.f),
                                    .shaftLength = 50.f,
                                    .shaftWidth  = 10.f,
                                    .headLength  = 20.f,
                                    .headWidth   = 30.f};
        const auto               gb = sd.getGlobalBounds();
        // Width and height swap under 90 deg rotation.
        CHECK(gb.size == Approx(za::Vec2f{30.f, 70.f}));
    }

    SECTION("Global bounds tighter than transformRect of local bounds at 45 deg")
    {
        // Demonstrates the reason we iterate actual vertices instead of transforming the bbox rect.
        const za::ArrowShapeData sd{.rotation    = za::degrees(45.f),
                                    .shaftLength = 50.f,
                                    .shaftWidth  = 10.f,
                                    .headLength  = 20.f,
                                    .headWidth   = 30.f};
        const auto               tight = sd.getGlobalBounds();
        // The naive transformRect AABB of a 70x30 local box rotated 45 deg would be 70.71 wide.
        // The tight bbox must be strictly smaller because the arrow does not fill its bbox corners.
        const float naiveAabbSide = (70.f + 30.f) * 0.70710678f;
        CHECK(tight.size.x < naiveAabbSide);
        CHECK(tight.size.y < naiveAabbSide);
    }

    SECTION("Centroid: Cy is always 0 (symmetric about the shaft axis)")
    {
        const za::ArrowShapeData sd{.shaftLength = 50.f, .shaftWidth = 10.f, .headLength = 20.f, .headWidth = 30.f};
        CHECK(sd.getCentroid().y == Approx(0.f));
    }

    SECTION("Centroid: shaft-only arrow sits at the shaft midpoint")
    {
        const za::ArrowShapeData sd{.shaftLength = 50.f, .shaftWidth = 10.f, .headLength = 0.f, .headWidth = 0.f};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{25.f, 0.f}));
    }

    SECTION("Centroid: head-only arrow sits at the triangle centroid (1/3 in)")
    {
        const za::ArrowShapeData sd{.shaftLength = 0.f, .shaftWidth = 0.f, .headLength = 30.f, .headWidth = 20.f};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{10.f, 0.f}));
    }

    SECTION("Centroid: agrees with the 7-vertex polygon shoelace")
    {
        const za::ArrowShapeData sd{.shaftLength = 50.f, .shaftWidth = 10.f, .headLength = 20.f, .headWidth = 30.f};

        const auto reference = referencePolygonCentroid(7u, [&](const unsigned int i) noexcept {
            return za::ShapeUtils::computeArrowPoint(i, sd.shaftLength, sd.shaftWidth, sd.headLength, sd.headWidth);
        });

        CHECK(sd.getCentroid() == Approx(reference));
    }
}

TEST_CASE("[Graphics] za::CrossShapeData")
{
    SECTION("Local bounds match size")
    {
        const za::CrossShapeData sd{.size = {40.f, 30.f}, .armThickness = 10.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {40.f, 30.f}}));
    }

    SECTION("Global bounds: 45 deg rotation strictly smaller than transformRect")
    {
        const za::CrossShapeData sd{.origin       = {20.f, 20.f},
                                    .rotation     = za::degrees(45.f),
                                    .size         = {40.f, 40.f},
                                    .armThickness = 10.f};
        const auto               tight         = sd.getGlobalBounds();
        const float              naiveAabbSide = 40.f * 1.41421356f;
        CHECK(tight.size.x < naiveAabbSide);
    }

    SECTION("Centroid: coincides with cross center (symmetric)")
    {
        const za::CrossShapeData sd{.size = {40.f, 30.f}, .armThickness = 10.f};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{20.f, 15.f}));
    }

    SECTION("Centroid: agrees with the 12-vertex polygon shoelace")
    {
        const za::CrossShapeData sd{.size = {40.f, 30.f}, .armThickness = 10.f};
        const auto               reference = referencePolygonCentroid(12u, [&](const unsigned int i) noexcept {
            return za::ShapeUtils::computeCrossPoint(i, sd.size, sd.armThickness);
        });
        CHECK(sd.getCentroid() == Approx(reference));
    }
}

TEST_CASE("[Graphics] za::ChevronShapeData")
{
    SECTION("Local bounds match size")
    {
        const za::ChevronShapeData sd{.size = {40.f, 30.f}, .thickness = 8.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {40.f, 30.f}}));
    }

    SECTION("Degenerate size.y yields an empty bounding box (nothing drawn)")
    {
        const za::ChevronShapeData sd{.position = {5.f, 5.f}, .size = {40.f, 0.f}, .thickness = 0.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {}));
    }

    SECTION("Thickness beyond size.y/2 cleanly falls back to a filled triangle")
    {
        // At exactly `size.y/2`, the inner tip collapses to `(0, size.y/2)`; beyond that,
        // the shape clamps to the same degenerate triangle rather than self-intersecting.
        const za::ChevronShapeData full{.size = {40.f, 30.f}, .thickness = 15.f};
        const za::ChevronShapeData over{.size = {40.f, 30.f}, .thickness = 1000.f};

        // Both should produce the outer-triangle AABB, which spans the full local size.
        CHECK(rectsApproxEqual(full.getLocalBounds(), {{0.f, 0.f}, {40.f, 30.f}}));
        CHECK(rectsApproxEqual(over.getLocalBounds(), {{0.f, 0.f}, {40.f, 30.f}}));

        // Tight global bounds should be identical for any thickness >= size.y/2.
        CHECK(rectsApproxEqual(full.getGlobalBounds(), over.getGlobalBounds()));
    }

    SECTION("Centroid: Cy is always size.y / 2 (symmetric about the horizontal axis)")
    {
        const za::ChevronShapeData sd{.size = {40.f, 30.f}, .thickness = 8.f};
        CHECK(sd.getCentroid().y == Approx(15.f));
    }

    SECTION("Centroid: thickness exactly size.y/2 degenerates to a triangle centroid")
    {
        const za::ChevronShapeData sd{.size = {40.f, 30.f}, .thickness = 15.f};
        // Triangle (0,0), (40,15), (0,30) has centroid ((0+40+0)/3, (0+15+30)/3) = (40/3, 15).
        CHECK(sd.getCentroid() == Approx(za::Vec2f{40.f / 3.f, 15.f}));
    }

    SECTION("Centroid: agrees with the 6-vertex polygon shoelace for a thin chevron")
    {
        const za::ChevronShapeData sd{.size = {40.f, 30.f}, .thickness = 8.f};
        const auto                 reference = referencePolygonCentroid(6u, [&](const unsigned int i) noexcept {
            return za::ShapeUtils::computeChevronPoint(i, sd.size, sd.thickness);
        });
        CHECK(sd.getCentroid() == Approx(reference));
    }
}

TEST_CASE("[Graphics] za::TrapezoidShapeData")
{
    SECTION("Local bounds use max(topWidth, bottomWidth)")
    {
        const za::TrapezoidShapeData sd{.topWidth = 20.f, .bottomWidth = 40.f, .height = 30.f};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {40.f, 30.f}}));
    }

    SECTION("Rectangular special case: equal widths")
    {
        const za::TrapezoidShapeData sd{.topWidth = 20.f, .bottomWidth = 20.f, .height = 10.f};
        // Degenerates into a rectangle; global bounds should match local since trapezoid corners are on the bbox.
        CHECK(rectsApproxEqual(sd.getGlobalBounds(), {{0.f, 0.f}, {20.f, 10.f}}));
    }

    SECTION("Centroid: rectangular case sits at the rectangle center")
    {
        const za::TrapezoidShapeData sd{.topWidth = 20.f, .bottomWidth = 20.f, .height = 10.f};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{10.f, 5.f}));
    }

    SECTION("Centroid: agrees with the classic (a + 2b)/(3(a+b)) formula")
    {
        const za::TrapezoidShapeData sd{.topWidth = 20.f, .bottomWidth = 40.f, .height = 30.f};
        // Cx at bbox center (max width = 40), Cy = 30 * (20 + 80) / (3 * 60) = 30 * 100 / 180 = 16.666...
        CHECK(sd.getCentroid() == Approx(za::Vec2f{20.f, 30.f * 100.f / 180.f}));
    }

    SECTION("Centroid: agrees with the 4-vertex polygon shoelace")
    {
        const za::TrapezoidShapeData sd{.topWidth = 20.f, .bottomWidth = 40.f, .height = 30.f};
        const auto                   reference = referencePolygonCentroid(4u, [&](const unsigned int i) noexcept {
            return za::ShapeUtils::computeTrapezoidPoint(i, sd.topWidth, sd.bottomWidth, sd.height);
        });
        CHECK(sd.getCentroid() == Approx(reference));
    }
}

TEST_CASE("[Graphics] za::HeartShapeData")
{
    SECTION("Local bounds match size")
    {
        const za::HeartShapeData sd{.size = {40.f, 30.f}, .pointCount = 64u};
        CHECK(rectsApproxEqual(sd.getLocalBounds(), {{0.f, 0.f}, {40.f, 30.f}}));
    }

    SECTION("Global bounds fit within local bounds at identity transform")
    {
        const za::HeartShapeData sd{.size = {40.f, 30.f}, .pointCount = 128u};
        const auto               gb = sd.getGlobalBounds();
        // With denser tessellation, global bounds should closely match local bounds.
        CHECK(gb.position.x >= -0.01f);
        CHECK(gb.position.y >= -0.01f);
        CHECK(gb.position.x + gb.size.x <= 40.01f);
        CHECK(gb.position.y + gb.size.y <= 30.01f);
    }

    SECTION("Centroid: Cx lies on the axis of symmetry")
    {
        const za::HeartShapeData sd{.size = {40.f, 30.f}, .pointCount = 64u};
        CHECK(sd.getCentroid().x == Approx(20.f));
    }

    SECTION("Centroid: lies inside the heart's bounding box")
    {
        const za::HeartShapeData sd{.size = {40.f, 30.f}, .pointCount = 64u};
        const auto               c = sd.getCentroid();
        CHECK(c.x >= 0.f);
        CHECK(c.y >= 0.f);
        CHECK(c.x <= 40.f);
        CHECK(c.y <= 30.f);
    }

    SECTION("Centroid: degenerate pointCount falls back to bbox center")
    {
        const za::HeartShapeData sd{.size = {40.f, 30.f}, .pointCount = 3u};
        CHECK(sd.getCentroid() == Approx(za::Vec2f{20.f, 15.f}));
    }
}

TEST_CASE("[Graphics] Centroid membership: convex shapes contain their centroid")
{
    // A centroid is not guaranteed to lie inside a non-convex polygon in general, but for the
    // convex / centrally-symmetric shapes in the library it should.
    SECTION("Circle")
    {
        const za::CircleShapeData sd{.radius = 10.f};
        const auto                c  = sd.getCentroid();
        const auto                lb = sd.getLocalBounds();
        CHECK(lb.contains(c));
    }

    SECTION("Ellipse")
    {
        const za::EllipseShapeData sd{.horizontalRadius = 20.f, .verticalRadius = 10.f};
        CHECK(sd.getLocalBounds().contains(sd.getCentroid()));
    }

    SECTION("Rectangle")
    {
        const za::RectangleShapeData sd{.size = {40.f, 20.f}};
        CHECK(sd.getLocalBounds().contains(sd.getCentroid()));
    }

    SECTION("Trapezoid")
    {
        const za::TrapezoidShapeData sd{.topWidth = 10.f, .bottomWidth = 40.f, .height = 30.f};
        CHECK(sd.getLocalBounds().contains(sd.getCentroid()));
    }

    SECTION("Arrow")
    {
        const za::ArrowShapeData sd{.shaftLength = 50.f, .shaftWidth = 10.f, .headLength = 20.f, .headWidth = 30.f};
        CHECK(sd.getLocalBounds().contains(sd.getCentroid()));
    }
}

TEST_CASE("[Graphics] Anchor-point mixins")
{
    SECTION("Local helpers read from getLocalBounds")
    {
        const za::RectangleShapeData sd{.size = {40.f, 20.f}};
        CHECK(sd.getLocalCenter() == Approx(za::Vec2f{20.f, 10.f}));
        CHECK(sd.getLocalTopLeft() == Approx(za::Vec2f{0.f, 0.f}));
        CHECK(sd.getLocalBottomRight() == Approx(za::Vec2f{40.f, 20.f}));
        CHECK(sd.getLocalWidth() == Approx(40.f));
        CHECK(sd.getLocalHeight() == Approx(20.f));
    }

    SECTION("Global helpers read from getGlobalBounds")
    {
        const za::RectangleShapeData sd{.position = {5.f, 5.f}, .size = {40.f, 20.f}};
        CHECK(sd.getGlobalTopLeft() == Approx(za::Vec2f{5.f, 5.f}));
        CHECK(sd.getGlobalCenter() == Approx(za::Vec2f{25.f, 15.f}));
        CHECK(sd.getGlobalBottomRight() == Approx(za::Vec2f{45.f, 25.f}));
    }

    SECTION("setGlobalCenter repositions the shape")
    {
        za::RectangleShapeData sd{.size = {40.f, 20.f}};
        sd.setGlobalCenter({100.f, 50.f});
        CHECK(sd.getGlobalCenter() == Approx(za::Vec2f{100.f, 50.f}));
    }
}

TEST_CASE("[Graphics] Global bounds invariance under rotation about the geometric center")
{
    // For any shape whose origin is placed at its geometric (local) center and whose local
    // bounds are square, rotating by 90 deg produces the same AABB size. This is a direct
    // sanity check that our bounds formulas handle rotation correctly.
    SECTION("Circle")
    {
        const za::CircleShapeData rot0{.origin = {10.f, 10.f}, .radius = 10.f};
        const za::CircleShapeData rot90{.origin = {10.f, 10.f}, .rotation = za::degrees(90.f), .radius = 10.f};
        CHECK(rot0.getGlobalBounds().size == Approx(rot90.getGlobalBounds().size));
    }

    SECTION("Ring")
    {
        const za::RingShapeData rot0{.origin = {20.f, 20.f}, .outerRadius = 20.f, .innerRadius = 10.f};
        const za::RingShapeData rot90{.origin      = {20.f, 20.f},
                                      .rotation    = za::degrees(90.f),
                                      .outerRadius = 20.f,
                                      .innerRadius = 10.f};
        CHECK(rot0.getGlobalBounds().size == Approx(rot90.getGlobalBounds().size));
    }

    SECTION("Square Rectangle")
    {
        const za::RectangleShapeData rot0{.origin = {10.f, 10.f}, .size = {20.f, 20.f}};
        const za::RectangleShapeData rot90{.origin = {10.f, 10.f}, .rotation = za::degrees(90.f), .size = {20.f, 20.f}};
        CHECK(rot0.getGlobalBounds().size == Approx(rot90.getGlobalBounds().size));
    }
}

TEST_CASE("[Graphics] hasVisibleGeometry")
{
    // Default-constructed shape data should not report anything drawable: every type's
    // "zero state" (radius = 0, size = {0, 0}, etc.) leaves the draw path with no geometry.
    SECTION("Default-constructed shapes have no visible geometry")
    {
        CHECK_FALSE(za::ArrowShapeData{.shaftLength = 0.f, .shaftWidth = 0.f, .headLength = 0.f, .headWidth = 0.f}
                        .hasVisibleGeometry());
        CHECK_FALSE(za::ChevronShapeData{}.hasVisibleGeometry());
        CHECK_FALSE(za::CircleShapeData{}.hasVisibleGeometry());
        CHECK_FALSE(za::CogShapeData{}.hasVisibleGeometry());
        CHECK_FALSE(za::CrossShapeData{}.hasVisibleGeometry());
        CHECK_FALSE(za::CurvedArrowShapeData{.outerRadius = 0.f, .innerRadius = 0.f}.hasVisibleGeometry());
        CHECK_FALSE(za::EllipseShapeData{}.hasVisibleGeometry());
        CHECK_FALSE(za::HeartShapeData{}.hasVisibleGeometry());
        CHECK_FALSE(za::PieSliceShapeData{}.hasVisibleGeometry());
        CHECK_FALSE(za::RectangleShapeData{}.hasVisibleGeometry());
        CHECK_FALSE(za::RingPieSliceShapeData{}.hasVisibleGeometry());
        CHECK_FALSE(za::RingShapeData{}.hasVisibleGeometry());
        CHECK_FALSE(za::RoundedRectangleShapeData{}.hasVisibleGeometry());
        CHECK_FALSE(za::StarShapeData{}.hasVisibleGeometry());
        CHECK_FALSE(za::TrapezoidShapeData{}.hasVisibleGeometry());
    }

    // Reasonable "minimally valid" configurations should be visible.
    SECTION("Minimally valid configurations are visible")
    {
        CHECK(za::ArrowShapeData{}.hasVisibleGeometry()); // defaults are all positive
        CHECK(za::ChevronShapeData{.size = {40.f, 30.f}, .thickness = 5.f}.hasVisibleGeometry());
        CHECK(za::CircleShapeData{.radius = 10.f}.hasVisibleGeometry());
        CHECK(za::CogShapeData{.outerRadius = 20.f, .innerRadius = 10.f}.hasVisibleGeometry());
        CHECK(za::CrossShapeData{.size = {40.f, 30.f}, .armThickness = 10.f}.hasVisibleGeometry());
        CHECK(za::CurvedArrowShapeData{.outerRadius = 30.f, .innerRadius = 15.f}.hasVisibleGeometry());
        CHECK(za::EllipseShapeData{.horizontalRadius = 20.f, .verticalRadius = 10.f}.hasVisibleGeometry());
        CHECK(za::HeartShapeData{.size = {40.f, 30.f}}.hasVisibleGeometry());
        CHECK(za::PieSliceShapeData{.radius = 10.f, .sweepAngle = za::degrees(90.f)}.hasVisibleGeometry());
        CHECK(za::RectangleShapeData{.size = {40.f, 20.f}}.hasVisibleGeometry());
        CHECK(za::RingPieSliceShapeData{.outerRadius = 30.f, .innerRadius = 15.f, .sweepAngle = za::degrees(90.f)}
                  .hasVisibleGeometry());
        CHECK(za::RingShapeData{.outerRadius = 30.f, .innerRadius = 15.f}.hasVisibleGeometry());
        CHECK(za::RoundedRectangleShapeData{.size = {40.f, 30.f}, .cornerRadius = 5.f}.hasVisibleGeometry());
        CHECK(za::StarShapeData{.outerRadius = 30.f, .innerRadius = 15.f}.hasVisibleGeometry());
        CHECK(za::TrapezoidShapeData{.topWidth = 20.f, .bottomWidth = 40.f, .height = 30.f}.hasVisibleGeometry());
    }

    // Per-shape boundary conditions: each knob that can falsify the predicate.
    SECTION("ArrowShapeData visibility boundaries")
    {
        // Zero total length -> invisible.
        CHECK_FALSE(za::ArrowShapeData{.shaftLength = 0.f, .headLength = 0.f}.hasVisibleGeometry());
        // Zero thickness on both the shaft and the head -> invisible.
        CHECK_FALSE(za::ArrowShapeData{.shaftWidth = 0.f, .headWidth = 0.f}.hasVisibleGeometry());
        // Negative dimension -> invisible.
        CHECK_FALSE(za::ArrowShapeData{.shaftLength = -1.f}.hasVisibleGeometry());
    }

    SECTION("CircleShapeData visibility boundaries")
    {
        CHECK_FALSE(za::CircleShapeData{.radius = 0.f}.hasVisibleGeometry());
        CHECK_FALSE(za::CircleShapeData{.radius = 10.f, .pointCount = 2u}.hasVisibleGeometry());
    }

    SECTION("CogShapeData visibility boundaries")
    {
        CHECK_FALSE(za::CogShapeData{.outerRadius = 20.f, .innerRadius = 10.f, .toothCount = 2u}.hasVisibleGeometry());
        CHECK_FALSE(za::CogShapeData{.outerRadius = 20.f, .innerRadius = 10.f, .toothCount = 8u, .toothWidthRatio = 0.f}
                        .hasVisibleGeometry());
        CHECK_FALSE(za::CogShapeData{.outerRadius = 20.f, .innerRadius = 10.f, .toothCount = 8u, .toothWidthRatio = 1.f}
                        .hasVisibleGeometry());
    }

    SECTION("CrossShapeData visibility boundaries")
    {
        CHECK_FALSE(za::CrossShapeData{.size = {0.f, 30.f}}.hasVisibleGeometry());
        CHECK_FALSE(za::CrossShapeData{.size = {40.f, 0.f}}.hasVisibleGeometry());
        CHECK_FALSE(za::CrossShapeData{.size = {40.f, 30.f}, .armThickness = -1.f}.hasVisibleGeometry());
    }

    SECTION("EllipseShapeData visibility boundaries")
    {
        CHECK_FALSE(za::EllipseShapeData{.horizontalRadius = 0.f, .verticalRadius = 10.f}.hasVisibleGeometry());
        CHECK_FALSE(za::EllipseShapeData{.horizontalRadius = 10.f, .verticalRadius = 0.f}.hasVisibleGeometry());
        CHECK_FALSE(
            za::EllipseShapeData{.horizontalRadius = 10.f, .verticalRadius = 10.f, .pointCount = 2u}.hasVisibleGeometry());
    }

    SECTION("HeartShapeData visibility boundaries")
    {
        CHECK_FALSE(za::HeartShapeData{.size = {40.f, 30.f}, .pointCount = 2u}.hasVisibleGeometry());
        CHECK_FALSE(za::HeartShapeData{.size = {40.f, 30.f}, .pointCount = 3u}.hasVisibleGeometry());
        CHECK(za::HeartShapeData{.size = {40.f, 30.f}, .pointCount = 4u}.hasVisibleGeometry());
    }

    SECTION("RectangleShapeData visibility boundaries")
    {
        CHECK_FALSE(za::RectangleShapeData{.size = {0.f, 20.f}}.hasVisibleGeometry());
        CHECK_FALSE(za::RectangleShapeData{.size = {40.f, 0.f}}.hasVisibleGeometry());
    }

    SECTION("RoundedRectangleShapeData visibility boundaries")
    {
        CHECK_FALSE(za::RoundedRectangleShapeData{.size = {40.f, 30.f}, .cornerPointCount = 0u}.hasVisibleGeometry());
        CHECK_FALSE(za::RoundedRectangleShapeData{.size = {40.f, 30.f}, .cornerPointCount = 1u}.hasVisibleGeometry());
        CHECK_FALSE(za::RoundedRectangleShapeData{.size = {40.f, 30.f}, .cornerRadius = -1.f}.hasVisibleGeometry());
    }

    SECTION("StarShapeData visibility boundaries")
    {
        CHECK_FALSE(za::StarShapeData{.outerRadius = 30.f, .pointCount = 1u}.hasVisibleGeometry());
        CHECK_FALSE(za::StarShapeData{.outerRadius = 30.f, .innerRadius = -1.f}.hasVisibleGeometry());
    }

    SECTION("TrapezoidShapeData visibility boundaries")
    {
        // Zero height -> no area -> invisible.
        CHECK_FALSE(za::TrapezoidShapeData{.topWidth = 20.f, .bottomWidth = 40.f, .height = 0.f}.hasVisibleGeometry());
        // Zero on both widths -> no area -> invisible.
        CHECK_FALSE(za::TrapezoidShapeData{.topWidth = 0.f, .bottomWidth = 0.f, .height = 30.f}.hasVisibleGeometry());
        // One zero is fine (degenerates to a triangle).
        CHECK(za::TrapezoidShapeData{.topWidth = 0.f, .bottomWidth = 40.f, .height = 30.f}.hasVisibleGeometry());
    }

    // When `hasVisibleGeometry()` is false, the bounds should be empty.
    SECTION("Invisible shapes produce empty bounds")
    {
        const za::ArrowShapeData invalidArrow{.shaftLength = 0.f, .shaftWidth = 0.f, .headLength = 0.f, .headWidth = 0.f};
        const za::CircleShapeData           invalidCircle{.radius = 0.f};
        const za::CrossShapeData            invalidCross{.size = {0.f, 0.f}};
        const za::EllipseShapeData          invalidEllipse{.horizontalRadius = 0.f, .verticalRadius = 0.f};
        const za::HeartShapeData            invalidHeart{.size = {0.f, 0.f}};
        const za::RectangleShapeData        invalidRect{.size = {0.f, 0.f}};
        const za::RoundedRectangleShapeData invalidRoundedRect{.size = {0.f, 0.f}};
        const za::StarShapeData             invalidStar{.outerRadius = 0.f};
        const za::TrapezoidShapeData        invalidTrap{.topWidth = 0.f, .bottomWidth = 0.f, .height = 30.f};
        const za::CogShapeData              invalidCog{.outerRadius = 0.f};

        CHECK(rectsApproxEqual(invalidArrow.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(invalidArrow.getGlobalBounds(), {}));
        CHECK(rectsApproxEqual(invalidCircle.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(invalidCircle.getGlobalBounds(), {}));
        CHECK(rectsApproxEqual(invalidCross.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(invalidCross.getGlobalBounds(), {}));
        CHECK(rectsApproxEqual(invalidEllipse.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(invalidEllipse.getGlobalBounds(), {}));
        CHECK(rectsApproxEqual(invalidHeart.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(invalidHeart.getGlobalBounds(), {}));
        CHECK(rectsApproxEqual(invalidRect.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(invalidRect.getGlobalBounds(), {}));
        CHECK(rectsApproxEqual(invalidRoundedRect.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(invalidRoundedRect.getGlobalBounds(), {}));
        CHECK(rectsApproxEqual(invalidStar.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(invalidStar.getGlobalBounds(), {}));
        CHECK(rectsApproxEqual(invalidTrap.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(invalidTrap.getGlobalBounds(), {}));
        CHECK(rectsApproxEqual(invalidCog.getLocalBounds(), {}));
        CHECK(rectsApproxEqual(invalidCog.getGlobalBounds(), {}));
    }
}
