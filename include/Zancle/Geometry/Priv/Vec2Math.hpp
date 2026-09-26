#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Math/Atan2.hpp"
#include "Zancle/Math/Cos.hpp"
#include "Zancle/Math/Floor.hpp"
#include "Zancle/Math/Sin.hpp"
#include "Zancle/Math/Sqrt.hpp"

#include "Zancle/Trait/IsFloatingPoint.hpp"

#include "Zancle/Base/AssertAndAssume.hpp"


namespace za
{
////////////////////////////////////////////////////////////
template <typename T>
constexpr Vec2<T> Vec2<T>::fromAngle(const T r, const Angle phi)
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    return {r * static_cast<T>(za::cos(phi.asRadians())), r * static_cast<T>(za::sin(phi.asRadians()))};
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr T Vec2<T>::length() const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    // don't use `std::hypot` because of slow performance
    return za::sqrt(x * x + y * y);
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Vec2<T> Vec2<T>::normalized() const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    ZA_ASSERT_AND_ASSUME((x != T{0} || y != T{0}) && "Vec2::normalized() cannot normalize a zero vec2");

    const T len    = length();
    const T invLen = T{1} / len;

    return {x * invLen, y * invLen};
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Angle Vec2<T>::angleTo(const Vec2 rhs) const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    ZA_ASSERT_AND_ASSUME((x != T{0} || y != T{0}) && "cannot calculate angle to a zero vec2");
    ZA_ASSERT_AND_ASSUME((rhs.x != T{0} || rhs.y != T{0}) && "cannot calculate angle to a zero vec2");

    return radians(static_cast<float>(za::atan2(cross(rhs), dot(rhs))));
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Angle Vec2<T>::angle() const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    ZA_ASSERT_AND_ASSUME((x != T{0} || y != T{0}) && "cannot calculate angle from a zero vec2");

    return radians(static_cast<float>(za::atan2(y, x)));
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Vec2<T> Vec2<T>::rotatedBy(const Angle phi) const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    // No zero vec2 assert, because rotating a zero vec2 is well-defined (yields always itself)
    const T cos = za::cos(static_cast<T>(phi.asRadians()));
    const T sin = za::sin(static_cast<T>(phi.asRadians()));

    // Don't manipulate x and y separately, otherwise they're overwritten too early
    return Vec2<T>(cos * x - sin * y, sin * x + cos * y);
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Vec2<T> Vec2<T>::movedTowards(const T r, const Angle phi) const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    return *this + Vec2<T>::fromAngle(r, phi);
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Vec2<T> Vec2<T>::projectedOnto(const Vec2 axis) const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    ZA_ASSERT_AND_ASSUME((axis.x != T{0} || axis.y != T{0}) && "cannot project onto a zero vec2");

    return dot(axis) / axis.lengthSquared() * axis;
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Vec2<T> Vec2<T>::clampMaxLength(const T maxLength) const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    ZA_ASSERT_AND_ASSUME(maxLength >= T{0} && "non-negative maxLength required");

    const T currentLengthSquared = lengthSquared();

    if (currentLengthSquared <= maxLength * maxLength)
        return *this;

    return *this * (maxLength / za::sqrt(currentLengthSquared));
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Vec2<T> Vec2<T>::clampMaxLengthSquared(const T maxLengthSquared) const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    ZA_ASSERT_AND_ASSUME(maxLengthSquared >= T{0} && "non-negative maxLengthSquared required");

    const T currentLengthSquared = lengthSquared();

    if (currentLengthSquared <= maxLengthSquared)
        return *this;

    return *this * za::sqrt(maxLengthSquared / currentLengthSquared);
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Vec2<T> Vec2<T>::clampMinLength(const T minLength) const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    ZA_ASSERT_AND_ASSUME(minLength >= T{0} && "non-negative minLength required");

    const T currentLengthSquared = lengthSquared();

    ZA_ASSERT_AND_ASSUME(
        (currentLengthSquared != T{0} || minLength == T{0}) && "cannot clamp zero vec2 to a positive minimum length");

    if (currentLengthSquared >= minLength * minLength)
        return *this;

    return *this * (minLength / za::sqrt(currentLengthSquared));
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Vec2<T> Vec2<T>::clampMinLengthSquared(const T minLengthSquared) const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    ZA_ASSERT_AND_ASSUME(minLengthSquared >= T{0} && "non-negative minLengthSquared required");

    const T currentLengthSquared = lengthSquared();

    ZA_ASSERT_AND_ASSUME((currentLengthSquared != T{0} || minLengthSquared == T{0}) &&
                         "cannot clamp zero vec2 to a positive minimum squared length");

    if (currentLengthSquared >= minLengthSquared)
        return *this;

    return *this * za::sqrt(minLengthSquared / currentLengthSquared);
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Vec2<T> Vec2<T>::clampLength(const T minLength, const T maxLength) const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    ZA_ASSERT_AND_ASSUME(minLength >= T{0} && minLength <= maxLength && "0 <= minLength <= maxLength required");

    const T currentLengthSquared = lengthSquared();

    ZA_ASSERT_AND_ASSUME(
        (currentLengthSquared != T{0} || minLength == T{0}) && "cannot clamp zero vec2 to a positive minimum length");

    const T minLengthSquared = minLength * minLength;
    const T maxLengthSquared = maxLength * maxLength;

    if (currentLengthSquared >= minLengthSquared && currentLengthSquared <= maxLengthSquared)
        return *this;

    // If length is zero we cannot scale, but previous check/assert handles this unless minLength is 0.
    // If minLength is 0 and currentLength is 0, we fall through. Clamping 0 to [0, max] should result in 0.
    if (currentLengthSquared == T{0})
        return *this;

    const T currentLength = za::sqrt(currentLengthSquared);

    if (currentLength < minLength) // We know currentLength > 0 here
        return *this * (minLength / currentLength);

    // Must be currentLength > maxLength
    return *this * (maxLength / currentLength);
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Vec2<T> Vec2<T>::clampLengthSquared(const T minLengthSquared, const T maxLengthSquared) const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    ZA_ASSERT_AND_ASSUME(minLengthSquared >= T{0} && minLengthSquared <= maxLengthSquared &&
                         "0 <= minLengthSquared <= maxLengthSquared required");

    const T currentLengthSquared = lengthSquared();

    ZA_ASSERT_AND_ASSUME((currentLengthSquared != T{0} || minLengthSquared == T{0}) &&
                         "cannot clamp zero vec2 to a positive minimum squared length");

    if (currentLengthSquared >= minLengthSquared && currentLengthSquared <= maxLengthSquared)
        return *this;

    // If length is zero we cannot scale, but previous check/assert handles this unless minLengthSquared is 0.
    // If minLengthSquared is 0 and currentLengthSquared is 0, we fall through. Clamping 0 to [0, max^2] results in 0.
    if (currentLengthSquared == T{0})
        return *this;

    if (currentLengthSquared < minLengthSquared) // We know currentLengthSquared > 0 here
        return *this * za::sqrt(minLengthSquared / currentLengthSquared);

    // Must be currentLengthSquared > maxLengthSquared
    return *this * za::sqrt(maxLengthSquared / currentLengthSquared);
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr Vec2<T> Vec2<T>::componentWiseFloor() const
{
    static_assert(ZA_IS_FLOATING_POINT(T));

    return {za::floor(x), za::floor(y)};
}

} // namespace za
