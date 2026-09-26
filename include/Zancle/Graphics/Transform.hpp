#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/SinCosLookup.hpp"

#include "Zancle/Base/AssertAndAssume.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief 2D transformation convertible to 4x4 matrix
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] Transform
{
    ////////////////////////////////////////////////////////////
    /// \brief Build a translation-only transform
    ///
    /// Equivalent to `Identity` followed by `translate(position)`,
    /// but expressed as a direct construction.
    ///
    /// \param position Translation to bake into the transform
    ///
    /// \return Transform that translates by `position`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] static constexpr Transform fromPosition(const Vec2f position)
    {
        return {
            .a00 = 1.f,
            .a01 = 0.f,
            .a02 = position.x,
            .a10 = 0.f,
            .a11 = 1.f,
            .a12 = position.y,
        };
    }


    ////////////////////////////////////////////////////////////
    /// \brief Build a transform from a position, scale, and origin
    ///
    /// Equivalent to building the transform that
    /// `za::Transformable` with the same `position`, `scale`, and
    /// `origin` (and a zero rotation) would produce. Useful as a
    /// fast path when no rotation is involved.
    ///
    /// \param position World-space position
    /// \param scale    Per-axis scale factors
    /// \param origin   Origin of translation/scaling, in local space
    ///
    /// \return Transform that applies the requested translation and scaling
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] static constexpr Transform fromPositionScaleOrigin(
        const Vec2f position,
        const Vec2f scale,
        const Vec2f origin)
    {
        return {
            .a00 = scale.x,
            .a01 = 0.f,
            .a02 = position.x - origin.x * scale.x,
            .a10 = 0.f,
            .a11 = scale.y,
            .a12 = position.y - origin.y * scale.y,
        };
    }


    ////////////////////////////////////////////////////////////
    /// \brief Build a transform from a position, scale, origin, and a precomputed (sine, cosine) pair
    ///
    /// This is the fully general form: it produces the same matrix
    /// as the equivalent `Transformable`, but skips the trigonometry
    /// by accepting `sine` and `cosine` directly. Pass values from
    /// `za::sinCosLookup` (or any equivalent source) when you
    /// already have them, to avoid recomputing them per object.
    ///
    /// \param position World-space position
    /// \param scale    Per-axis scale factors
    /// \param origin   Origin of translation/rotation/scaling, in local space
    /// \param sine     Sine of the rotation angle (must be in `[-1, 1]`)
    /// \param cosine   Cosine of the rotation angle (must be in `[-1, 1]`)
    ///
    /// \return Transform that applies the requested translation, rotation, and scaling
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] static constexpr Transform fromPositionScaleOriginSinCos(
        const Vec2f position,
        const Vec2f scale,
        const Vec2f origin,
        const float sine,
        const float cosine)
    {
        ZA_ASSERT_AND_ASSUME(sine >= -1.f && sine <= 1.f);
        ZA_ASSERT_AND_ASSUME(cosine >= -1.f && cosine <= 1.f);

        const float sxc = scale.x * cosine;
        const float syc = scale.y * cosine;
        const float sxs = scale.x * sine;
        const float sys = scale.y * sine;
        const float tx  = -origin.x * sxc + origin.y * sys + position.x;
        const float ty  = -origin.x * sxs - origin.y * syc + position.y;

        return {
            .a00 = sxc,
            .a01 = -sys,
            .a02 = tx,
            .a10 = sxs,
            .a11 = syc,
            .a12 = ty,
        };
    }


    ////////////////////////////////////////////////////////////
    /// \brief Build a transform from a position, scale, origin, and rotation
    ///
    /// Computes sine and cosine of `rotation`, so if you already have those values,
    /// prefer `fromPositionScaleOriginSinCos` to avoid redundant trig calculations.
    ///
    /// \param position World-space position
    /// \param scale    Per-axis scale factors
    /// \param origin   Origin of translation/rotation/scaling, in local space
    /// \param rotation Rotation angle
    ///
    /// \return Transform that applies the requested translation, rotation, and scaling
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] static constexpr Transform fromPositionScaleOriginRotation(
        const Vec2f position,
        const Vec2f scale,
        const Vec2f origin,
        const Angle rotation)
    {
        const auto [sine, cosine] = za::sinCosLookup(rotation.asRadians());
        return fromPositionScaleOriginSinCos(position, scale, origin, sine, cosine);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Write the 2D transform into a 4x4 column-major matrix
    ///
    /// This writes the six meaningful elements of the 2D transform
    /// (`a00`, `a01`, `a02`, `a10`, `a11`, `a12`) into the appropriate
    /// slots of a column-major 4x4 matrix. The remaining slots are
    /// **not** touched, so the caller must pre-initialize `target`
    /// to a 4x4 identity matrix (or to whatever Z/W column they need)
    /// before calling this function.
    ///
    /// The resulting layout is directly compatible with OpenGL's
    /// `glUniformMatrix4fv` and similar APIs.
    ///
    /// \code
    /// za::Transform transform = ...;
    ///
    /// float matrix[16] = {
    ///     1.f, 0.f, 0.f, 0.f,
    ///     0.f, 1.f, 0.f, 0.f,
    ///     0.f, 0.f, 1.f, 0.f,
    ///     0.f, 0.f, 0.f, 1.f,
    /// };
    ///
    /// transform.writeTo4x4Matrix(matrix);
    /// // 'matrix' is now ready to be uploaded as a uniform.
    /// \endcode
    ///
    /// \param target Pre-initialized 4x4 matrix to write into
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void writeTo4x4Matrix(float (&target)[16]) const
    {
        target[0]  = a00;
        target[1]  = a10;
        target[4]  = a01;
        target[5]  = a11;
        target[12] = a02;
        target[13] = a12;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Return the inverse of the transform
    ///
    /// If the inverse cannot be computed, an identity transform
    /// is returned.
    ///
    /// \return A new transform which is the inverse of self
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Transform getInverse() const
    {
        // clang-format off
        // Compute the determinant
        const float det = a00 * a11 - a10 * a01;
        // clang-format on

        // Compute the inverse if the determinant is not zero
        // (don't use an epsilon because the determinant may *really* be tiny)
        if (det == 0.f)
            return Identity;

        const float invDet = 1.f / det;

        return {
            .a00 = a11 * invDet,
            .a01 = -a01 * invDet,
            .a02 = (a12 * a01 - a11 * a02) * invDet,
            .a10 = -a10 * invDet,
            .a11 = a00 * invDet,
            .a12 = -(a12 * a00 - a10 * a02) * invDet,
        };
    }


    ////////////////////////////////////////////////////////////
    /// \brief Transform a 2D point
    ///
    /// These two statements are equivalent:
    /// \code
    /// za::Vec2f transformedPoint = matrix.transformPoint(point);
    /// za::Vec2f transformedPoint = matrix * point;
    /// \endcode
    ///
    /// \param point Point to transform
    ///
    /// \return Transformed point
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr Vec2f transformPoint(const Vec2f point) const
    {
        return {
            .x = a00 * point.x + a01 * point.y + a02,
            .y = a10 * point.x + a11 * point.y + a12,
        };
    }


    ////////////////////////////////////////////////////////////
    /// \brief Transform a rectangle
    ///
    /// Since Zancle doesn't provide support for oriented rectangles,
    /// the result of this function is always an axis-aligned
    /// rectangle. Which means that if the transform contains a
    /// rotation, the bounding rectangle of the transformed rectangle
    /// is returned.
    ///
    /// \param rectangle Rectangle to transform
    ///
    /// \return Transformed rectangle
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr Rect2f transformRect(const Rect2f& rectangle) const
    {
        const Vec2f p0 = transformPoint(rectangle.position);

        // Per-axis edge contributions from each side of the rectangle
        const float e0x = a00 * rectangle.size.x;
        const float e0y = a10 * rectangle.size.x;
        const float e1x = a01 * rectangle.size.y;
        const float e1y = a11 * rectangle.size.y;

        // The minimum corner is p0 offset by the negative contributions
        const float minX = p0.x + (e0x < 0.f ? e0x : 0.f) + (e1x < 0.f ? e1x : 0.f);
        const float minY = p0.y + (e0y < 0.f ? e0y : 0.f) + (e1y < 0.f ? e1y : 0.f);

        // The size is the sum of the absolute edge contributions
        return Rect2f{{minX, minY}, {ZA_MATH_FABSF(e0x) + ZA_MATH_FABSF(e1x), ZA_MATH_FABSF(e0y) + ZA_MATH_FABSF(e1y)}};
    }


    ////////////////////////////////////////////////////////////
    /// \relates za::Transform
    /// \brief Overload of binary `operator*` to combine two transforms
    ///
    /// This call is equivalent to calling `Transform(lhs).combine(rhs)`.
    ///
    /// \param lhs Left operand (the first transform)
    /// \param rhs Right operand (the second transform)
    ///
    /// \return New combined transform
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] friend constexpr Transform operator*(const Transform& lhs, const Transform& rhs)
    {
        return {
            .a00 = lhs.a00 * rhs.a00 + lhs.a01 * rhs.a10,
            .a01 = lhs.a00 * rhs.a01 + lhs.a01 * rhs.a11,
            .a02 = lhs.a00 * rhs.a02 + lhs.a01 * rhs.a12 + lhs.a02,
            .a10 = lhs.a10 * rhs.a00 + lhs.a11 * rhs.a10,
            .a11 = lhs.a10 * rhs.a01 + lhs.a11 * rhs.a11,
            .a12 = lhs.a10 * rhs.a02 + lhs.a11 * rhs.a12 + lhs.a12,
        };
    }


    ////////////////////////////////////////////////////////////
    /// \relates za::Transform
    /// \brief Overload of binary `operator*=` to combine two transforms
    ///
    /// This call is equivalent to calling `lhs.combine(rhs)`.
    ///
    /// \param lhs Left operand (the first transform)
    /// \param rhs Right operand (the second transform)
    ///
    /// \return The combined transform
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] friend constexpr Transform& operator*=(Transform& lhs, const Transform& rhs)
    {
        return lhs.combine(rhs);
    }


    ////////////////////////////////////////////////////////////
    /// \relates za::Transform
    /// \brief Overload of binary `operator*` to transform a point
    ///
    /// This call is equivalent to calling `lhs.transformPoint(rhs)`.
    ///
    /// \param lhs Left operand (the transform)
    /// \param rhs Right operand (the point to transform)
    ///
    /// \return New transformed point
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] friend constexpr Vec2f operator*(const Transform& lhs,
                                                                                                const Vec2f      rhs)
    {
        return lhs.transformPoint(rhs);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Combine the current transform with another one
    ///
    /// The result is a transform that is equivalent to applying
    /// `transform` followed by `*this`. Mathematically, it is
    /// equivalent to a matrix multiplication `(*this) * transform`.
    ///
    /// These two statements are equivalent:
    /// \code
    /// lhs.combine(rhs);
    /// lhs *= rhs;
    /// \endcode
    ///
    /// \param transform Transform to combine with this transform
    ///
    /// \return Reference to `*this`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] constexpr Transform& combine(const Transform& transform)
    {
        return *this = operator*(*this, transform);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Combine the current transform with a translation
    ///
    /// This function returns a reference to `*this`, so that calls
    /// can be chained.
    /// \code
    /// za::Transform transform;
    /// transform.translate(za::Vec2f{100, 200}).rotate(za::degrees(45));
    /// \endcode
    ///
    /// \param offset Translation offset to apply
    ///
    /// \return Reference to `*this`
    ///
    /// \see `rotate`, `scale`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr Transform& translate(const Vec2f offset)
    {
        a02 += a00 * offset.x + a01 * offset.y;
        a12 += a10 * offset.x + a11 * offset.y;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Combine the current transform with a rotation
    ///
    /// This function returns a reference to `*this`, so that calls
    /// can be chained.
    /// \code
    /// za::Transform transform;
    /// transform.rotate(za::degrees(90)).translate(50, 20);
    /// \endcode
    ///
    /// \param angle Rotation angle
    ///
    /// \return Reference to `*this`
    ///
    /// \see `translate`, `scale`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr Transform& rotate(const Angle angle)
    {
        const auto [sine, cosine] = za::sinCosLookup(angle.asRadians());

        const float m00 = a00;
        const float m01 = a01;
        const float m10 = a10;
        const float m11 = a11;

        a00 = m00 * cosine + m01 * sine;
        a01 = m00 * -sine + m01 * cosine;
        a10 = m10 * cosine + m11 * sine;
        a11 = m10 * -sine + m11 * cosine;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Combine the current transform with a rotation
    ///
    /// The center of rotation is provided for convenience as a second
    /// argument, so that you can build rotations around arbitrary points
    /// more easily (and efficiently) than the usual
    /// `translate(-center).rotate(angle).translate(center)`.
    ///
    /// This function returns a reference to `*this`, so that calls
    /// can be chained.
    /// \code
    /// za::Transform transform;
    /// transform.rotate(za::degrees(90), za::Vec2f{8, 3}).translate(za::Vec2f{50, 20});
    /// \endcode
    ///
    /// \param angle Rotation angle
    /// \param center Center of rotation
    ///
    /// \return Reference to `*this`
    ///
    /// \see `translate`, `scale`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] constexpr Transform& rotate(const Angle angle, const Vec2f center)
    {
        const auto [sine, cosine] = za::sinCosLookup(angle.asRadians());

        // Precompute the translation components of the rotation matrix
        const float tx = center.x * (1.f - cosine) + center.y * sine;
        const float ty = center.y * (1.f - cosine) - center.x * sine;

        // Apply the translation to the current translation column
        a02 += a00 * tx + a01 * ty;
        a12 += a10 * tx + a11 * ty;

        // Apply the rotation to the scale/shear columns
        const float m00 = a00;
        const float m01 = a01;
        const float m10 = a10;
        const float m11 = a11;

        a00 = m00 * cosine + m01 * sine;
        a01 = m00 * -sine + m01 * cosine;
        a10 = m10 * cosine + m11 * sine;
        a11 = m10 * -sine + m11 * cosine;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Combine the current transform with a scaling
    ///
    /// This function returns a reference to `*this`, so that calls
    /// can be chained.
    /// \code
    /// za::Transform transform;
    /// transform.scaleBy(za::Vec2f{2, 1}).rotate(za::degrees(45));
    /// \endcode
    ///
    /// \param factors Scaling factors
    ///
    /// \return Reference to `*this`
    ///
    /// \see `translate`, `rotate`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr Transform& scaleBy(const Vec2f factors)
    {
        a00 *= factors.x;
        a01 *= factors.y;
        a10 *= factors.x;
        a11 *= factors.y;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Combine the current transform with a scaling
    ///
    /// The center of scaling is provided for convenience as a second
    /// argument, so that you can build scaling around arbitrary points
    /// more easily (and efficiently) than the usual
    /// `translate(-center).scaleBy(factors).translate(center)`.
    ///
    /// This function returns a reference to `*this`, so that calls
    /// can be chained.
    /// \code
    /// za::Transform transform;
    /// transform.scaleBy(za::Vec2f{2, 1}, za::Vec2f{8, 3}).rotate(45);
    /// \endcode
    ///
    /// \param factors Scaling factors
    /// \param center Center of scaling
    ///
    /// \return Reference to `*this`
    ///
    /// \see `translate`, `rotate`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr Transform& scaleBy(const Vec2f factors, const Vec2f center)
    {
        const float s02 = center.x * (1.f - factors.x);
        const float s12 = center.y * (1.f - factors.y);

        a02 += a00 * s02 + a01 * s12;
        a12 += a10 * s02 + a11 * s12;

        a00 *= factors.x;
        a01 *= factors.y;
        a10 *= factors.x;
        a11 *= factors.y;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Overload of binary `operator==` to compare two transforms
    ///
    /// Performs an element-wise comparison of the elements of the
    /// lhs transform with the elements of the rhs transform.
    ///
    /// \param rhs Right operand
    ///
    /// \return `true` if the transforms are equal, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool operator==(const Transform& rhs) const = default;

    ////////////////////////////////////////////////////////////
    // Static member data
    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(readability-identifier-naming)
    static const Transform Identity; //!< The identity transform (does nothing)

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    float a00{1.f}, a01{0.f}, a02{0.f}, a10{0.f}, a11{1.f}, a12{0.f};
};


////////////////////////////////////////////////////////////
inline constexpr Transform Transform::Identity{};

} // namespace za


////////////////////////////////////////////////////////////
/// \struct za::Transform
/// \ingroup graphics
///
/// `za::Transform` is a 2D affine transform that can translate,
/// rotate, scale, and shear points. Internally it stores the six
/// meaningful elements of the upper-left 2x3 block of a 4x4
/// homogeneous matrix.
///
/// In mathematical terms, a `za::Transform` defines how to map a
/// coordinate system into another. For example, applying a rotation
/// transform to a sprite produces a rotated sprite, and applying it
/// to a point produces the rotated point.
///
/// Transforms are most commonly used for drawing (via
/// `za::Transformable` or directly as part of `za::RenderStates`),
/// but they are equally useful for converting points between local
/// and world space (e.g. for picking, hit testing, or collision
/// detection).
///
/// Example:
/// \code
/// // Define a translation transform.
/// za::Transform translation;
/// translation.translate({20.f, 50.f});
///
/// // Define a rotation transform.
/// za::Transform rotation;
/// rotation.rotate(za::degrees(45.f));
///
/// // Combine them: 'transform' first applies 'rotation', then 'translation'.
/// const za::Transform transform = translation * rotation;
///
/// // Use the result to transform points and rectangles.
/// const za::Vec2f  point = transform.transformPoint({10.f, 20.f});
/// const za::Rect2f rect  = transform.transformRect({{0.f, 0.f}, {10.f, 100.f}});
/// \endcode
///
/// `za::Transform` is an aggregate, can be created with designated
/// initializers, and exposes static factory helpers (`fromPosition`,
/// `fromPositionScaleOrigin`, `fromPositionScaleOriginSinCos`) for
/// efficient construction from `Transformable`-style parameters.
///
/// \see `za::Transformable`, `za::RenderStates`
///
////////////////////////////////////////////////////////////
