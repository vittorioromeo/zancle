// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/CircleShape.hpp"

#include "Zancle/Graphics/Shape.hpp"
#include "Zancle/Graphics/ShapeUtils.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Math/Constants.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace za
{
////////////////////////////////////////////////////////////
CircleShape::CircleShape(const Data& data) :
    Shape(priv::toShapeData(data)),
    m_radius{data.radius},
    m_startAngle{data.startAngle},
    m_pointCount{data.pointCount}
{
    updateCircleGeometry();
}


////////////////////////////////////////////////////////////
void CircleShape::setRadius(float radius)
{
    if (radius == m_radius)
        return;

    m_radius = radius;
    updateCircleGeometry();
}


////////////////////////////////////////////////////////////
float CircleShape::getRadius() const
{
    return m_radius;
}


////////////////////////////////////////////////////////////
void CircleShape::setStartAngle(Angle startAngle)
{
    if (startAngle == m_startAngle)
        return;

    m_startAngle = startAngle;
    updateCircleGeometry();
}


////////////////////////////////////////////////////////////
Angle CircleShape::getStartAngle() const
{
    return m_startAngle;
}


////////////////////////////////////////////////////////////
void CircleShape::setPointCount(unsigned int pointCount)
{
    if (pointCount == m_pointCount)
        return;

    m_pointCount = pointCount;
    updateCircleGeometry();
}


////////////////////////////////////////////////////////////
unsigned int CircleShape::getPointCount() const
{
    return m_pointCount;
}


////////////////////////////////////////////////////////////
Vec2f CircleShape::getPoint(za::SizeT index) const
{
    ZA_ASSERT(index < m_pointCount && "Index is out of bounds");
    return ShapeUtils::computeCirclePoint(index, m_startAngle.asRadians(), m_pointCount, m_radius);
}


////////////////////////////////////////////////////////////
Vec2f CircleShape::getGeometricCenter() const
{
    return {m_radius, m_radius};
}


////////////////////////////////////////////////////////////
void CircleShape::updateCircleGeometry()
{
    const float angleStep = za::tau / static_cast<float>(m_pointCount);

    updateFromFunc([&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i) {
        return ShapeUtils::computeCirclePointFromAngleStep(i, m_startAngle.asRadians(), angleStep, m_radius);
    }, m_pointCount);
}

} // namespace za
