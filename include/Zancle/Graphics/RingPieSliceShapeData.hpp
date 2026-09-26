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

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Base/RequireDesignatedInitializers.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Data required to draw a ring pie slice shape
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_GRAPHICS_API RingPieSliceShapeData : LocalAnchorPointMixin, GlobalAnchorPointMixin
{
    ZA_REQUIRE_DESIGNATED_INITIALIZERS;

    ZA_PRIV_DEFINE_SETTINGS_DATA_MEMBERS_TRANSFORMABLE;
    ZA_PRIV_DEFINE_SETTINGS_DATA_MEMBERS_SHAPE;

    float        outerRadius{0.f}; //!< Radius of the outer circle defining the boundary
    float        innerRadius{0.f}; //!< Radius of the inner circle defining the hole
    Angle        startAngle{};     //!< Starting angle of the pie slice
    Angle        sweepAngle{};     //!< Sweep angle of the pie slice
    unsigned int pointCount{30u};  //!< Number of points used to approximate the arcs (per full circle of sweep)


    ////////////////////////////////////////////////////////////
    /// \brief `true` when the ring pie slice would actually render anything.
    ///
    /// A ring pie slice with `outerRadius <= 0`, `innerRadius < 0`,
    /// `innerRadius >= outerRadius`, `pointCount < 3`, or zero
    /// `sweepAngle` produces no geometry and is treated as an empty
    /// shape. Negative sweeps are accepted and sweep in the opposite
    /// direction (matching `CurvedArrowShapeData`).
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr bool hasVisibleGeometry() const noexcept
    {
        return outerRadius > 0.f && innerRadius >= 0.f && innerRadius < outerRadius && pointCount >= 3u &&
               sweepAngle.asRadians() != 0.f;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Tight local-space AABB of the ring pie slice.
    ///
    /// Takes the swept angle into account: narrow sweeps produce
    /// correspondingly narrow bounding boxes. The inner arc and the
    /// radial connector segments are also considered, so the result
    /// is tight for any valid combination of inner/outer radii.
    ///
    /// \par Cost
    /// **Analytical, O(1)** -- folds outer + inner arc endpoints
    /// and up to four in-sweep cardinal extrema (a handful of trig
    /// lookups). Independent of `pointCount`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] Rect2f getLocalBounds() const noexcept;


    ////////////////////////////////////////////////////////////
    /// \brief Tight world-space AABB of the ring pie slice.
    ///
    /// Tight up to arc tessellation: every rendered outer- and
    /// inner-arc sample is folded through the shape transform.
    /// The analytical cardinal-extrema trick used by
    /// `getLocalBounds` can't be reused here because rotation
    /// (and non-uniform scale) move the world extrema off the
    /// local cardinals.
    ///
    /// \par Cost
    /// **Vertex iteration, O(pointCount)** -- transforms
    /// two vertices per rendered arc sample (outer arc + inner arc;
    /// the sample count is sweep-scaled from `pointCount`, matching
    /// the renderer) and folds them into an AABB. Each iteration
    /// performs one trig lookup and one point transform. Cost grows
    /// linearly with `pointCount`; the default `pointCount = 30`
    /// means 60 vertex transforms per call at a full-circle sweep.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] Rect2f getGlobalBounds() const noexcept;


    ////////////////////////////////////////////////////////////
    /// \brief Local-space geometric centroid of the ring pie slice.
    ///
    /// For an annulus sector with outer radius `R`, inner radius `r`,
    /// half-sweep `alpha = sweepAngle / 2`, the centroid lies on the
    /// bisector at distance
    /// `d = (4 * sin(alpha) * (R^3 - r^3)) / (3 * sweepAngle * (R^2 - r^2))`
    /// from the ring's geometric center `(R, R)`.
    ///
    /// \par Cost
    /// **Analytical, O(1)** -- one trig lookup for the bisector
    /// direction, one `sin` for the distance, plus a handful of
    /// multiplies and two divisions. Independent of `pointCount`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] Vec2f getCentroid() const noexcept;
};

} // namespace za


////////////////////////////////////////////////////////////
/// \struct za::RingPieSliceShapeData
/// \ingroup graphics
///
/// `za::RingPieSliceShapeData` describes a slice of a ring -- the
/// intersection of a `za::RingShapeData` and a `za::PieSliceShapeData`.
/// It is defined by an `outerRadius`, an `innerRadius`, a `startAngle`,
/// and a `sweepAngle`. `pointCount` controls the smoothness of the
/// arcs.
///
/// Like other `*ShapeData` types, it embeds the standard
/// transformable and shape appearance members and is consumed by
/// `za::ShapeUtils` and `za::CPUDrawableBatch::add` -- it is not
/// directly drawable on its own.
///
/// \see `za::Shape`, `za::RingShapeData`, `za::PieSliceShapeData`, `za::CurvedArrowShapeData`
///
////////////////////////////////////////////////////////////
