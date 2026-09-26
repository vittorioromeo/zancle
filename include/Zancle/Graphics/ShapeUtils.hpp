#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Vertex.hpp"

#include "Zancle/Geometry/Vec2.hpp"

#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/MinMaxMacros.hpp"
#include "Zancle/Math/SinCosLookup.hpp"
#include "Zancle/Math/Sqrt.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/AssertAndAssume.hpp"
#include "Zancle/Base/Restrict.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace za::ShapeUtils
{
////////////////////////////////////////////////////////////
/// \brief Computes a point on an ellipse using a precomputed angle step.
///
/// This function calculates the 2D position of a vertex on an ellipse
/// given its horizontal and vertical radii. It uses a precomputed angular
/// step multiplied by the vertex index.
///
/// \param index The index of the vertex.
/// \param angleStep The angular increment between vertices (in radians).
/// \param hRadius The horizontal radius of the ellipse.
/// \param vRadius The vertical radius of the ellipse.
///
/// \return The computed 2D position.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f computeEllipsePointFromAngleStep(
    const za::SizeT index,
    const float     startRadians,
    const float     angleStep,
    const float     hRadius,
    const float     vRadius) noexcept
{
    const auto [sine, cosine] = za::sinCosLookup(startRadians + static_cast<float>(index) * angleStep);

    ZA_ASSERT_AND_ASSUME(sine >= -1.f && sine <= 1.f);
    ZA_ASSERT_AND_ASSUME(cosine >= -1.f && cosine <= 1.f);

    // Winding is CW-visual in screen space (y-down) so that outline normals point inward:
    // `outlineThickness > 0` overlays the outline on top of the fill without growing the bounds.
    return {hRadius * (1.f - sine), vRadius * (1.f + cosine)};
}

////////////////////////////////////////////////////////////
/// \brief Computes a point on an ellipse.
///
/// Given the vertex index and total number of points defining the ellipse,
/// this function computes the vertex position by first calculating the angle step.
///
/// \param index The index of the vertex.
/// \param pointCount The total number of vertices.
/// \param hRadius The horizontal radius of the ellipse.
/// \param vRadius The vertical radius of the ellipse.
///
/// \return The computed 2D position.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f computeEllipsePoint(
    const za::SizeT    index,
    const float        startRadians,
    const unsigned int pointCount,
    const float        hRadius,
    const float        vRadius) noexcept
{
    return computeEllipsePointFromAngleStep(index, startRadians, za::tau / static_cast<float>(pointCount), hRadius, vRadius);
}

////////////////////////////////////////////////////////////
/// \brief Computes a point on a circle using a precomputed angle step.
///
/// This function calculates the position of a vertex on a circle, reusing the
/// ellipse computation by passing the same value for both radii.
///
/// \param index The index of the vertex.
/// \param angleStep The angular increment between vertices (in radians).
/// \param radius The radius of the circle.
///
/// \return The computed 2D position.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f computeCirclePointFromAngleStep(
    const za::SizeT index,
    const float     startRadians,
    const float     angleStep,
    const float     radius) noexcept
{
    return computeEllipsePointFromAngleStep(index, startRadians, angleStep, radius, radius);
}

////////////////////////////////////////////////////////////
/// \brief Computes a point on a circle.
///
/// Given the vertex index and total number of vertices for a circle,
/// this function computes the position of the vertex.
///
/// \param index The index of the vertex.
/// \param pointCount The total number of vertices.
/// \param radius The radius of the circle.
///
/// \return The computed 2D position.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f computeCirclePoint(
    const za::SizeT    index,
    const float        startRadians,
    const unsigned int pointCount,
    const float        radius) noexcept
{
    return computeCirclePointFromAngleStep(index, startRadians, za::tau / static_cast<float>(pointCount), radius);
}

////////////////////////////////////////////////////////////
/// \brief Computes a vertex point for a rectangle's boundary.
///
/// This function calculates the 2D position of one of the four
/// vertices defining a rectangle. The vertices are ordered as
/// follows: top-left (0), top-right (1), bottom-right (2),
/// and bottom-left (3).
///
/// \param index The index of the vertex to compute (0 to 3).
/// \param size The width and height of the rectangle.
///
/// \return The computed 2D position of the boundary vertex.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr Vec2f computeRectanglePoint(
    const za::SizeT index,
    const Vec2f     size) noexcept
{
    ZA_ASSERT_AND_ASSUME(index < 4u);

    const Vec2f points[]{{0.f, 0.f}, {size.x, 0.f}, {size.x, size.y}, {0.f, size.y}};
    return points[index];
}

