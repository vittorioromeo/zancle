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
/// \brief Defines the geometric properties of a curved arrow shape
///
/// `za::CurvedArrowShapeData` is used to specify the characteristics
/// of an arrow that follows a curved path, such as its path radius,
/// thickness, angular span, and arrowhead dimensions.
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_GRAPHICS_API CurvedArrowShapeData : LocalAnchorPointMixin, GlobalAnchorPointMixin
{
    ZA_REQUIRE_DESIGNATED_INITIALIZERS;

    ZA_PRIV_DEFINE_SETTINGS_DATA_MEMBERS_TRANSFORMABLE;
    ZA_PRIV_DEFINE_SETTINGS_DATA_MEMBERS_SHAPE;

    float        outerRadius{0.f};              //!< Radius of the outer circle defining the boundary
    float        innerRadius{0.f};              //!< Radius of the inner circle defining the hole
    Angle        startAngle{};                  //!< Starting angle of the curved path
    Angle        sweepAngle{za::degrees(90.f)}; //!< Angular extent of the curved path
    float        headLength{15.f};              //!< Length of the arrowhead (from base to tip)
    float        headWidth{20.f};               //!< Width of the arrowhead at its base
    unsigned int pointCount{30u}; //!< Number of points to approximate the curved body (per 90 degrees of sweep)


    ////////////////////////////////////////////////////////////
    /// \brief `true` when the curved arrow would actually render anything.
    ///
    /// A curved arrow with `outerRadius <= 0`, `innerRadius < 0`,
    /// `innerRadius >= outerRadius`, zero `sweepAngle`, negative
    /// `headLength`/`headWidth`, or `pointCount < 2` produces no
    /// geometry and is treated as an empty shape.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr bool hasVisibleGeometry() const noexcept
    {
        return outerRadius > 0.f && innerRadius >= 0.f && innerRadius < outerRadius && sweepAngle.asRadians() != 0.f &&
               headLength >= 0.f && headWidth >= 0.f && pointCount >= 2u;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Tight local-space AABB of the curved arrow.
    ///
    /// Folds both the inner/outer arc extrema (sweep-aware) and
    /// the three arrowhead vertices (tip and two barbs). The result
    /// is tight even when the arrowhead extends beyond the outer
    /// ring envelope.
    ///
    /// \par Cost
    /// **Analytical, O(1)** -- folds outer + inner arc endpoints,
    /// up to four in-sweep cardinal extrema, and the three
    /// arrowhead vertices (a handful of trig lookups). Independent
    /// of `pointCount`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] Rect2f getLocalBounds() const noexcept;


    ////////////////////////////////////////////////////////////
    /// \brief Tight world-space AABB of the curved arrow.
    ///
    /// Tight up to arc tessellation: every rendered outer- and
    /// inner-arc sample plus the three arrowhead vertices (tip and
    /// two barbs) is folded through the shape transform. The
    /// analytical cardinal-extrema trick used by `getLocalBounds`
    /// can't be reused here because rotation (and non-uniform
    /// scale) move the world extrema off the local cardinals.
    ///
    /// \par Cost
    /// **Vertex iteration, O(pointCount)** -- transforms
    /// two vertices per rendered arc sample plus the three head
    /// vertices (the sample count is sweep-scaled from `pointCount`,
    /// matching the renderer) and folds them into an AABB. Each
    /// iteration performs one trig lookup and one point transform.
    /// Cost grows linearly with `pointCount`; the default
    /// `pointCount = 30` means 63 vertex transforms per call at a
    /// 90-degree sweep.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] Rect2f getGlobalBounds() const noexcept;


    ////////////////////////////////////////////////////////////
    /// \brief Local-space geometric centroid of the curved arrow.
    ///
    /// Decomposes the shape into its ring-sector body (outer radius
    /// `outerRadius`, inner radius `innerRadius`, span `sweepAngle`)
    /// and its arrowhead triangle (`tip`, `outerBarb`, `innerBarb`)
    /// and returns the area-weighted combination of the two
    /// centroids.
    ///
    /// \par Cost
    /// **Analytical, O(1)** -- two trig lookups (for the bisector
    /// direction of the body and the attach point of the head) and
    /// a handful of multiplies/divisions. Independent of
    /// `pointCount`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] Vec2f getCentroid() const noexcept;
};

} // namespace za


////////////////////////////////////////////////////////////
/// \struct za::CurvedArrowShapeData
/// \ingroup graphics
///
/// `za::CurvedArrowShapeData` describes an arrow that bends along
/// a circular arc, useful for diagrams, UI affordances, and game
/// indicators.
///
/// Key properties:
/// - `outerRadius` -- radius of the outer edge of the arc body.
/// - `innerRadius` -- radius of the inner edge of the arc body
///   (the difference with `outerRadius` defines its thickness).
/// - `startAngle`, `sweepAngle` -- define the angular segment of
///   the arc that the arrow occupies.
/// - `headLength`, `headWidth` -- dimensions of the triangular
///   arrowhead at the end of the arc.
/// - `pointCount` -- controls the smoothness of the curved body.
///   It is interpreted as the number of points per 90 degrees of
///   sweep, so that arcs of different lengths render with comparable
///   smoothness.
///
/// Like other `*ShapeData` types, this struct also embeds the
/// standard transformable and shape appearance members.
///
/// `CurvedArrowShapeData` is consumed by `za::ShapeUtils` and by
/// `za::CPUDrawableBatch::add` -- it is not directly drawable.
///
/// Example usage:
/// \code
/// const za::CurvedArrowShapeData curvedArrowData{
///     .position    = {200.f, 200.f},
///     .fillColor   = za::Color::Red,
///     .outerRadius = 100.f,
///     .innerRadius = 50.f,
///     .startAngle  = za::degrees(0.f),
///     .sweepAngle  = za::degrees(120.f),
///     .headLength  = 20.f,
///     .headWidth   = 25.f,
///     .pointCount  = 20,
/// };
///
/// drawableBatch.add(curvedArrowData);
/// \endcode
///
/// \see `za::ArrowShapeData`, `za::RingPieSliceShapeData`, `za::ShapeUtils`, `za::DrawableBatch`
///
////////////////////////////////////////////////////////////
