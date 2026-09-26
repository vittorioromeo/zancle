#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Remainder.hpp"

#include "Zancle/Base/Assert.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Represents an angle value.
///
/// Prefer constructing angles via `za::radians` or `za::degrees`
/// (or the `_rad` and `_deg` literals) for readability.
///
////////////////////////////////////////////////////////////
class [[nodiscard]] Angle
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Return the angle's value in degrees
    ///
    /// \see `asRadians`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr float asDegrees() const
    {
        return radians * (180.f / za::pi);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Return the angle's value in radians
    ///
    /// \see `asDegrees`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr float asRadians() const
    {
        return radians;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Wrap to a range such that -180° <= angle < 180°
    ///
    /// Similar to a modulo operation, this returns a copy of the angle
    /// constrained to the range [-180°, 180°) == [-Pi, Pi).
    /// The resulting angle represents a rotation which is equivalent to `*this`.
    ///
    /// The name "signed" originates from the similarity to signed integers:
    /// <table>
    /// <tr>
    ///   <th></th>
    ///   <th>signed</th>
    ///   <th>unsigned</th>
    /// </tr>
    /// <tr>
    ///   <td>char</td>
    ///   <td>[-128, 128)</td>
    ///   <td>[0, 256)</td>
    /// </tr>
    /// <tr>
    ///   <td>Angle</td>
    ///   <td>[-180°, 180°)</td>
    ///   <td>[0°, 360°)</td>
    /// </tr>
    /// </table>
    ///
    /// \see `wrapUnsigned`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Angle wrapSigned() const
    {
        return Angle(za::positiveRemainder(radians + za::pi, za::tau) - za::pi);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Wrap to a range such that 0° <= angle < 360°
    ///
    /// Similar to a modulo operation, this returns a copy of the angle
    /// constrained to the range [0°, 360°) == [0, Tau) == [0, 2*Pi).
    /// The resulting angle represents a rotation which is equivalent to `*this`.
    ///
    /// The name "unsigned" originates from the similarity to unsigned integers:
    /// <table>
    /// <tr>
    ///   <th></th>
    ///   <th>signed</th>
    ///   <th>unsigned</th>
    /// </tr>
    /// <tr>
    ///   <td>char</td>
    ///   <td>[-128, 128)</td>
    ///   <td>[0, 256)</td>
    /// </tr>
    /// <tr>
    ///   <td>Angle</td>
    ///   <td>[-180°, 180°)</td>
    ///   <td>[0°, 360°)</td>
    /// </tr>
    /// </table>
    ///
    /// \see `wrapSigned`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Angle wrapUnsigned() const
    {
        // Fast path for the common case of an angle that is already wrapped (e.g. every `AutoWrapAngle`
        // assignment of a bounded value): skips the division, with bit-identical results. Non-negative
        // floats order like their bit patterns, while negative values and NaNs compare greater than `tau`,
        // so `0 <= radians < tau` is a single, predictable unsigned comparison (`-0.f` takes the slow path).
        if (__builtin_bit_cast(unsigned int, radians) < __builtin_bit_cast(unsigned int, za::tau)) [[likely]]
            return *this;

        return Angle(za::positiveRemainder(radians, za::tau));
    }


    ////////////////////////////////////////////////////////////
    /// \brief Rotate towards another angle by a maximum step.
    ///
    /// Calculates the shortest difference between `*this` and `other` (handling wrapping)
    /// and returns a new angle by rotating `*this` towards `other` by at most `speed` radians.
    /// If the shortest difference is less than or equal to `speed`, `other` (wrapped) is returned.
    /// The result is normalized to the range `[0, 2*Pi)`.
    ///
    /// \param speed Maximum rotation step in radians. Must be non-negative.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Angle rotatedTowards(const Angle other,
                                                                                                     const float speed) const
    {
        ZA_ASSERT(speed >= 0.f && "Angle::rotatedTowards requires a non-negative speed");

        float diff = za::remainder(other.radians - radians, za::tau);

        if (diff > za::pi)
            diff -= za::tau;
        else if (diff < -za::pi)
            diff += za::tau;

        if (diff <= speed && -diff <= speed)
            return other.wrapUnsigned();

        return Angle{diff > 0.f ? radians + speed : radians - speed}.wrapUnsigned();
    }


    ////////////////////////////////////////////////////////////
    // Static member data
    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(readability-identifier-naming)
    static const Angle Zero; //!< Predefined 0 degree angle value
    // NOLINTNEXTLINE(readability-identifier-naming)
    static const Angle Quarter; //!< Predefined 90 degree angle value
    // NOLINTNEXTLINE(readability-identifier-naming)
    static const Angle Half; //!< Predefined 180 degree angle value
    // NOLINTNEXTLINE(readability-identifier-naming)
    static const Angle Full; //!< Predefined 360 degree angle value


    ////////////////////////////////////////////////////////////
    /// \brief Equality comparison of two angles
    /// \note Does not automatically wrap the angle value
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr bool operator==(Angle lhs, Angle rhs) = default;


    ////////////////////////////////////////////////////////////
    /// \brief Less-than comparison of two angles
    /// \note Does not automatically wrap the angle value
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr bool operator<(const Angle lhs, const Angle rhs)
    {
        return lhs.radians < rhs.radians;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Greater-than comparison of two angles
    /// \note Does not automatically wrap the angle value
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr bool operator>(const Angle lhs, const Angle rhs)
    {
        return lhs.radians > rhs.radians;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Less-than-or-equal comparison of two angles
    /// \note Does not automatically wrap the angle value
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr bool operator<=(const Angle lhs, const Angle rhs)
    {
        return lhs.radians <= rhs.radians;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Greater-than-or-equal comparison of two angles
    /// \note Does not automatically wrap the angle value
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr bool operator>=(const Angle lhs, const Angle rhs)
    {
        return lhs.radians >= rhs.radians;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Negation of an angle (rotation in the opposite direction)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr Angle operator-(const Angle rhs)
    {
        return Angle(-rhs.radians);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Sum of two angles
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr Angle operator+(const Angle lhs, const Angle rhs)
    {
        return Angle(lhs.radians + rhs.radians);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Compound addition of two angles
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] friend constexpr Angle& operator+=(Angle& lhs, const Angle rhs)
    {
        lhs.radians += rhs.radians;
        return lhs;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Difference of two angles
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr Angle operator-(const Angle lhs, const Angle rhs)
    {
        return Angle(lhs.radians - rhs.radians);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Compound subtraction of two angles
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] friend constexpr Angle& operator-=(Angle& lhs, const Angle rhs)
    {
        lhs.radians -= rhs.radians;
        return lhs;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Scale an angle by a scalar
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr Angle operator*(const Angle lhs, const float rhs)
    {
        return Angle(lhs.radians * rhs);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Scale an angle by a scalar
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr Angle operator*(const float lhs, const Angle rhs)
    {
        return rhs * lhs;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Compound scaling of an angle by a scalar
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] friend constexpr Angle& operator*=(Angle& lhs, const float rhs)
    {
        lhs.radians *= rhs;
        return lhs;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Divide an angle by a scalar
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr Angle operator/(const Angle lhs, const float rhs)
    {
        ZA_ASSERT(rhs != 0.f && "Angle::operator/ cannot divide by 0");
        return Angle(lhs.radians / rhs);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Compound division of an angle by a scalar
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] friend constexpr Angle& operator/=(Angle& lhs, const float rhs)
    {
        ZA_ASSERT(rhs != 0.f && "Angle::operator/= cannot divide by 0");
        lhs.radians /= rhs;
        return lhs;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Ratio of two angles
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr float operator/(const Angle lhs, const Angle rhs)
    {
        ZA_ASSERT(rhs.radians != 0.f && "Angle::operator/ cannot divide by 0");
        return lhs.radians / rhs.radians;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Modulo of an angle by another angle (always non-negative).
    ///
    /// Right hand angle must be greater than zero.
    ///
    /// Examples:
    /// \code
    /// za::degrees(90) % za::degrees(40)  // 10 degrees
    /// za::degrees(-90) % za::degrees(40) // 30 degrees (not -10)
    /// \endcode
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] friend constexpr Angle operator%(const Angle lhs, const Angle rhs)
    {
        ZA_ASSERT(rhs.radians != 0.f && "Angle::operator% cannot modulus by 0");
        return Angle(za::positiveRemainder(lhs.radians, rhs.radians));
    }


    ////////////////////////////////////////////////////////////
    /// \brief Compound modulo of an angle by another angle
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] friend constexpr Angle& operator%=(Angle& lhs, const Angle rhs)
    {
        ZA_ASSERT(rhs.radians != 0.f && "Angle::operator%= cannot modulus by 0");
        lhs.radians = za::positiveRemainder(lhs.radians, rhs.radians);
        return lhs;
    }


    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    float radians{0.f}; //!< Angle value stored as radians
};


////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-identifier-naming)
inline constexpr Angle Angle::Zero{}; //!< Predefined 0 degree angle value


////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-identifier-naming)
inline constexpr Angle Angle::Quarter{za::halfPi}; //!< Predefined 90 degree angle value


////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-identifier-naming)
inline constexpr Angle Angle::Half{za::pi}; //!< Predefined 180 degree angle value


////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-identifier-naming)
inline constexpr Angle Angle::Full{za::tau}; //!< Predefined 360 degree angle value


////////////////////////////////////////////////////////////
/// \brief Construct an angle value from a number of degrees
///
/// \see `radians`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] constexpr Angle degrees(const float angle)
{
    return Angle(angle * (za::pi / 180.f));
}


////////////////////////////////////////////////////////////
/// \brief Construct an angle value from a number of radians
///
/// \see `degrees`
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] constexpr Angle radians(const float angle)
{
    return Angle(angle);
}


namespace Literals
{
////////////////////////////////////////////////////////////
/// \brief User-defined literal for angles in degrees, e.g.\ `10.5_deg`
///
////////////////////////////////////////////////////////////
[[nodiscard]] consteval Angle operator""_deg(const long double angle)
{
    return degrees(static_cast<float>(angle));
}


////////////////////////////////////////////////////////////
/// \brief User-defined literal for angles in degrees, e.g.\ `90_deg`
///
////////////////////////////////////////////////////////////
[[nodiscard]] consteval Angle operator""_deg(const unsigned long long int angle)
{
    return degrees(static_cast<float>(angle));
}


////////////////////////////////////////////////////////////
/// \brief User-defined literal for angles in radians, e.g.\ `0.1_rad`
///
////////////////////////////////////////////////////////////
[[nodiscard]] consteval Angle operator""_rad(const long double angle)
{
    return radians(static_cast<float>(angle));
}

////////////////////////////////////////////////////////////
/// \brief User-defined literal for angles in radians, e.g.\ `2_rad`
///
////////////////////////////////////////////////////////////
[[nodiscard]] consteval Angle operator""_rad(const unsigned long long int angle)
{
    return radians(static_cast<float>(angle));
}

} // namespace Literals
} // namespace za


////////////////////////////////////////////////////////////
/// \class za::Angle
/// \ingroup system
///
/// `za::Angle` encapsulates an angle value, allowing it to be
/// defined and read back as either degrees or radians, without
/// imposing any fixed unit on the API.
///
/// Angle values support the usual mathematical operations
/// (addition, subtraction, scaling, comparison, etc.).
///
/// Usage example:
/// \code
/// za::Angle a1  = za::degrees(90);
/// float radians = a1.asRadians(); // 1.5708f
///
/// za::Angle a2 = za::radians(3.141592654f);
/// float degrees = a2.asDegrees(); // 180.f
///
/// using namespace za::Literals;
/// za::Angle a3 = 10_deg;   // 10 degrees
/// za::Angle a4 = 1.5_deg;  // 1.5 degrees
/// za::Angle a5 = 1_rad;    // 1 radians
/// za::Angle a6 = 3.14_rad; // 3.14 radians
/// \endcode
///
////////////////////////////////////////////////////////////