////////////////////////////////////////////////////////////
/// \brief Computes a vertex point for a rounded rectangle's boundary.
///
/// This function calculates the 2D position of a vertex on the boundary
/// of a rounded rectangle. The vertices are generated by iterating
/// through each of the four corners, drawing an arc for each.
///
/// \param index The index of the vertex to compute. This iterates
///              sequentially through the points of each corner.
/// \param size The overall width and height of the rectangle.
/// \param cornerRadius The radius of each rounded corner.
/// \param cornerPointCount The number of vertices used to define each
///                         individual rounded corner.
///
/// \return The computed 2D position of the boundary vertex.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f computeRoundedRectanglePoint(
    const za::SizeT    index,
    const Vec2f        size,
    const float        cornerRadius,
    const unsigned int cornerPointCount) noexcept
{
    [[maybe_unused]] const auto totalNumPoints = cornerPointCount * 4u;
    ZA_ASSERT_AND_ASSUME(index < totalNumPoints);

    const za::SizeT centerIndex = index / cornerPointCount;
    ZA_ASSERT_AND_ASSUME(centerIndex <= 3u);

    const float deltaAngle = (za::halfPi) / static_cast<float>(cornerPointCount - 1u);

    // Corner order: TL (0), TR (1), BR (2), BL (3). This traces the perimeter CW-visually in screen
    // space (y-down) so that outline normals point inward -- keeps `outlineThickness > 0` on top of
    // the fill without growing the bounds.
    const Vec2f center{(centerIndex == 1u || centerIndex == 2u) ? size.x - cornerRadius : cornerRadius,
                       (centerIndex >= 2u) ? size.y - cornerRadius : cornerRadius};

    // Base angle per corner: TL starts at `pi` (west), TR at `3*pi/2` (north), BR at `0` (east),
    // BL at `pi/2` (south) -- measured in the (cos, sin) screen convention used below.
    const float baseAngle = za::pi + static_cast<float>(centerIndex) * za::halfPi;

    const za::SizeT localIndex = index - centerIndex * cornerPointCount;
    const auto [sine, cosine]  = za::sinCosLookup(baseAngle + deltaAngle * static_cast<float>(localIndex));
    return center + Vec2f{cornerRadius * cosine, cornerRadius * sine};
}

////////////////////////////////////////////////////////////
/// \brief Computes a vertex point for an arrow shape's boundary.
///
/// Defines an arrow pointing along the positive X-axis, with its
/// tail base centered at the local origin `(0,0)`. The arrow boundary
/// is defined by 7 distinct vertices in counter-clockwise order:
///
/// 0: Bottom-left tail corner
/// 1: Bottom-right shaft corner (at shaft end)
/// 2: Bottom barb corner (inner corner of head)
/// 3: Tip of the arrowhead
/// 4: Top barb corner (inner corner of head)
/// 5: Top-right shaft corner (at shaft end)
/// 6: Top-left tail corner
///
/// \param index       The index of the vertex to compute `(0 <= index < 7)`.
/// \param shaftLength Length of the arrow's shaft section.
/// \param shaftWidth  Full width of the arrow's shaft.
/// \param headLength  Length of the arrowhead section (from shaft end to tip).
/// \param headWidth   Full width of the arrowhead at its base (barbs).
///
/// \return The computed 2D position of the boundary vertex.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr Vec2f computeArrowPoint(
    const za::SizeT index,
    const float     shaftLength,
    const float     shaftWidth,
    const float     headLength,
    const float     headWidth) noexcept
{
    ZA_ASSERT_AND_ASSUME(index < 7u);
    ZA_ASSERT_AND_ASSUME(shaftLength >= 0.f);
    ZA_ASSERT_AND_ASSUME(shaftWidth >= 0.f);
    ZA_ASSERT_AND_ASSUME(headLength >= 0.f);
    ZA_ASSERT_AND_ASSUME(headWidth >= 0.f);

    const float halfShaftW = shaftWidth / 2.f;
    const float halfHeadW  = headWidth / 2.f;
    const float tipX       = shaftLength + headLength;

    const Vec2f points[7u] = {
        {0.f, -halfShaftW},         // 0: Bottom-left tail
        {shaftLength, -halfShaftW}, // 1: Bottom-right shaft
        {shaftLength, -halfHeadW},  // 2: Bottom barb
        {tipX, 0.f},                // 3: Tip
        {shaftLength, halfHeadW},   // 4: Top barb
        {shaftLength, halfShaftW},  // 5: Top-right shaft
        {0.f, halfShaftW}           // 6: Top-left tail
    };

    return points[index];
}

////////////////////////////////////////////////////////////
/// \brief Computes the angular step per point for a pie slice's arc.
///
/// This helper function calculates the constant angular increment
/// between successive points along the curved edge of a pie slice.
/// It's used to distribute the `pointCount` vertices (minus the center
/// and one endpoint, as the angle is between segments) evenly across
/// the `sweepAngle`.
///
/// \param sweepAngle The total angular extent of the pie slice's arc, in radians.
/// \param pointCount The total number of vertices defining the pie slice,
///                   including the center point and all points along the arc.
///                   Must be >= 3.
///
/// \return The computed angle step in radians between points on the arc.
///
/// \see computePieSlicePoint, computePieSlicePointFromArcAngleStep, computeArcAngleStep
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr float computePieSliceArcAngleStep(
    const float        sweepAngle,
    const unsigned int pointCount) noexcept
{
    ZA_ASSERT_AND_ASSUME(pointCount >= 3u);
    return sweepAngle / static_cast<float>(pointCount - 2u);
}

