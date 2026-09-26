#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/Priv/ShapeMacros.hpp"
#include "Zancle/Graphics/Priv/TransformableMacros.hpp"

#include "Zancle/Mixin/GlobalAnchorPointMixin.hpp"
#include "Zancle/Mixin/LocalAnchorPointMixin.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Base/RequireDesignatedInitializers.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Data required to draw a heart shape
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_GRAPHICS_API HeartShapeData : LocalAnchorPointMixin, GlobalAnchorPointMixin
{
    ZA_REQUIRE_DESIGNATED_INITIALIZERS;

    ZA_PRIV_DEFINE_SETTINGS_DATA_MEMBERS_TRANSFORMABLE;
    ZA_PRIV_DEFINE_SETTINGS_DATA_MEMBERS_SHAPE;

    Vec2f        size{};          //!< Overall bounding size of the heart (width x height)
    unsigned int pointCount{64u}; //!< Number of points composing the heart contour (higher = smoother)


    ////////////////////////////////////////////////////////////
    /// \brief `true` when the heart would actually render anything.
    ///
    /// A heart with non-positive `size.x` / `size.y`, or
    /// `pointCount < 4`, produces no geometry and is treated as an
    /// empty shape. (`ShapeUtils::computeHeartPoint` requires at
    /// least 4 samples.)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr bool hasVisibleGeometry() const noexcept
    {
        return size.x > 0.f && size.y > 0.f && pointCount >= 4u;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Tight local-space AABB of the heart.
    ///
    /// Returns an empty bbox when `hasVisibleGeometry()` is `false`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Rect2f getLocalBounds() const noexcept
    {
        if (!hasVisibleGeometry()) [[unlikely]]
            return {};

        return {{0.f, 0.f}, size};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Tight world-space AABB of the heart.
    ///
    /// Tight up to tessellation: the sampled perimeter points are
    /// folded individually through the shape transform.
    ///
    /// \par Cost
    /// **Vertex iteration, O(pointCount)** -- transforms `pointCount`
    /// sampled heart-curve points and folds them into an AABB.
    /// Each iteration performs one trig lookup, several multiplies
    /// for the parametric formula, and one point transform. Cost
    /// grows linearly with `pointCount`; the default `pointCount = 64`
    /// is the dominant per-call cost for this shape.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] Rect2f getGlobalBounds() const noexcept;


    ////////////////////////////////////////////////////////////
    /// \brief Local-space geometric centroid of the heart.
    ///
    /// The heart's parametric `sin^3` curve has no pleasant closed
    /// form for its centroid, so this is computed numerically by
    /// applying the polygon shoelace/first-moment formula to the
    /// same sampled perimeter used for rendering.
    ///
    /// \par Cost
    /// **Vertex iteration, O(pointCount)** -- samples `pointCount`
    /// heart-curve points and folds them via two-pass shoelace to
    /// compute the signed area and first moments. Each iteration
    /// performs one trig lookup plus several multiplies. Cost grows
    /// linearly with `pointCount`; the default `pointCount = 64` is
    /// the dominant per-call cost for this shape.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] Vec2f getCentroid() const noexcept;
};

} // namespace za
