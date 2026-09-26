#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Transform.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/Vec2.hpp"

#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/SinCosLookup.hpp"
#include "Zancle/Math/Sqrt.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Tight axis-aligned world bounds of a locally-elliptical shape.
///
/// Computes the AABB (in world space) of an ellipse centered at
/// `localCenter` with local semi-axes `localSemiAxes`, transformed
/// by `scale`, `rotation`, and translated to `position` about
/// `origin`. Scale is applied before rotation.
///
/// For a disk (`localSemiAxes.x == localSemiAxes.y`) this is
/// rotation-invariant under uniform scale. For a true ellipse,
/// it gives the tight rotated-ellipse AABB.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline Rect2f getEllipticalGlobalBounds(
    const Vec2f localCenter,
    const Vec2f localSemiAxes,
    const Vec2f position,
    const Vec2f scale,
    const Vec2f origin,
    const Angle rotation) noexcept
{
    const auto [sine, cosine] = za::sinCosLookup(rotation.asRadians());

    const float a = localSemiAxes.x * ZA_MATH_FABSF(scale.x);
    const float b = localSemiAxes.y * ZA_MATH_FABSF(scale.y);

    const float a2 = a * a;
    const float b2 = b * b;
    const float c2 = cosine * cosine;
    const float s2 = sine * sine;

    const float hw = ZA_MATH_SQRTF(a2 * c2 + b2 * s2);
    const float hh = ZA_MATH_SQRTF(a2 * s2 + b2 * c2);

    const auto  transform   = Transform::fromPositionScaleOriginSinCos(position, scale, origin, sine, cosine);
    const Vec2f worldCenter = transform.transformPoint(localCenter);

    return {{worldCenter.x - hw, worldCenter.y - hh}, {2.f * hw, 2.f * hh}};
}

} // namespace za::priv