////////////////////////////////////////////////////////////
/// \brief Computes the angular step per point for a hub-less arc.
///
/// Companion to `computePieSliceArcAngleStep`, but for shapes whose
/// `pointCount` samples all lie on the arc (no separate hub vertex) --
/// `RingPieSliceShapeData`, `CurvedArrowShapeData`, etc. The
/// `pointCount` samples span `pointCount - 1` segments, so the step
/// is `sweepAngle / (pointCount - 1)`.
///
/// \param sweepAngle The total angular extent of the arc, in radians.
///                   May be negative (sweeps in the opposite direction).
/// \param pointCount The number of vertices defining the arc; must be >= 2.
///
/// \return The computed angle step in radians between successive arc samples.
///
/// \see computePieSliceArcAngleStep
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr float computeArcAngleStep(
    const float        sweepAngle,
    const unsigned int pointCount) noexcept
{
    ZA_ASSERT_AND_ASSUME(pointCount >= 2u);
    return sweepAngle / static_cast<float>(pointCount - 1u);
}

////////////////////////////////////////////////////////////
/// \see computePieSlicePoint
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f computePieSlicePointFromArcAngleStep(
    const za::SizeT index,
    const float     radius,
    const float     arcAngleStep,
    const float     startAngle) noexcept
{

    ZA_ASSERT_AND_ASSUME(radius >= 0.f); // Radius should be non-negative

    // Vertex `0` is the center of the arc/circle, acting as the hub for the triangle fan.

    if (index == 0u)
        return {radius, radius};

    // Vertices `1` to `pointCount - 1` are on the arc's curved edge.
    // There are `(pointCount - 1)` points on the arc edge.
    // These points span `(pointCount - 2)` segments along the arc.

    const auto arcPointStep   = static_cast<float>(index - 1u);
    const auto [sine, cosine] = za::sinCosLookup(startAngle + arcPointStep * arcAngleStep);

    ZA_ASSERT_AND_ASSUME(sine >= -1.f && sine <= 1.f);
    ZA_ASSERT_AND_ASSUME(cosine >= -1.f && cosine <= 1.f);

    // Winding is CW-visual in screen space (y-down) so that outline normals point inward:
    // `outlineThickness > 0` overlays the outline on top of the fill without growing the bounds.
    return {radius * (1.f - sine), radius * (1.f + cosine)};
}

////////////////////////////////////////////////////////////
/// \brief Computes a vertex point for a filled convex arc (pie slice).
///
/// A filled arc is defined by a center point, a radius, a starting
/// angle, and an angular extent (sweep angle). The shape consists
/// of the center point (vertex `0`) and a series of points along the
/// circular arc segment (vertices `1` to `pointCount - 1`), suitable for
/// rendering as a triangle fan originating from the center.
///
/// The arc is generated using the same coordinate system and angle
/// convention as `computeCirclePoint`: the implicit center of the circle
/// from which the arc is derived is at `(radius, radius)`. An angle of
/// `0` radians corresponds to the top point `(radius, 2 * radius)`,
/// `pi / 2` radians to the right point `(2 * radius, radius)`, `pi`
/// radians to the bottom point `(radius, 0)`, and `3 * pi / 2` radians
/// to the left point `(0, radius)`.
///
/// The resulting shape must be convex, which requires the absolute value
/// of `sweepAngle` to be greater than `0` and less than or equal to `pi` radians
/// (`180` degrees).
///
/// Vertex `0` is the center point, located at `(radius, radius)`.
/// Vertices `1` to `pointCount - 1` trace the curved edge of the arc.
///
/// \param index      The index of the vertex to compute `(0 <= index < pointCount)`.
/// \param radius     The radius of the arc.
/// \param startAngle The starting angle of the arc in radians (using the described convention).
/// \param sweepAngle The angular extent of the arc in radians (must be > `0` and <= `pi`).
///                   Positive sweeps counter-clockwise, negative sweeps clockwise.
/// \param pointCount Total number of vertices for the shape (must be >= `3`).
///                   Includes the center point and (`pointCount - 1`) points on the arc.
///
/// \return The computed 2D position of the vertex.
///
/// \see computeCirclePoint
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f computePieSlicePoint(
    const za::SizeT    index,
    const float        radius,
    const float        startAngle,
    const float        sweepAngle,
    const unsigned int pointCount) noexcept
{
    ZA_ASSERT_AND_ASSUME(pointCount >= 3u); // Need center + at least start/end points on arc
    ZA_ASSERT_AND_ASSUME(sweepAngle > 0.f); // Sweep angle must be positive
    ZA_ASSERT_AND_ASSUME(index < pointCount);

    return computePieSlicePointFromArcAngleStep(index, radius, computePieSliceArcAngleStep(sweepAngle, pointCount), startAngle);
}

