#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/Priv/ShapeMacros.hpp"
#include "Zancle/Graphics/Priv/TransformableMacros.hpp"
#include "Zancle/Graphics/TransformableMixin.hpp"
#include "Zancle/Graphics/Vertex.hpp"
#include "Zancle/Graphics/VertexSpan.hpp"
#include "Zancle/Graphics/VertexUtils.hpp"

#include "Zancle/Mixin/GlobalAnchorPointMixin.hpp"
#include "Zancle/Mixin/LocalAnchorPointMixin.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class CPUDrawableBatch;
class RenderTarget;
class Texture;
struct Color;
struct RenderStates;
} // namespace za


namespace za::priv
{
template <typename TStorage>
class DrawableBatchImpl;
}

namespace za
{
////////////////////////////////////////////////////////////
/// \brief Base class for textured shapes with outline
///
////////////////////////////////////////////////////////////
class ZA_GRAPHICS_API Shape : public TransformableMixin, public GlobalAnchorPointMixin, public LocalAnchorPointMixin
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Plain-data initializer for `za::Shape`
    ///
    /// Bundles the standard transformable members (`position`,
    /// `scale`, `origin`, `rotation`) with the standard shape
    /// appearance members (`textureRect`, `outlineTextureRect`,
    /// `fillColor`, `outlineColor`, `outlineThickness`,
    /// `miterLimit`). Designed to be created with C++20
    /// designated initializers.
    ///
    ////////////////////////////////////////////////////////////
    struct [[nodiscard]] Data
    {
        ZA_PRIV_DEFINE_SETTINGS_DATA_MEMBERS_TRANSFORMABLE;
        ZA_PRIV_DEFINE_SETTINGS_DATA_MEMBERS_SHAPE;
    };

    ////////////////////////////////////////////////////////////
    /// \brief Construct a shape from a `Data` initializer
    ///
    /// The shape's transform and appearance are taken from
    /// `data`. Concrete subclasses (`za::CircleShape`,
    /// `za::RectangleShape`, `za::ConvexShape`, ...) extend the
    /// `Data` struct with their own geometry parameters.
    ///
    /// \param data Combined transformable and appearance settings
    ///
    ////////////////////////////////////////////////////////////
    explicit Shape(const Data& data);

    ////////////////////////////////////////////////////////////
    /// \brief Set the sub-rectangle of the texture that the shape will display
    ///
    /// The texture rect is useful when you don't want to display
    /// the whole texture, but rather a part of it.
    /// By default, the texture rect covers the entire texture.
    ///
    /// \param rect Rectangle defining the region of the texture to display
    ///
    /// \see `getTextureRect`
    ///
    ////////////////////////////////////////////////////////////
    void setTextureRect(const Rect2f& rect);

    ////////////////////////////////////////////////////////////
    /// \brief Set the sub-rectangle of the texture that the shape outline will display
    ///
    /// The texture rect is useful when you don't want to display
    /// the whole texture, but rather a part of it.
    /// By default, the texture rect covers the entire texture.
    ///
    /// \param rect Rectangle defining the region of the texture to display
    ///
    /// \see getOutlineTextureRect
    ///
    ////////////////////////////////////////////////////////////
    void setOutlineTextureRect(const Rect2f& rect);

    ////////////////////////////////////////////////////////////
    /// \brief Set the fill color of the shape
    ///
    /// This color is modulated (multiplied) with the shape's
    /// texture if any. It can be used to colorize the shape,
    /// or change its global opacity.
    /// You can use `za::Color::Transparent` to make the inside of
    /// the shape transparent, and have the outline alone.
    /// By default, the shape's fill color is opaque white.
    ///
    /// \param color New color of the shape
    ///
    /// \see `getFillColor`, `setOutlineColor`
    ///
    ////////////////////////////////////////////////////////////
    void setFillColor(Color color);

    ////////////////////////////////////////////////////////////
    /// \brief Set the outline color of the shape
    ///
    /// By default, the shape's outline color is opaque white.
    ///
    /// \param color New outline color of the shape
    ///
    /// \see `getOutlineColor`, `setFillColor`
    ///
    ////////////////////////////////////////////////////////////
    void setOutlineColor(Color color);

    ////////////////////////////////////////////////////////////
    /// \brief Set the thickness of the shape's outline
    ///
    /// Note that negative values are allowed (so that the outline
    /// expands towards the center of the shape), and using zero
    /// disables the outline.
    /// By default, the outline thickness is 0.
    ///
    /// The outline is generated in the shape's **local** coordinate
    /// system: its thickness scales with the shape's transform, and
    /// a mirroring (negative) scale cannot flip the side it grows
    /// on.
    ///
    /// Which side the outline grows on follows the winding of the
    /// points: built-in shapes emit their perimeter clockwise in
    /// screen space (y-down), so positive thickness draws the
    /// outline on top of the fill; custom counter-clockwise
    /// polygons grow outward instead.
    ///
    /// \param thickness New outline thickness
    ///
    /// \see `getOutlineThickness`
    ///
    ////////////////////////////////////////////////////////////
    void setOutlineThickness(float thickness);

