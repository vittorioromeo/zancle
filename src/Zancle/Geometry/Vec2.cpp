// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/Vec2.hpp"


////////////////////////////////////////////////////////////
// Explicit instantiation definitions
////////////////////////////////////////////////////////////
template struct za::Vec2<float>;
template struct za::Vec2<double>;

#define ZA_INSTANTIATE_VECTOR2_INTEGRAL_MEMBER_FUNCTIONS(type)            \
    template type           za::Vec2<type>::lengthSquared() const;        \
    template za::Vec2<type> za::Vec2<type>::perpendicular() const;        \
    template type           za::Vec2<type>::dot(Vec2) const;              \
    template type           za::Vec2<type>::cross(Vec2) const;            \
    template za::Vec2<type> za::Vec2<type>::componentWiseMul(Vec2) const; \
    template za::Vec2<type> za::Vec2<type>::componentWiseDiv(Vec2) const;

ZA_INSTANTIATE_VECTOR2_INTEGRAL_MEMBER_FUNCTIONS(int)
ZA_INSTANTIATE_VECTOR2_INTEGRAL_MEMBER_FUNCTIONS(unsigned int)
ZA_INSTANTIATE_VECTOR2_INTEGRAL_MEMBER_FUNCTIONS(::za::SizeT)