////////////////////////////////////////////////////////////
/// \see computeStarPoint
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f computeStarPointFromAngleStep(
    const za::SizeT index,
    const float     angleStep,
    const float     outerRadius,
    const float     innerRadius) noexcept
{
    ZA_ASSERT_AND_ASSUME(outerRadius >= 0.f);
    ZA_ASSERT_AND_ASSUME(innerRadius >= 0.f);

    const float angle = static_cast<float>(index) * angleStep - za::halfPi; // Start from top (-pi/2)

    const float radius = (index % 2u == 0u) ? outerRadius : innerRadius;

    const auto [sine, cosine] = za::sinCosLookup(angle);

    ZA_ASSERT_AND_ASSUME(sine >= -1.f && sine <= 1.f);
    ZA_ASSERT_AND_ASSUME(cosine >= -1.f && cosine <= 1.f);

    // Use outerRadius for centering consistently, like circle/ellipse
    return {outerRadius + radius * cosine, outerRadius + radius * sine};
}

////////////////////////////////////////////////////////////
/// \brief Computes a vertex point for a star shape.
///
/// A star is defined by alternating outer and inner points around a center.
/// Vertices are generated by alternating between outerRadius and innerRadius
/// at regular angular steps. The total number of vertices generated is
/// `2 * pointCount`. Vertex indices `0, 2, 4, ...` correspond to outer points,
/// and indices `1, 3, 5, ...` correspond to inner points.
///
/// Assumes the center for angle calculations is `(outerRadius, outerRadius)`.
///
/// \param index        The index of the vertex to compute `(0 <= index < 2 * pointCount)`.
/// \param pointCount   The number of points the star has (e.g., 5 for a 5-pointed star, must be >= 2).
/// \param outerRadius  Distance from the center to the outer points.
/// \param innerRadius  Distance from the center to the inner points.
///
/// \return The computed 2D position of the vertex.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f computeStarPoint(
    const za::SizeT    index,
    const unsigned int pointCount,
    const float        outerRadius,
    const float        innerRadius) noexcept
{
    ZA_ASSERT_AND_ASSUME(pointCount >= 2u);
    ZA_ASSERT_AND_ASSUME(index < 2u * pointCount);

    const float angleStep = za::tau / static_cast<float>(2u * pointCount);

    return computeStarPointFromAngleStep(index, angleStep, outerRadius, innerRadius);
}

////////////////////////////////////////////////////////////
/// \brief Computes the outer and inner points of a ring.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr auto computeRingPointsFromAngleStep(
    const za::SizeT index,
    const float     startRadians,
    const float     angleStep,
    const float     outerRadius,
    const float     innerRadius)
{
    ZA_ASSERT_AND_ASSUME(outerRadius >= 0.f);
    ZA_ASSERT_AND_ASSUME(innerRadius >= 0.f);

    const auto [sine, cosine] = za::sinCosLookup(startRadians + static_cast<float>(index) * angleStep);

    ZA_ASSERT_AND_ASSUME(sine >= -1.f && sine <= 1.f);
    ZA_ASSERT_AND_ASSUME(cosine >= -1.f && cosine <= 1.f);

    // Local center of the ring
    const Vec2f localCenter{outerRadius, outerRadius};

    // Calculate offset from `localCenter`
    const Vec2f outerOffset{outerRadius * cosine, outerRadius * sine};
    const Vec2f innerOffset{innerRadius * cosine, innerRadius * sine};

    struct RingPoints
    {
        Vec2f outerPoint;
        Vec2f innerPoint;
    };

    // Add offset to `localCenter` to get final local position
    return RingPoints{localCenter + outerOffset, localCenter + innerOffset};
}

////////////////////////////////////////////////////////////
/// \brief Computes a vertex point for a cross (plus) shape.
///
/// The cross occupies a bounding box of dimensions `size` with a
/// horizontal and a vertical arm of common `armThickness` sharing a
/// central square. Vertices are generated in counter-clockwise
/// (math) / clockwise (screen-y-down) winding order, starting at
/// the top-left corner of the top arm.
///
/// \param index         The index of the vertex to compute `(0 <= index < 12)`.
/// \param size          Overall bounding size of the cross.
/// \param armThickness  Thickness of the horizontal and vertical arms
///                      (clamped to `min(size.x, size.y)`).
///
/// \return The computed 2D position of the boundary vertex.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr Vec2f computeCrossPoint(
    const za::SizeT index,
    const Vec2f     size,
    const float     armThickness) noexcept
{
    ZA_ASSERT_AND_ASSUME(index < 12u);
    ZA_ASSERT_AND_ASSUME(armThickness >= 0.f);

    // Clamp the arm thickness to the smaller bounding dimension: beyond that, the inner
    // corners would cross over and create a self-intersecting polygon escaping `size`.
    // Clamping lets the shape cleanly degenerate into a full rectangle instead.
    const float clampedThickness = ZA_MIN(armThickness, ZA_MIN(size.x, size.y));

    const float cx = size.x * 0.5f;
    const float cy = size.y * 0.5f;
    const float h  = clampedThickness * 0.5f;

    const Vec2f points[12u] = {
        {cx - h, 0.f},    //  0: top-left of top arm
        {cx + h, 0.f},    //  1: top-right of top arm
        {cx + h, cy - h}, //  2: inner corner (top-right)
        {size.x, cy - h}, //  3: top-right of right arm
        {size.x, cy + h}, //  4: bottom-right of right arm
        {cx + h, cy + h}, //  5: inner corner (bottom-right)
        {cx + h, size.y}, //  6: bottom-right of bottom arm
        {cx - h, size.y}, //  7: bottom-left of bottom arm
        {cx - h, cy + h}, //  8: inner corner (bottom-left)
        {0.f, cy + h},    //  9: bottom-left of left arm
        {0.f, cy - h},    // 10: top-left of left arm
        {cx - h, cy - h}, // 11: inner corner (top-left)
    };

    return points[index];
}

