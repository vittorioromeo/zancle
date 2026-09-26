#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/CircleShapeData.hpp"
#include "Zancle/Graphics/Shape.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Base/SizeT.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Specialized shape representing a circle
///
////////////////////////////////////////////////////////////
class ZA_GRAPHICS_API CircleShape : public Shape
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Initialization data for a `za::CircleShape`
    ///
    /// This type alias points to `za::CircleShapeData`, which
    /// holds all the configurable parameters for a circle shape.
    /// It is used when constructing a `za::CircleShape`.
    ///
    /// \see za::CircleShapeData
    ////////////////////////////////////////////////////////////
    using Data = CircleShapeData;

    ////////////////////////////////////////////////////////////
    /// \brief Construct a circle shape from a `Data` initializer
    ///
    /// \param data Geometry, transform, and appearance settings
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit CircleShape(const Data& data);

    ////////////////////////////////////////////////////////////
    /// \brief Set the radius of the circle
    ///
    /// \param radius New radius of the circle
    ///
    /// \see `getRadius`
    ///
    ////////////////////////////////////////////////////////////
    void setRadius(float radius);

    ////////////////////////////////////////////////////////////
    /// \brief Get the radius of the circle
    ///
    /// \return Radius of the circle
    ///
    /// \see `setRadius`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getRadius() const;

    ////////////////////////////////////////////////////////////
    /// \brief Set the starting angle of the first generated point on the contour
    ///
    /// \param startAngle New starting angle
    ///
    /// \see `getStartAngle`
    ///
    ////////////////////////////////////////////////////////////
    void setStartAngle(Angle startAngle);

    ////////////////////////////////////////////////////////////
    /// \brief Get the starting angle of the first generated point on the contour
    ///
    /// \return Starting angle of the first generated point
    ///
    /// \see `setStartAngle`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Angle getStartAngle() const;

    ////////////////////////////////////////////////////////////
    /// \brief Set the number of points of the circle
    ///
    /// \param pointCount New number of points of the circle
    ///
    /// \see `getPointCount`
    ///
    ////////////////////////////////////////////////////////////
    void setPointCount(unsigned int pointCount);

    ////////////////////////////////////////////////////////////
    /// \brief Get the number of points of the circle
    ///
    /// \return Number of points of the circle
    ///
    /// \see `setPointCount`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getPointCount() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get a point of the circle
    ///
    /// The returned point is in local coordinates, that is,
    /// the shape's transforms (position, rotation, scale) are
    /// not taken into account.
    /// The result is undefined if `index` is out of the valid range.
    ///
    /// \param index Index of the point to get, in range [0 .. getPointCount() - 1]
    ///
    /// \return `index`-th point of the shape
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vec2f getPoint(za::SizeT index) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the geometric center of the circle
    ///
    /// The returned point is in local coordinates, that is,
    /// the shape's transforms (position, rotation, scale) are
    /// not taken into account.
    ///
    /// \return The geometric center of the shape
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vec2f getGeometricCenter() const;

private:
    ////////////////////////////////////////////////////////////
    /// \brief Recompute the circle geometry
    ///
    ////////////////////////////////////////////////////////////
    void updateCircleGeometry();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    float        m_radius;     //!< Radius of the circle
    Angle        m_startAngle; //!< Starting angle of the first generated point on the contour
    unsigned int m_pointCount; //!< Number of points composing the circle
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::CircleShape
/// \ingroup graphics
///
/// `za::CircleShape` is the concrete `za::Shape` subclass for
/// drawing a circle (or, more generally, a regular polygon
/// approximation of one). It exposes the standard transform and
/// appearance members inherited from `za::Shape` plus a `radius`
/// and `pointCount` (number of vertices used to approximate the
/// curve).
///
/// Usage example:
/// \code
/// za::CircleShape circle{{
///     .position         = {10.f, 20.f},
///     .outlineColor     = za::Color::Red,
///     .outlineThickness = 5.f,
///     .radius           = 150.f,
/// }};
///
/// window.draw(circle);
/// \endcode
///
/// Since the GPU cannot draw perfect circles, the circle is
/// approximated by `pointCount` connected triangles fanning out
/// from the center. Higher counts produce smoother circles at
/// the cost of more vertices.
///
/// `pointCount` can also be used to draw regular polygons --
/// pass a small value to get an equilateral triangle (3), a
/// square (4), a pentagon (5), a hexagon (6), and so on.
///
/// \see `za::Shape`, `za::RectangleShape`, `za::ConvexShape`,
///      `za::CircleShapeData`
///
////////////////////////////////////////////////////////////
