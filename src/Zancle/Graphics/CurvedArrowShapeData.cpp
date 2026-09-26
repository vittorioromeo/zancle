// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/CurvedArrowShapeData.hpp"

#include "Zancle/Graphics/Priv/ArcBounds.hpp"
#include "Zancle/Graphics/ShapeUtils.hpp"
#include "Zancle/Graphics/Transform.hpp"

#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/Vec2.hpp"

#include "Zancle/Math/Ceil.hpp"
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/MinMax.hpp"
#include "Zancle/Math/MinMaxMacros.hpp"
#include "Zancle/Math/Remainder.hpp"
#include "Zancle/Math/SinCosLookup.hpp"


namespace za
{
namespace
{
////////////////////////////////////////////////////////////
// Fold the arrowhead's 3 local vertices (tip + two barbs) into `inout`.
////////////////////////////////////////////////////////////
template <typename MapFn>
[[gnu::always_inline, gnu::flatten]] inline void foldCurvedArrowHeadPoints(Rect2f&                     inout,
                                                                           const CurvedArrowShapeData& sd,
                                                                           MapFn&&                     mapFn) noexcept
{
    const float sweepRad   = sd.sweepAngle.asRadians();
    const float sweepSign  = (sweepRad < 0.f) ? -1.f : 1.f;
    const float endAngle   = sd.startAngle.asRadians() + sweepRad;
    const float pathRadius = (sd.outerRadius + sd.innerRadius) * 0.5f;

    const auto [endSin, endCos] = za::sinCosLookup(za::positiveRemainder(endAngle, za::tau));

    const Vec2f attach    = {sd.outerRadius + pathRadius * endCos, sd.outerRadius + pathRadius * endSin};
    const Vec2f tangent   = {-endSin, endCos};
    const Vec2f radialOut = {endCos, endSin};

    const Vec2f tip       = attach + tangent * (sd.headLength * sweepSign);
    const Vec2f outerBarb = attach + radialOut * (sd.headWidth * 0.5f);
    const Vec2f innerBarb = attach - radialOut * (sd.headWidth * 0.5f);

    const auto fold = [&](const Vec2f localPoint)
    {
        const Vec2f p    = mapFn(localPoint);
        const float minX = ZA_MIN(inout.position.x, p.x);
        const float minY = ZA_MIN(inout.position.y, p.y);
        const float maxX = ZA_MAX(inout.position.x + inout.size.x, p.x);
        const float maxY = ZA_MAX(inout.position.y + inout.size.y, p.y);
        inout            = {{minX, minY}, {maxX - minX, maxY - minY}};
    };

    fold(tip);
    fold(outerBarb);
    fold(innerBarb);
}

} // namespace


////////////////////////////////////////////////////////////
Rect2f CurvedArrowShapeData::getLocalBounds() const noexcept
{
    if (!hasVisibleGeometry()) [[unlikely]]
        return {};

    // Normalize for `computeArcBounds`, which expects a non-negative sweep starting at `startRad`.
    // A negative sweep sweeps backwards from `startAngle`, so the equivalent forward-sweep sector
    // starts at `startAngle + sweepAngle` and has sweep `|sweepAngle|`.
    const float sweepRad = sweepAngle.asRadians();
    const float absSweep = ZA_MATH_FABSF(sweepRad);
    const float startRad = (sweepRad < 0.f) ? startAngle.asRadians() + sweepRad : startAngle.asRadians();
    const auto  idFn     = [](const Vec2f p) noexcept { return p; };

    const auto outerAabb = priv::computeArcBounds(startRad,
                                                  absSweep,
                                                  /* includeHub */ false,
                                                  {},
                                                  [this](const float a) noexcept
    { return priv::ringArcPointAtAngle(outerRadius, outerRadius, a); },
                                                  idFn);

    const auto innerAabb = priv::computeArcBounds(startRad,
                                                  absSweep,
                                                  /* includeHub */ false,
                                                  {},
                                                  [this](const float a) noexcept
    { return priv::ringArcPointAtAngle(outerRadius, innerRadius, a); },
                                                  idFn);

    Rect2f result = priv::mergeAabb(outerAabb, innerAabb);
    foldCurvedArrowHeadPoints(result, *this, idFn);
    return result;
}


////////////////////////////////////////////////////////////
Rect2f CurvedArrowShapeData::getGlobalBounds() const noexcept
{
    if (!hasVisibleGeometry()) [[unlikely]]
        return {};

    const auto transform = Transform::fromPositionScaleOriginRotation(position, scale, origin, rotation);

    const float sweepRad = sweepAngle.asRadians();
    const float startRad = startAngle.asRadians();

    // Must match the arc tessellation of `DrawableBatchImpl::add(const CurvedArrowShapeData&)`:
    // `pointCount` is per 90 degrees of sweep.
    const unsigned int arcSamples = za::max(2u,
                                            static_cast<unsigned int>(ZA_MATH_CEILF(
                                                static_cast<float>(pointCount) * (ZA_MATH_FABSF(sweepRad) / za::halfPi))));

    const float arcStep = ShapeUtils::computeArcAngleStep(sweepRad, arcSamples);

    const auto transformFn = [&](const Vec2f p) noexcept { return transform.transformPoint(p); };

    Rect2f result = priv::computePolygonBounds(2u * arcSamples,
                                               [&](const unsigned int i) noexcept
    {
        const unsigned int local  = i % arcSamples;
        const float        radius = (i < arcSamples) ? outerRadius : innerRadius;
        return priv::ringArcPointAtAngle(outerRadius, radius, startRad + static_cast<float>(local) * arcStep);
    },
                                               transformFn);

    foldCurvedArrowHeadPoints(result, *this, transformFn);
    return result;
}


////////////////////////////////////////////////////////////
Vec2f CurvedArrowShapeData::getCentroid() const noexcept
{
    const Vec2f center{outerRadius, outerRadius};

    if (!hasVisibleGeometry()) [[unlikely]]
        return center;

    const float sweepRad   = sweepAngle.asRadians();
    const float absSweep   = ZA_MATH_FABSF(sweepRad);
    const float halfSwpAbs = absSweep * 0.5f;
    const float sweepSign  = (sweepRad < 0.f) ? -1.f : 1.f;
    const float startRad   = startAngle.asRadians();

    // ---- Body: annulus sector centroid (centered on the ring center, along the bisector). ----
    const float diffSq = outerRadius * outerRadius - innerRadius * innerRadius;

    Vec2f bodyCentroid = center;
    float bodyArea     = 0.f;

    if (diffSq > 0.f && absSweep > 0.f)
    {
        const auto [sinHalf, cosHalf] = za::sinCosLookup(za::positiveRemainder(halfSwpAbs, za::tau));

        const float d        = priv::annulusSectorCentroidDistance(outerRadius, innerRadius, absSweep, sinHalf);
        const float bisector = startRad + sweepRad * 0.5f;

        const auto [sinB, cosB] = za::sinCosLookup(za::positiveRemainder(bisector, za::tau));

        bodyCentroid = {center.x + d * cosB, center.y + d * sinB};
        bodyArea     = 0.5f * diffSq * absSweep;
    }

    // ---- Head: triangle formed by tip, outer barb, inner barb at the end of the curve. ----
    const float endAngle   = startRad + sweepRad;
    const float pathRadius = (outerRadius + innerRadius) * 0.5f;

    const auto [endSin, endCos] = za::sinCosLookup(za::positiveRemainder(endAngle, za::tau));

    const Vec2f attach    = {center.x + pathRadius * endCos, center.y + pathRadius * endSin};
    const Vec2f tangent   = {-endSin, endCos};
    const Vec2f radialOut = {endCos, endSin};

    const Vec2f tip       = attach + tangent * (headLength * sweepSign);
    const Vec2f outerBarb = attach + radialOut * (headWidth * 0.5f);
    const Vec2f innerBarb = attach - radialOut * (headWidth * 0.5f);

    const Vec2f headCentroid{(tip.x + outerBarb.x + innerBarb.x) * (1.f / 3.f),
                             (tip.y + outerBarb.y + innerBarb.y) * (1.f / 3.f)};
    const float headArea = 0.5f * headLength * headWidth;

    // ---- Combine. ----
    const float totalArea = bodyArea + headArea;

    if (totalArea <= 0.f) [[unlikely]]
        return center;

    return {(bodyArea * bodyCentroid.x + headArea * headCentroid.x) / totalArea,
            (bodyArea * bodyCentroid.y + headArea * headCentroid.y) / totalArea};
}

} // namespace za