////////////////////////////////////////////////////////////
/// \brief Computes a vertex point for an isosceles trapezoid.
///
/// The trapezoid has its top edge of width `topWidth` at `y = 0`
/// and bottom edge of width `bottomWidth` at `y = height`, both
/// horizontally centered within a bounding box of width
/// `max(topWidth, bottomWidth)`.
///
/// \param index       The index of the vertex to compute `(0 <= index < 4)`.
/// \param topWidth    Width of the top edge.
/// \param bottomWidth Width of the bottom edge.
/// \param height      Vertical distance between top and bottom edges.
///
/// \return The computed 2D position of the boundary vertex.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr Vec2f computeTrapezoidPoint(
    const za::SizeT index,
    const float     topWidth,
    const float     bottomWidth,
    const float     height) noexcept
{
    ZA_ASSERT_AND_ASSUME(index < 4u);

    const float maxW  = ZA_MAX(topWidth, bottomWidth);
    const float topXL = (maxW - topWidth) * 0.5f;
    const float topXR = topXL + topWidth;
    const float botXL = (maxW - bottomWidth) * 0.5f;
    const float botXR = botXL + bottomWidth;

    const Vec2f points[4u] = {
        {topXL, 0.f},    // 0: top-left
        {topXR, 0.f},    // 1: top-right
        {botXR, height}, // 2: bottom-right
        {botXL, height}, // 3: bottom-left
    };

    return points[index];
}

////////////////////////////////////////////////////////////
/// \brief Computes a vertex point for a chevron (`>`) shape.
///
/// The chevron occupies a bounding box of dimensions `size` with
/// its tip at `{size.x, size.y/2}` and its open (back) end along
/// `x = 0`. `thickness` is the vertical thickness of the stroke at
/// the back; the inner tip is pulled back from the outer tip so
/// that the two arms have constant perpendicular thickness.
///
/// Vertices are returned in CCW (math) / CW (screen-y-down) winding
/// order: outer top-back, outer tip, outer bottom-back, inner
/// bottom-back, inner tip, inner top-back.
///
/// When `thickness >= size.y / 2`, the chevron fills up entirely:
/// the three inner vertices all collapse to `{0, size.y / 2}` and
/// the resulting 6-vertex polygon degenerates into the outer
/// triangle `{(0, 0), (size.x, size.y/2), (0, size.y)}` (with the
/// three duplicate inner vertices producing zero-area triangles in
/// the fan). This keeps the fill continuous as `thickness` crosses
/// `size.y / 2` without self-intersections.
///
/// \param index     The index of the vertex to compute `(0 <= index < 6)`.
/// \param size      Overall bounding size of the chevron.
/// \param thickness Vertical stroke thickness at the back (clamped to `size.y / 2`).
///
/// \return The computed 2D position of the boundary vertex.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr Vec2f computeChevronPoint(
    const za::SizeT index,
    const Vec2f     size,
    const float     thickness) noexcept
{
    ZA_ASSERT_AND_ASSUME(index < 6u);
    ZA_ASSERT_AND_ASSUME(thickness >= 0.f);
    ZA_ASSERT_AND_ASSUME(size.y > 0.f);

    const float halfH = size.y * 0.5f;

    // Clamp thickness to half the height: beyond that, the inner vertices would cross
    // and create a self-intersecting polygon. Clamping lets the shape cleanly degenerate
    // into the outer triangle instead.
    const float clampedThickness = ZA_MIN(thickness, halfH);

    const float innerTipX = size.x * (1.f - 2.f * clampedThickness / size.y);

    const Vec2f points[6u] = {
        {0.f, 0.f},                       // 0: outer top-back
        {size.x, halfH},                  // 1: outer tip
        {0.f, size.y},                    // 2: outer bottom-back
        {0.f, size.y - clampedThickness}, // 3: inner bottom-back
        {innerTipX, halfH},               // 4: inner tip
        {0.f, clampedThickness},          // 5: inner top-back
    };

    return points[index];
}

