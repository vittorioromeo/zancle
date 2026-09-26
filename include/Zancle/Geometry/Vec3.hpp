#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Math/Sqrt.hpp"

#include "Zancle/Trait/IsFloatingPoint.hpp"

#include "Zancle/Base/AssertAndAssume.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Utility template class for manipulating
///        3-dimensional vectors
///
////////////////////////////////////////////////////////////
template <typename T>
struct [[nodiscard]] Vec3
{
    ////////////////////////////////////////////////////////////
    /// \brief Length of the vec3 (floating-point); prefer `lengthSquared()` for comparisons
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr T length() const
    {
        static_assert(ZA_IS_FLOATING_POINT(T), "only supported for floating point types");

        // don't use `std::hypot` because of slow performance
        return za::sqrt(x * x + y * y + z * z);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Squared length; cheaper than `length()` and suitable for comparisons
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr T lengthSquared() const
    {
        return x * x + y * y + z * z;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Vector of length 1 with the same direction (floating-point)
    ///
    /// \pre `*this` is not the zero vec3.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr Vec3 normalized() const
    {
        static_assert(ZA_IS_FLOATING_POINT(T), "only supported for floating point types");

        ZA_ASSERT_AND_ASSUME((x != T{0} || y != T{0} || z != T{0}) && "cannot normalize a zero vec3");

        const T invLen = T{1} / length();
        return Vec3<T>(x * invLen, y * invLen, z * invLen);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Dot product
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr T dot(const Vec3 rhs) const
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Cross product
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr Vec3 cross(const Vec3 rhs) const
    {
        return Vec3<T>((y * rhs.z) - (z * rhs.y), (z * rhs.x) - (x * rhs.z), (x * rhs.y) - (y * rhs.x));
    }


    ////////////////////////////////////////////////////////////
    /// \brief Component-wise multiplication: `(x*rhs.x, y*rhs.y, z*rhs.z)` (Hadamard product)
    ///
    /// Most useful for non-uniform scaling.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr Vec3 componentWiseMul(const Vec3 rhs) const
    {
        return Vec3<T>(x * rhs.x, y * rhs.y, z * rhs.z);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Component-wise division: `(x/rhs.x, y/rhs.y, z/rhs.z)`
    ///
    /// \pre No component of `rhs` is zero.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr Vec3 componentWiseDiv(const Vec3 rhs) const
    {
        ZA_ASSERT_AND_ASSUME(rhs.x != 0 && "cannot divide by 0 (x coordinate)");
        ZA_ASSERT_AND_ASSUME(rhs.y != 0 && "cannot divide by 0 (y coordinate)");
        ZA_ASSERT_AND_ASSUME(rhs.z != 0 && "cannot divide by 0 (z coordinate)");

        return Vec3<T>(x / rhs.x, y / rhs.y, z / rhs.z);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Convert to another `Vec3<U>` (`OtherVec3` must be a `Vec3<...>` type)
    ///
    ////////////////////////////////////////////////////////////
    template <typename OtherVec3>
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr OtherVec3 to() const
    {
        using ValueType = decltype(OtherVec3{}.x);
        return Vec3<ValueType>{static_cast<ValueType>(x), static_cast<ValueType>(y), static_cast<ValueType>(z)};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Strict member-wise equality
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr bool operator==(const Vec3<T>& rhs) const = default;


    ////////////////////////////////////////////////////////////
    /// \brief Member-wise negation
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] friend constexpr Vec3 operator-(const Vec3 rhs)
    {
        return Vec3(-rhs.x, -rhs.y, -rhs.z);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Member-wise addition assignment
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] friend constexpr Vec3& operator+=(Vec3& lhs, const Vec3 rhs)
    {
        lhs.x += rhs.x;
        lhs.y += rhs.y;
        lhs.z += rhs.z;

        return lhs;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Member-wise subtraction assignment
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] friend constexpr Vec3& operator-=(Vec3& lhs, const Vec3 rhs)
    {
        lhs.x -= rhs.x;
        lhs.y -= rhs.y;
        lhs.z -= rhs.z;

        return lhs;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Member-wise addition of two vec3s
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] friend constexpr Vec3 operator+(const Vec3 lhs, const Vec3 rhs)
    {
        return Vec3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Member-wise subtraction of two vec3s
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] friend constexpr Vec3 operator-(const Vec3 lhs, const Vec3 rhs)
    {
        return Vec3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Member-wise multiplication of a vec3 by a scalar
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] friend constexpr Vec3 operator*(const Vec3 lhs, const T rhs)
    {
        return Vec3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Member-wise multiplication of a scalar by a vec3
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] friend constexpr Vec3 operator*(const T lhs, const Vec3 rhs)
    {
        return Vec3(rhs.x * lhs, rhs.y * lhs, rhs.z * lhs);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Member-wise multiplication assignment by a scalar
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] friend constexpr Vec3& operator*=(Vec3& lhs, const T rhs)
    {
        lhs.x *= rhs;
        lhs.y *= rhs;
        lhs.z *= rhs;

        return lhs;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Member-wise division of a vec3 by a scalar (asserts `rhs != 0`)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] friend constexpr Vec3 operator/(const Vec3 lhs, const T rhs)
    {
        ZA_ASSERT_AND_ASSUME(rhs != 0 && "cannot divide by 0");

        return Vec3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Member-wise division assignment by a scalar (asserts `rhs != 0`)
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] friend constexpr Vec3& operator/=(Vec3& lhs, const T rhs)
    {
        ZA_ASSERT_AND_ASSUME(rhs != 0 && "cannot divide by 0");

        lhs.x /= rhs;
        lhs.y /= rhs;
        lhs.z /= rhs;

        return lhs;
    }


    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    T x{}; //!< X coordinate of the vec3
    T y{}; //!< Y coordinate of the vec3
    T z{}; //!< Z coordinate of the vec3
};

// Aliases for the most common types
using Vec3i  = Vec3<int>;
using Vec3u  = Vec3<unsigned int>;
using Vec3f  = Vec3<float>;
using Vec3uz = Vec3<za::SizeT>;

} // namespace za


////////////////////////////////////////////////////////////
// Explicit instantiation declarations
////////////////////////////////////////////////////////////
extern template struct za::Vec3<float>;
extern template struct za::Vec3<double>;
extern template struct za::Vec3<long double>;
extern template struct za::Vec3<bool>;
extern template struct za::Vec3<int>;
extern template struct za::Vec3<unsigned int>;
extern template struct za::Vec3<za::SizeT>;


////////////////////////////////////////////////////////////
/// \class za::Vec3
/// \ingroup system
///
/// `za::Vec3` is a simple class that defines a mathematical
/// vector with three coordinates (x, y and z). It can be used to
/// represent anything that has three dimensions: a size, a point,
/// a velocity, etc.
///
/// The template parameter T is the type of the coordinates. It
/// can be any type that supports arithmetic operations (+, -, /, *)
/// and comparisons (==, !=), for example int or float.
/// Note that some operations are only meaningful for vectors where T is
/// a floating point type (e.g. float or double), often because
/// results cannot be represented accurately with integers.
/// The method documentation mentions "(floating-point)" in those cases.
///
/// You generally don't have to care about the templated form (`za::Vec3<T>`),
/// the most common specializations have special type aliases:
/// \li `za::Vec3<float>` is `za::Vec3f`
/// \li `za::Vec3<int>` is `za::Vec3i`
/// \li `za::Vec3<unsigned int>` is `za::Vec3u`
/// \li `za::Vec3<za::SizeT>` is `za::Vec3uz`
///
/// The `za::Vec3` class has a small and simple interface, its x, y and z members
/// can be accessed directly (there are no accessors like `setX()`, `getX()`).
///
/// Usage example:
/// \code
/// za::Vec3f v(16.5f, 24.f, -3.2f);
/// v.x = 18.2f;
/// float y = v.y;
///
/// za::Vec3f w = v * 5.f;
/// za::Vec3f u;
/// u = v + w;
///
/// float s = v.dot(w);
/// za::Vec3f t = v.cross(w);
///
/// bool different = (v != u);
/// \endcode
///
/// Note: for 2-dimensional vectors, see `za::Vec2`.
///
////////////////////////////////////////////////////////////
