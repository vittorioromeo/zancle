#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/Angle.hpp"

#include "Zancle/Math/Constants.hpp"

#include "Zancle/Base/Assert.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief A wrapper around `za::Angle` that automatically wraps the angle value
///
/// This class behaves similarly to `za::Angle` but automatically wraps
/// the angle to the range `[0, 360)` degrees (or `[0, 2*Pi)` radians)
/// whenever its value is modified, so reading it is free.
///
/// This is useful for representing properties like rotation where only
/// the final orientation matters, regardless of the number of full turns.
///
////////////////////////////////////////////////////////////
class [[nodiscard]] AutoWrapAngle
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default-construct to 0 degrees
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr AutoWrapAngle() = default;

    ////////////////////////////////////////////////////////////
    /// \brief Construct from an `za::Angle`, wrapping it into `[0, 360)`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr /* implicit */ AutoWrapAngle(const Angle angle) :
        m_radians(angle.wrapUnsigned().radians)
    {
    }

    ////////////////////////////////////////////////////////////
    /// \brief Assign an `za::Angle`, wrapping it into `[0, 360)`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] inline constexpr AutoWrapAngle& operator=(const Angle angle) noexcept
    {
        m_radians = angle.wrapUnsigned().radians;
        return *this;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Implicitly convert to `za::Angle` (always in `[0, 360)`)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr operator Angle() const noexcept
    {
        return radians(m_radians);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Wrapped value in degrees
    ///
    /// \see `asRadians`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr float asDegrees() const
    {
        return m_radians * (180.f / za::pi);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Wrapped value in radians
    ///
    /// \see `asDegrees`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr float asRadians() const
    {
        return m_radians;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Rotate towards `other` by at most `speed` (see `za::Angle::rotatedTowards`)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Angle rotatedTowards(const Angle other,
                                                                                                     const float speed) const
    {
        return operator Angle().rotatedTowards(other, speed);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Equality of the wrapped angle values
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] friend constexpr bool operator==(const AutoWrapAngle lhs,
                                                                                                const AutoWrapAngle rhs)
    {
        return lhs.m_radians == rhs.m_radians;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Equality of the wrapped angle values (`rhs` is wrapped before comparing)
    ///
    /// Also needed to disambiguate mixed comparisons, as both types
    /// are implicitly convertible to each other.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] friend constexpr bool operator==(const AutoWrapAngle lhs,
                                                                                                const Angle         rhs)
    {
        return lhs.m_radians == rhs.wrapUnsigned().radians;
    }

    ////////////////////////////////////////////////////////////
    // Make the non-mutating `za::Angle` operators (hidden friends of `za::Angle`)
    // findable via ADL when all operands are `AutoWrapAngle`, e.g. `a < b` or
    // `a + b`. The operands are converted to their wrapped `za::Angle` values.
    ////////////////////////////////////////////////////////////
    friend constexpr bool  operator<(Angle lhs, Angle rhs);
    friend constexpr bool  operator>(Angle lhs, Angle rhs);
    friend constexpr bool  operator<=(Angle lhs, Angle rhs);
    friend constexpr bool  operator>=(Angle lhs, Angle rhs);
    friend constexpr Angle operator-(Angle rhs);
    friend constexpr Angle operator+(Angle lhs, Angle rhs);
    friend constexpr Angle operator-(Angle lhs, Angle rhs);
    friend constexpr Angle operator*(Angle lhs, float rhs);
    friend constexpr Angle operator*(float lhs, Angle rhs);
    friend constexpr Angle operator/(Angle lhs, float rhs);
    friend constexpr float operator/(Angle lhs, Angle rhs);
    friend constexpr Angle operator%(Angle lhs, Angle rhs);

    ////////////////////////////////////////////////////////////
    /// \brief Add `rhs` to the angle value, then wrap
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] inline constexpr AutoWrapAngle& operator+=(const Angle rhs)
    {
        return *this = radians(m_radians + rhs.radians);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Subtract `rhs` from the angle value, then wrap
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] inline constexpr AutoWrapAngle& operator-=(const Angle rhs)
    {
        return *this = radians(m_radians - rhs.radians);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Multiply the angle value by `rhs`, then wrap
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] inline constexpr AutoWrapAngle& operator*=(const float rhs)
    {
        return *this = radians(m_radians * rhs);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Divide the angle value by `rhs` (asserts `rhs != 0`), then wrap
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] inline constexpr AutoWrapAngle& operator/=(const float rhs)
    {
        ZA_ASSERT(rhs != 0.f);
        return *this = radians(m_radians / rhs);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Assign `*this % rhs`, then wrap (see `za::Angle::operator%`)
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] inline constexpr AutoWrapAngle& operator%=(const Angle rhs)
    {
        return *this = (operator Angle() % rhs);
    }

private:
    float m_radians{0.f}; //!< Always in `[0, 2*Pi)`
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::AutoWrapAngle
/// \ingroup system
///
/// `za::AutoWrapAngle` is a wrapper around `za::Angle` that automatically
/// normalizes the angle to the range `[0, 360)` degrees (or `[0, 2*Pi)` radians)
/// whenever it is modified.
///
/// Wrapping on modification (rather than on access) keeps the stored value
/// small, so repeated increments do not lose precision, and makes reads free.
///
/// This is particularly useful for representing properties like rotation
/// where angles outside the standard range are equivalent (e.g., 450 degrees
/// is the same orientation as 90 degrees). Using `AutoWrapAngle` ensures
/// that comparisons and operations work intuitively in such cases.
///
/// \see `za::Angle`
///
////////////////////////////////////////////////////////////
