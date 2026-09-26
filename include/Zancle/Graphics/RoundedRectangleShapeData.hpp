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
/// \brief Data required to draw a rounded rectangle shape
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_GRAPHICS_API RoundedRectangleShapeData : LocalAnchorPointMixin, GlobalAnchorPointMixin
{
    ZA_REQUIRE_DESIGNATED_INITIALIZERS;

    ZA_PRIV_DEFINE_SETTINGS_DATA_MEMBERS_TRANSFORMABLE;
    ZA_PRIV_DEFINE_SETTINGS_DATA_MEMBERS_SHAPE;

    Vec2f size{};                       //!< Size of the rounded rectangle
    float cornerRadius{0.f};            //!< Radius of the corners of the rounded rectangle
    unsigned int cornerPointCount{16u}; //!< Number of points used to draw each corner of the rounded rectangle (must be >= 2)


    ////////////////////////////////////////////////////////////
    /// \brief `true` when the rounded rectangle would actually render anything.
    ///
    /// A rounded rectangle with non-positive `size.x` / `size.y`,
    /// negative `cornerRadius`, or `cornerPointCount < 2` produces
    /// no geometry and is treated as an empty shape.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr bool hasVisibleGeometry() const noexcept
    {
        return size.x > 0.f && size.y > 0.f && cornerRadius >= 0.f && cornerPointCount >= 2u;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Tight local-space AABB of the rounded rectangle.
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
    /// \brief Tight world-space AABB of the rounded rectangle.
    ///
    /// Tight up to corner-arc tessellation: each of the
    /// `4*cornerPointCount` corner-arc vertices is folded through
    /// the shape transform.
    ///
    /// \par Cost
    /// **Vertex iteration, O(cornerPointCount)** -- transforms
    /// `4 * cornerPointCount` corner-arc samples and folds them
    /// into an AABB. Each iteration performs one trig lookup and
    /// one point transform. Cost grows linearly with
    /// `cornerPointCount`; the default `cornerPointCount = 16`
    /// means 64 vertex transforms per call.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] Rect2f getGlobalBounds() const noexcept;


    ////////////////////////////////////////////////////////////
    /// \brief Local-space geometric centroid of the rounded rectangle.
    ///
    /// \par Cost
    /// **Analytical, O(1)** -- two multiplies. The rounded rectangle
    /// has two axes of symmetry, so the centroid coincides with
    /// `getLocalBounds().getCenter()`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f getCentroid() const noexcept
    {
        return size * 0.5f;
    }
};

} // namespace za


////////////////////////////////////////////////////////////
/// \struct za::RoundedRectangleShapeData
/// \ingroup graphics
///
/// `za::RoundedRectangleShapeData` describes an axis-aligned
/// rectangle whose corners are rounded with a quarter-circle of
/// radius `cornerRadius`. The smoothness of each rounded corner is
/// controlled by `cornerPointCount`.
///
/// Like other `*ShapeData` types, it embeds the standard
/// transformable and shape appearance members and is consumed by
/// `za::ShapeUtils` and `za::CPUDrawableBatch::add` -- it is not
/// directly drawable on its own.
///
/// \see `za::Shape`, `za::RectangleShapeData`, `za::ShapeUtils`
///
////////////////////////////////////////////////////////////