    ////////////////////////////////////////////////////////////
    /// \brief Set the limit on the ratio between miter length and outline thickness
    ///
    /// Outline segments around each shape corner are joined either
    /// with a miter or a bevel join.
    /// - A miter join is formed by extending outline segments until
    ///   they intersect. The distance between the point of
    ///   intersection and the shape's corner is the miter length.
    /// - A bevel join is formed by connecting outline segments with
    ///   a straight line perpendicular to the corner's bissector.
    ///
    /// The miter limit is used to determine whether ouline segments
    /// around a corner are joined with a bevel or a miter.
    /// When the ratio between the miter length and outline thickness
    /// exceeds the miter limit, a bevel is used instead of a miter.
    ///
    /// The miter limit is linked to the maximum inner angle of a
    /// corner below which a bevel is used by the following formula:
    ///
    /// miterLimit = 1 / sin(angle / 2)
    ///
    /// The miter limit must be greater than or equal to 1.
    /// By default, the miter limit is 4.
    ///
    /// \param miterLimit New miter limit
    ///
    /// \see getMiterLimit
    ///
    ////////////////////////////////////////////////////////////
    void setMiterLimit(float miterLimit);

    ////////////////////////////////////////////////////////////
    /// \brief Get the sub-rectangle of the texture displayed by the shape
    ///
    /// \return Texture rectangle of the shape
    ///
    /// \see `setTextureRect`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Rect2f& getTextureRect() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the sub-rectangle of the texture displayed by the shape outline
    ///
    /// \return Texture rectangle of the shape outline
    ///
    /// \see setTextureRect
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Rect2f& getOutlineTextureRect() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the fill color of the shape
    ///
    /// \return Fill color of the shape
    ///
    /// \see `setFillColor`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Color getFillColor() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the outline color of the shape
    ///
    /// \return Outline color of the shape
    ///
    /// \see `setOutlineColor`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Color getOutlineColor() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the outline thickness of the shape
    ///
    /// \return Outline thickness of the shape
    ///
    /// \see `setOutlineThickness`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getOutlineThickness() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the limit on the ratio between miter length and outline thickness
    ///
    /// \return Limit on the ratio between miter length and outline thickness
    ///
    /// \see setMiterLimit
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getMiterLimit() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the local bounding rectangle of the entity
    ///
    /// The returned rectangle is in local coordinates, which means
    /// that it ignores the transformations (translation, rotation,
    /// scale, etc...) that are applied to the entity.
    /// In other words, this function returns the bounds of the
    /// entity in the entity's coordinate system.
    ///
    /// \return Local bounding rectangle of the entity
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Rect2f& getLocalBounds() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the global (non-minimal) bounding rectangle of the entity
    ///
    /// The returned rectangle is in global coordinates, which means
    /// that it takes into account the transformations (translation,
    /// rotation, scale, etc...) that are applied to the entity.
    /// In other words, this function returns the bounds of the
    /// shape in the global 2D world's coordinate system.
    ///
    /// This function does not necessarily return the _minimal_
    /// bounding rectangle. It merely ensures that the returned
    /// rectangle covers all the vertices (but possibly more).
    /// This allows for a fast approximation of the bounds as a
    /// first check; you may want to use more precise checks
    /// on top of that.
    ///
    /// \return Global bounding rectangle of the entity
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Rect2f getGlobalBounds() const;

    ////////////////////////////////////////////////////////////
    /// \brief Read-only view of the shape's fill vertices
    ///
    /// Returns the contiguous range of vertices that make up the
    /// filled interior of the shape, in the order expected by the
    /// renderer. Useful for custom drawable batches that want to
    /// consume the shape's geometry directly.
    ///
    /// \return Span over the fill vertices (invalidated if the shape changes)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] ConstVertexSpan getFillVertices() const
    {
        return {m_vertices.data(), m_verticesEndIndex};
    }

    ////////////////////////////////////////////////////////////
    /// \brief Read-only view of the shape's outline vertices
    ///
    /// Returns the contiguous range of vertices that make up the
    /// outline of the shape (empty if `outlineThickness == 0`).
    ///
    /// \return Span over the outline vertices (invalidated if the shape changes)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] ConstVertexSpan getOutlineVertices() const
    {
        return {m_vertices.data() + m_verticesEndIndex, m_vertices.size() - m_verticesEndIndex};
    }

protected:
    ////////////////////////////////////////////////////////////
    /// \brief Recompute the internal geometry of the shape
    ///
    /// This function must be called by the derived class every time
    /// the shape's points change (i.e. the result of either
    /// getPointCount or getPoint is different).
    ///
    ////////////////////////////////////////////////////////////
    void update(const za::Vec2f* points, za::SizeT pointCount); // TODO P1: make public?