////////////////////////////////////////////////////////////
/// \brief Computes a vertex point for a heart shape.
///
/// Based on the classic valentine-heart parametric curve
/// `x(t) = 16 sin^3(t)`,
/// `y(t) = 13 cos(t) - 5 cos(2t) - 2 cos(3t) - cos(4t)`,
/// rescaled to fit the axis-aligned bounding box
/// `[0, size.x] x [0, size.y]` in screen (y-down) coordinates.
///
/// The classic curve has two tangent cusps (both derivatives vanish)
/// at `t = 0` (dimple) and `t = pi` (bottom tip). These destabilize
/// the outline miter calculation regardless of where samples are
/// placed, because the curve itself does an instantaneous U-turn at
/// each cusp. To eliminate that, the `sin^3` factor is replaced by
/// `sin(t) * (sin^2(t) + delta)` with a small `delta`. At `t = 0`
/// and `t = pi`, `dx/dt = 16 * cos(t) * delta` is now non-zero, so
/// the tangent is a finite horizontal vector instead of a zero
/// vector -- a smooth point rather than a cusp. For small `delta`
/// the lobes widen and the notches round off by well under a pixel
/// at typical sizes, so the heart still looks like a classic
/// valentine heart.
///
/// \param index      The index of the vertex to compute `(0 <= index < pointCount)`.
/// \param pointCount Total number of samples along the contour (must be >= 4).
/// \param size       Overall bounding size of the heart.
///
/// \return The computed 2D position of the boundary vertex.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f computeHeartPoint(
    const za::SizeT    index,
    const unsigned int pointCount,
    const Vec2f        size) noexcept
{
    ZA_ASSERT_AND_ASSUME(pointCount >= 4u);
    ZA_ASSERT_AND_ASSUME(index < pointCount);

    // Cusp-removal smoothing. Larger kDelta reduces curvature at the former cusps, which keeps
    // the outline miter factor (1 + n1.n2) well above 0 at adjacent samples and avoids the
    // miter-limit clamp that causes visible spikes sticking past the shape boundary.
    constexpr float kDelta = 0.15f;
    constexpr float kXMax  = 16.f * (1.f + kDelta);
    constexpr float kYMax  = 12.f; // analytic y_max ~= 11.93; 12 gives a tiny safety margin
    constexpr float kYMin  = -17.f;

    const float t = za::tau * static_cast<float>(index) / static_cast<float>(pointCount);

    const auto [sinT, cosT] = za::sinCosLookup(t);

    const float sin2  = sinT * sinT;
    const float cos2t = cosT * cosT - sin2;
    const float cos3t = 4.f * cosT * cosT * cosT - 3.f * cosT;
    const float cos4t = 2.f * cos2t * cos2t - 1.f;

    const float xMath = 16.f * sinT * (sin2 + kDelta);
    const float yMath = 13.f * cosT - 5.f * cos2t - 2.f * cos3t - cos4t;

    return {(xMath + kXMax) / (2.f * kXMax) * size.x, (kYMax - yMath) / (kYMax - kYMin) * size.y};
}

////////////////////////////////////////////////////////////
/// \brief Computes a vertex point for a cog (gear) shape.
///
/// A cog has `toothCount` rectangular teeth evenly distributed
/// around its center. Each angular sector (of width
/// `2*pi / toothCount`) is split into a tooth of angular width
/// `toothWidthRatio * (2*pi / toothCount)` and a gap filling the
/// remainder. Four vertices are emitted per tooth (gap-start at
/// inner radius, tooth-tip-left at outer radius, tooth-tip-right
/// at outer radius, gap-end at inner radius).
///
/// The shape is centered at `(outerRadius, outerRadius)` to match
/// the convention used by `computeStarPoint`/`computeCirclePoint`.
///
/// \param index           The index of the vertex to compute `(0 <= index < 4 * toothCount)`.
/// \param toothCount      Number of teeth (must be >= 3).
/// \param outerRadius     Distance from the center to a tooth tip.
/// \param innerRadius     Distance from the center to a tooth root.
/// \param toothWidthRatio Fraction of each angular sector that is tooth (in `(0, 1)`).
///
/// \return The computed 2D position of the boundary vertex.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f computeCogPoint(
    const za::SizeT    index,
    const unsigned int toothCount,
    const float        outerRadius,
    const float        innerRadius,
    const float        toothWidthRatio) noexcept
{
    ZA_ASSERT_AND_ASSUME(toothCount >= 3u);
    ZA_ASSERT_AND_ASSUME(index < 4u * toothCount);
    ZA_ASSERT_AND_ASSUME(outerRadius >= 0.f);
    ZA_ASSERT_AND_ASSUME(innerRadius >= 0.f);
    ZA_ASSERT_AND_ASSUME(toothWidthRatio > 0.f && toothWidthRatio < 1.f);

    const float sector     = za::tau / static_cast<float>(toothCount);
    const float toothHalfA = sector * toothWidthRatio * 0.5f;
    const float sectorHalf = sector * 0.5f;

    const za::SizeT toothIdx   = index / 4u;
    const za::SizeT vertexKind = index % 4u; // 0: root start, 1: tip left, 2: tip right, 3: root end
    const float     center     = sector * static_cast<float>(toothIdx) - za::halfPi; // start from top

    const float offsets[4u] = {-sectorHalf, -toothHalfA, toothHalfA, sectorHalf};
    const float radii[4u]   = {innerRadius, outerRadius, outerRadius, innerRadius};

    const float angle  = center + offsets[vertexKind];
    const float radius = radii[vertexKind];

    const auto [sine, cosine] = za::sinCosLookup(angle);

    // Center at (outerRadius, outerRadius) for consistency with star/circle
    return {outerRadius + radius * cosine, outerRadius + radius * sine};
}

