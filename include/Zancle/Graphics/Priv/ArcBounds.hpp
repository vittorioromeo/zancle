#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/Vec2.hpp"

#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/MinMaxMacros.hpp"
#include "Zancle/Math/Remainder.hpp"
#include "Zancle/Math/SinCosLookup.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Point on a circle of `radius` centered at `(outerRadius, outerRadius)`.
///
/// Uses the ring convention `(cos, sin)` shared by `RingShapeData`,
/// `RingPieSliceShapeData`, and `CurvedArrowShapeData`. The implicit
/// center at `(outerRadius, outerRadius)` matches the local bounds
/// origin of these shapes.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline Vec2f ringArcPointAtAngle(
    const float outerRadius,
    const float radius,
    const float radians) noexcept
{
    const auto [sine, cosine] = za::sinCosLookup(radians);
    return {outerRadius + radius * cosine, outerRadius + radius * sine};
}


////////////////////////////////////////////////////////////
/// \brief Centroid offset of an annulus sector along its bisector.
///
/// For an annulus sector with outer radius `R`, inner radius `r`, and
/// sweep `2*alpha`, the geometric centroid lies on the bisector at
/// distance `d = 4 * sin(alpha) * (R^3 - r^3) / (3 * sweep * (R^2 - r^2))`
/// from the ring's center. Returns `0` when the sector degenerates to
/// zero area (`R == r` or `sweep == 0`).
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline float annulusSectorCentroidDistance(
    const float outerRadius,
    const float innerRadius,
    const float sweepRad,
    const float sinHalfSweep) noexcept
{
    const float outerSq = outerRadius * outerRadius;
    const float innerSq = innerRadius * innerRadius;
    const float diffSq  = outerSq - innerSq;

    if (diffSq <= 0.f || sweepRad <= 0.f) [[unlikely]]
        return 0.f;

    const float diffCu = outerSq * outerRadius - innerSq * innerRadius;
    return (4.f * sinHalfSweep * diffCu) / (3.f * sweepRad * diffSq);
}


////////////////////////////////////////////////////////////
/// \brief Tight AABB of a circular arc.
///
/// Computes the axis-aligned bounding box of an arc (and optionally
/// a separate hub/center point) by folding the arc's two endpoints
/// together with any of the four cardinal extrema (at local angles
/// `{0, pi/2, pi, 3*pi/2}`) that fall inside the swept range.
///
/// `pointAt(angle) -> Vec2f` converts an angle into the arc's local
/// 2D position (the caller decides the angle convention: circle-
/// style `(cos, sin)` or pie-slice style `(sin, cos)`). `mapFn(p)`
/// maps a local position into the target space: identity for local
/// bounds, or a `Transform::transformPoint` wrapper for tight world
/// bounds.
///
/// \param startRad    Start angle of the arc (radians).
/// \param sweepRad    Sweep angle, in radians (positive).
/// \param includeHub  Whether to fold an explicit hub point into the AABB.
/// \param hub         Hub point (in the same local space as `pointAt`).
/// \param pointAt     Callable mapping `angle -> Vec2f` on the arc.
/// \param mapFn       Callable mapping a local `Vec2f` to the desired space.
///
/// \return AABB of the mapped arc points (and hub, when requested).
///
////////////////////////////////////////////////////////////
template <typename PointAtAngleFn, typename MapFn>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline Rect2f computeArcBounds(
    const float      startRad,
    const float      sweepRad,
    const bool       includeHub,
    const Vec2f      hub,
    PointAtAngleFn&& pointAt,
    MapFn&&          mapFn) noexcept
{
    // Seed the AABB with the start-of-arc endpoint (always on the shape).
    Vec2f seed = mapFn(pointAt(startRad));

    float minX = seed.x;
    float maxX = seed.x;
    float minY = seed.y;
    float maxY = seed.y;

    const auto fold = [&](const Vec2f p)
    {
        minX = ZA_MIN(minX, p.x);
        maxX = ZA_MAX(maxX, p.x);
        minY = ZA_MIN(minY, p.y);
        maxY = ZA_MAX(maxY, p.y);
    };

    // End-of-arc endpoint.
    fold(mapFn(pointAt(startRad + sweepRad)));

    if (includeHub)
        fold(mapFn(hub));

    // Cardinal critical angles: any of {0, pi/2, pi, 3*pi/2} inside the swept range
    // contributes an extremum of one of the cartesian components (which one depends
    // on the caller's `pointAt` convention -- the helper doesn't need to know).
    const auto inSweep = [startRad, sweepRad](const float theta) noexcept
    {
        if (sweepRad >= za::tau)
            return true;

        return za::positiveRemainder(theta - startRad, za::tau) <= sweepRad;
    };

    if (inSweep(0.f))
        fold(mapFn(pointAt(0.f)));

    if (inSweep(za::halfPi))
        fold(mapFn(pointAt(za::halfPi)));

    if (inSweep(za::pi))
        fold(mapFn(pointAt(za::pi)));

    if (inSweep(3.f * za::halfPi))
        fold(mapFn(pointAt(3.f * za::halfPi)));

    return {{minX, minY}, {maxX - minX, maxY - minY}};
}


////////////////////////////////////////////////////////////
/// \brief Tight AABB of a discrete set of transformed shape vertices.
///
/// Folds `count` points returned by `pointAt(index)` through `mapFn`
/// into an AABB. Used to compute tight world-space bounds for
/// polygonal shapes by iterating their actual perimeter vertices
/// (the tip of an arrow, the inner corners of a cross, ...) rather
/// than the corners of a local bounding rectangle, which would
/// overestimate the bbox whenever the shape's perimeter does not
/// reach every corner of its local bounds.
///
////////////////////////////////////////////////////////////
template <typename PointAtFn, typename MapFn>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline Rect2f computePolygonBounds(const unsigned int count,
                                                                                   PointAtFn&&        pointAt,
                                                                                   MapFn&&            mapFn) noexcept
{
    Vec2f seed = mapFn(pointAt(0u));

    float minX = seed.x;
    float maxX = seed.x;
    float minY = seed.y;
    float maxY = seed.y;

    for (unsigned int i = 1u; i < count; ++i)
    {
        const Vec2f p = mapFn(pointAt(i));

        minX = ZA_MIN(minX, p.x);
        maxX = ZA_MAX(maxX, p.x);
        minY = ZA_MIN(minY, p.y);
        maxY = ZA_MAX(maxY, p.y);
    }

    return {{minX, minY}, {maxX - minX, maxY - minY}};
}


////////////////////////////////////////////////////////////
/// \brief Merge two AABBs into their enclosing AABB.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Rect2f mergeAabb(const Rect2f a, const Rect2f b) noexcept
{
    const float minX = ZA_MIN(a.position.x, b.position.x);
    const float minY = ZA_MIN(a.position.y, b.position.y);
    const float maxX = ZA_MAX(a.position.x + a.size.x, b.position.x + b.size.x);
    const float maxY = ZA_MAX(a.position.y + a.size.y, b.position.y + b.size.y);

    return {{minX, minY}, {maxX - minX, maxY - minY}};
}

} // namespace za::priv