    ////////////////////////////////////////////////////////////
    /// \brief Recompute the internal geometry of the shape
    ///
    /// This function must be called by the derived class every time
    /// the shape's points change (i.e. the result of either
    /// getPointCount or getPoint is different).
    ///
    ////////////////////////////////////////////////////////////
    void updateFromFunc(auto&& getPointFunc, const za::SizeT pointCount) // TODO P1: make public?
    {
        if (pointCount < 3u)
        {
            m_vertices.clear();
            m_verticesEndIndex = 0;

            m_insideBounds = {};
            m_bounds       = {};

            return;
        }

        m_vertices.resize(pointCount + 2u); // +2 for center and repeated first point
        m_verticesEndIndex = pointCount + 2u;

        for (za::SizeT i = 0u; i < pointCount; ++i)
            m_vertices[i + 1u].position = getPointFunc(i);

        m_vertices[pointCount + 1u].position = m_vertices[1].position; // repeated first point

        // Update the bounding rectangle
        m_insideBounds = VertexUtils::getVertexRangeBounds(m_vertices.data() + 1u, m_verticesEndIndex - 1u); // skip center

        // Compute the center and make it the first vertex
        m_vertices[0].position = m_insideBounds.getCenter();

        // Updates
        updateFillColors();
        updateTexCoords();
        updateOutline();
        updateOutlineTexCoords();
    }

private:
    friend RenderTarget;

    template <typename TStorage>
    friend class priv::DrawableBatchImpl;

    ////////////////////////////////////////////////////////////
    /// \brief Update the fill vertices' color
    ///
    ////////////////////////////////////////////////////////////
    void updateFillColors();

    ////////////////////////////////////////////////////////////
    /// \brief Update the fill vertices' texture coordinates
    ///
    ////////////////////////////////////////////////////////////
    void updateTexCoords();

    ////////////////////////////////////////////////////////////
    /// \brief Update the outline vertices' texture coordinates
    ///
    ////////////////////////////////////////////////////////////
    void updateOutlineTexCoords();

    ////////////////////////////////////////////////////////////
    /// \brief Update the outline vertices' position
    ///
    ////////////////////////////////////////////////////////////
    void updateOutline();

    ////////////////////////////////////////////////////////////
    /// \brief Update the outline vertices' color
    ///
    ////////////////////////////////////////////////////////////
    void updateOutlineColors();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    /* Ordered to minimize padding */
    za::Vector<Vertex> m_vertices;              //!< Vertex array containing the fill and outline geometry
    za::SizeT          m_verticesEndIndex = 0u; //!< Index where the fill vertices end and outline vertices begin

    Rect2f m_textureRect{};        //!< Area of the source texture to display for the fill
    Rect2f m_outlineTextureRect{}; //!< Area of the source texture to display for the outline
    Rect2f m_insideBounds;         //!< Bounding rectangle of the inside (fill)
    Rect2f m_bounds;               //!< Bounding rectangle of the whole shape (outline + fill)

    float m_outlineThickness{}; //!< Thickness of the shape's outline
    float m_miterLimit{4.f};    //!< Limit on the ratio between miter length and outline thickness

    Color m_fillColor{Color::White};    //!< Fill color
    Color m_outlineColor{Color::White}; //!< Outline color

public:
    ZA_DEFINE_TRANSFORMABLE_DATA_MEMBERS;
};

} // namespace za


namespace za::priv
{
////////////////////////////////////////////////////////////
template <typename TData>
[[nodiscard]] inline Shape::Data toShapeData(const TData& data)
{
    return {
        .position           = data.position,
        .scale              = data.scale,
        .origin             = data.origin,
        .rotation           = data.rotation,
        .textureRect        = data.textureRect,
        .outlineTextureRect = data.outlineTextureRect,
        .fillColor          = data.fillColor,
        .outlineColor       = data.outlineColor,
        .outlineThickness   = data.outlineThickness,
        .miterLimit         = data.miterLimit,
    };
}

} // namespace za::priv


////////////////////////////////////////////////////////////
/// \class za::Shape
/// \ingroup graphics
///
/// `za::Shape` is the **non-polymorphic** base class for all
/// built-in textured shapes with an optional outline. Concrete
/// shapes (`za::CircleShape`, `za::RectangleShape`,
/// `za::ConvexShape`, ...) compose with `Shape` and provide their
/// own geometry.
///
/// Unlike upstream SFML, `za::Shape` is **not** a virtual base.
/// Zancle deliberately removes the `za::Drawable` interface and
/// the `vector<unique_ptr<Drawable>>` pattern in favor of flat
/// concrete types. Each shape stores its tessellated vertex data
/// directly and is drawn through the dedicated `RenderTarget::draw`
/// overload, with the texture (if any) supplied via
/// `RenderStates::texture` -- the shape itself does not own a
/// texture pointer.
///
/// Every shape carries:
/// \li a texture rectangle for the fill
/// \li a texture rectangle for the outline
/// \li a fill color
/// \li an outline color
/// \li an outline thickness
/// \li a miter limit (for the outline join)
///
/// All of these are optional and can be disabled with the obvious
/// values (`za::Color::Transparent`, `0` thickness, full-texture
/// rectangles, ...).
///
/// \see `za::RectangleShape`, `za::CircleShape`, `za::ConvexShape`,
///      `za::Transformable`, `za::ShapeUtils`
///
////////////////////////////////////////////////////////////

// TODO P1: this should probably be a shapemixin
