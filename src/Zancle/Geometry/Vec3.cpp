// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/Vec3.hpp"


////////////////////////////////////////////////////////////
// Explicit instantiation definitions
////////////////////////////////////////////////////////////
template struct za::Vec3<float>;
template struct za::Vec3<double>;
template struct za::Vec3<long double>;

#define ZA_INSTANTIATE_VECTOR3_INTEGRAL_MEMBER_FUNCTIONS(type)            \
    template type           za::Vec3<type>::lengthSquared() const;        \
    template type           za::Vec3<type>::dot(Vec3) const;              \
    template za::Vec3<type> za::Vec3<type>::cross(Vec3) const;            \
    template za::Vec3<type> za::Vec3<type>::componentWiseMul(Vec3) const; \
    template za::Vec3<type> za::Vec3<type>::componentWiseDiv(Vec3) const;

ZA_INSTANTIATE_VECTOR3_INTEGRAL_MEMBER_FUNCTIONS(bool)
ZA_INSTANTIATE_VECTOR3_INTEGRAL_MEMBER_FUNCTIONS(int)
ZA_INSTANTIATE_VECTOR3_INTEGRAL_MEMBER_FUNCTIONS(unsigned int)
ZA_INSTANTIATE_VECTOR3_INTEGRAL_MEMBER_FUNCTIONS(::za::SizeT)