////////////////////////////////////////////////////////////
/// \brief Computes the normalized perpendicular of a segment.
///
/// Given two points defining a line segment, this function calculates the unit vec2
/// perpendicular to that segment.
///
/// \param p1 The starting point of the segment.
/// \param p2 The ending point of the segment.
///
/// \return The normal vec2 (normalized).
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline Vec2f computeSegmentNormal(const Vec2f p1, const Vec2f p2) noexcept
{
    // Compute the difference and its perpendicular.
    const Vec2f diff   = p2 - p1;
    const Vec2f normal = {-diff.y, diff.x};

    // Compute squared length of the normal.
    const float lenSq = normal.x * normal.x + normal.y * normal.y;

    // By computing the squared length (lenSq), we can avoid an extra temporary if the squared length is zero.
    // Only if nonzero do we compute the square root and the reciprocal.

    // Avoid division by zero.
    if (lenSq == 0.f)
        return normal;

    const float invLen = 1.f / ZA_MATH_SQRTF(lenSq);
    return {normal.x * invLen, normal.y * invLen};
}

////////////////////////////////////////////////////////////
/// \brief Internal helper to compute vertices for a shape's outline.
///
/// This template function calculates the vertices needed to draw an outline
/// of a given thickness around a shape defined by a set of fill points.
/// It generates a triangle strip, with two vertices (inner and outer)
/// for each point of the base shape, plus two additional vertices to
/// close the outline loop.
///
/// \param outlineThickness The desired thickness of the outline.
/// \param fillPositionFn   A function or lambda that, when called with an index
///                         `i` (from `0` to `pointCount - 1`), returns the
///                         `Vec2f` position of the i-th fill point of the shape.
/// \param outlineVertices  Pointer to the vertex array where the computed
///                         outline vertices will be stored. This array must be
///                         large enough to hold `(pointCount + 1) * 2` vertices.
/// \param pointCount       The number of unique points defining the base shape's
///                         fill area (e.g., for a triangle, this is 3).
/// \param miterLimit       The miter limit, used to control the appearance of
///                         sharp corners. If the miter length (distance from
///                         the inner to the outer corner point) exceeds this
///                         limit, a bevel join will be used instead of a miter join.
///
////////////////////////////////////////////////////////////
inline constexpr void updateOutlineImpl(const float               outlineThickness,
                                        auto&&                    fillPositionFn,
                                        Vertex* const ZA_RESTRICT outlineVertices,
                                        const za::SizeT           pointCount,
                                        const float               miterLimit) noexcept
{
    ZA_ASSERT_AND_ASSUME(outlineVertices != nullptr);
    ZA_ASSERT_AND_ASSUME(pointCount >= 3u);
    ZA_ASSERT_AND_ASSUME(miterLimit > 0.f);

    const float miterLimitSq = miterLimit * miterLimit; // Precompute squared limit

    for (za::SizeT i = 0u; i < pointCount; ++i)
    {
        const Vec2f& p1 = fillPositionFn(i);
        const Vec2f& p0 = fillPositionFn((i == 0u) ? (pointCount - 1u) : (i - 1u)); // Previous point
        const Vec2f& p2 = fillPositionFn((i + 1u) % pointCount);                    // Next point

        const Vec2f n1 = computeSegmentNormal(p0, p1);
        const Vec2f n2 = computeSegmentNormal(p1, p2);

        // Calculate the factor `1 + n1.dot(n2)`. This is related to the angle.
        // factor = 1 + cos(angle_between_normals)
        // If angle is near 180 degrees (segments nearly collinear, sharp internal angle), factor is near 0.
        // If angle is near 0 degrees (segments nearly collinear, flat internal angle), factor is near 2.
        const float factor = 1.f + n1.dot(n2);

        // Handle near-collinear segments (factor close to 0) to avoid division issues.
        // Use a simple normal offset in this case (effectively a bevel/butt cap).
        // Threshold chosen somewhat arbitrarily, needs to be small but non-zero.
        constexpr float collinearThreshold = 1e-5f;

        const auto outlineNormal = (ZA_MATH_FABSF(factor) < collinearThreshold)
                                       ? n1 // Segments are almost parallel/anti-parallel, use one normal
                                       : ((n1 + n2) / factor).clampMaxLengthSquared(miterLimitSq); // miter vec2 direction with miter limit

        const auto offsetVec2 = outlineNormal * outlineThickness;

        // Store the outline vertex positions
        const za::SizeT outIdx = i << 1u; // i * 2

        outlineVertices[outIdx + 0u].position = p1;              // Inner vertex
        outlineVertices[outIdx + 1u].position = p1 + offsetVec2; // Outer vertex
    }

    // Duplicate the first outline vertex pair at the end to close the outline loop.
    const za::SizeT dupIndex = pointCount << 1u; // pointCount * 2

    outlineVertices[dupIndex + 0u].position = outlineVertices[0].position;
    outlineVertices[dupIndex + 1u].position = outlineVertices[1].position;
}

