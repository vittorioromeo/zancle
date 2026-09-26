#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/MinMaxMacros.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Compute the intersection of two rectangles, handling negative sizes correctly
///
/// Prefer `Rect2::intersects` if only a yes/no answer is needed.
///
/// \return Intersection rectangle if they overlap, `za::nullOpt` otherwise
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard, gnu::always_inline, gnu::pure]] inline constexpr za::Optional<Rect2<T>> findIntersection(
    const Rect2<T>& rect0,
    const Rect2<T>& rect1)
{
    // Rectangles with negative dimensions are allowed, `getRectBounds` normalizes them
    const auto r0 = priv::getRectBounds(rect0.position, rect0.size);
    const auto r1 = priv::getRectBounds(rect1.position, rect1.size);

    // Compute the intersection boundaries for the X axis
    const T interLeft  = ZA_MAX(r0.minX, r1.minX);
    const T interRight = ZA_MIN(r0.maxX, r1.maxX);

    // Early exit if no overlap on X axis
    if (interLeft >= interRight)
        return za::nullOpt;

    // Compute the intersection boundaries for the Y axis
    const T interTop    = ZA_MAX(r0.minY, r1.minY);
    const T interBottom = ZA_MIN(r0.maxY, r1.maxY);

    // Check for overlap on Y axis
    if (interTop >= interBottom)
        return za::nullOpt;

    // Intersection found
    return za::makeOptional<Rect2<T>>(Vec2<T>{interLeft, interTop}, Vec2<T>{interRight - interLeft, interBottom - interTop});
}

} // namespace za


////////////////////////////////////////////////////////////
/// \fn `za::findIntersection(const Rect2<T>&, const Rect2<T>&)`
/// \ingroup system
///
/// Checks if two rectangles overlap and, if they do, returns the
/// rectangle representing their intersection.
/// Handles rectangles with negative sizes correctly.
///
////////////////////////////////////////////////////////////