////////////////////////////////////////////////////////////
/// \brief Computes outline vertices for a shape with a triangle fan fill.
///
/// This function is a convenience wrapper around `updateOutlineImpl`.
/// It is specifically designed for shapes whose fill is represented by
/// a triangle fan structure (e.g., a central point followed by points
/// tracing the perimeter). It extracts the fill point positions directly
/// from the `fillVertices` array to pass to `updateOutlineImpl`.
///
/// \param outlineThickness The desired thickness of the outline.
/// \param fillVertices     Pointer to the vertex array defining the shape's
///                         fill as a triangle fan. The positions from these
///                         vertices are used as the base for the outline.
/// \param outlineVertices  Pointer to the vertex array where the computed
///                         outline vertices will be stored. This array must be
///                         large enough to hold `(pointCount + 1) * 2` vertices.
/// \param pointCount       The number of points defining the perimeter of the
///                         triangle fan (i.e., total fill vertices minus the
///                         central hub point, if one exists and is part of the count
///                         passed to the underlying `updateOutlineImpl`).
/// \param miterLimit       The miter limit, used to control the appearance of
///                         sharp corners.
///
/// \see updateOutlineImpl
///
////////////////////////////////////////////////////////////
inline constexpr void updateOutlineFromTriangleFanFill(
    const float                     outlineThickness,
    const Vertex* const ZA_RESTRICT fillVertices,
    Vertex* const ZA_RESTRICT       outlineVertices,
    const za::SizeT                 pointCount,
    const float                     miterLimit) noexcept
{
    ZA_ASSERT_AND_ASSUME(fillVertices != nullptr);

    updateOutlineImpl(outlineThickness, [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i) {
        return fillVertices[i].position;
    }, outlineVertices, pointCount, miterLimit);
}

////////////////////////////////////////////////////////////
/// \brief Computes the geometric center (centroid) of a convex polygon.
///
/// This function calculates the 2D coordinates of the centroid for a
/// convex polygon defined by an ordered list of vertices.
/// The calculation is based on the formula for the centroid of a polygon,
/// which involves summing the cross products of adjacent vertices.
///
/// Preconditions:
/// - The polygon must be convex.
/// - The vertices in the `points` array must be ordered (e.g., clockwise or counter-clockwise).
/// - `pointCount` must be greater than 0.
///
/// Edge cases:
/// - If `pointCount` is 1, the single point is returned.
/// - If `pointCount` is 2, the midpoint of the segment is returned.
/// - If the polygon has zero area (e.g., collinear points), the center
///   of its bounding box is returned as a fallback.
///
/// \param points Pointer to an array of `Vec2f` representing the vertices
///               of the convex polygon.
/// \param pointCount The number of vertices in the polygon.
///
/// \return `Vec2f` representing the coordinates of the geometric center.
///
////////////////////////////////////////////////////////////
inline constexpr Vec2f computeConvexShapeGeometricCenter(const Vec2f* points, const za::SizeT pointCount)
{
    ZA_ASSERT(pointCount > 0u && "Cannot calculate geometric center of shape with no points");

    if (pointCount == 1u)
        return points[0];

    if (pointCount == 2u)
        return (points[0] + points[1]) / 2.f;

    Vec2f centroid{};
    float twiceArea = 0;

    auto previousPoint = points[pointCount - 1];
    for (za::SizeT i = 0; i < pointCount; ++i)
    {
        const auto  currentPoint = points[i];
        const float product      = previousPoint.cross(currentPoint);
        twiceArea += product;
        centroid += (currentPoint + previousPoint) * product;

        previousPoint = currentPoint;
    }

    if (twiceArea != 0.f)
        return centroid / 3.f / twiceArea;

    // Fallback for no area - find the center of the bounding box
    Vec2f minPoint = points[0];
    Vec2f maxPoint = minPoint;

    for (za::SizeT i = 1; i < pointCount; ++i)
    {
        const auto currentPoint = points[i];

        minPoint.x = ZA_MIN(minPoint.x, currentPoint.x);
        maxPoint.x = ZA_MAX(maxPoint.x, currentPoint.x);
        minPoint.y = ZA_MIN(minPoint.y, currentPoint.y);
        maxPoint.y = ZA_MAX(maxPoint.y, currentPoint.y);
    }

    return (maxPoint + minPoint) / 2.f;
}

} // namespace za::ShapeUtils


////////////////////////////////////////////////////////////
/// \namespace za::ShapeUtils
/// \ingroup graphics
///
/// \brief Utility functions for geometric calculations related to shapes.
///
/// This namespace contains a collection of helper functions for performing
/// various geometric computations. These include calculating vertex points
/// for primitive shapes like circles, ellipses, rectangles (standard and rounded),
/// arrows, pie slices, and stars. It also provides utilities for generating
/// vertices for shape outlines and for finding the geometric center of
/// convex polygons.
///
/// While these functions are primarily used internally by Zancle's shape
/// classes (e.g., `za::Shape`, `za::CircleShape`, `za::RectangleShape`),
/// they may also be useful for advanced users who need to perform custom
/// geometric operations or create new types of drawable shapes.
///
/// \see za::Shape, za::CircleShape, za::RectangleShape, za::ConvexShape
///
////////////////////////////////////////////////////////////
