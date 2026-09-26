#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Math/MinMaxMacros.hpp"

#include "Zancle/Trait/IsFloatingPoint.hpp"
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/IsUnsigned.hpp"

#include "Zancle/Base/SizeT.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Normalized extents of a rectangle: `min` is inclusive, `max` is exclusive
///
////////////////////////////////////////////////////////////
template <typename T>
struct RectBounds
{
    T minX, minY, maxX, maxY;
};


////////////////////////////////////////////////////////////
/// \brief Compute the normalized extents of the rectangle defined by `position` and `size`
///
/// Rectangles with negative dimensions are allowed, so the extents are sorted
/// per axis. Unsigned sizes cannot be negative, so that step is skipped.
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] constexpr RectBounds<T> getRectBounds(const Vec2<T> position,
                                                                                                  const Vec2<T> size)
{
    const T right  = position.x + size.x;
    const T bottom = position.y + size.y;

    if constexpr (ZA_IS_UNSIGNED(T))
        return {position.x, position.y, right, bottom};
    else
        return {ZA_MIN(position.x, right), ZA_MIN(position.y, bottom), ZA_MAX(position.x, right), ZA_MAX(position.y, bottom)};
}

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief 2D axis-aligned rectangle defined by a top-left `position` and a `size`.
/// \ingroup system
///
////////////////////////////////////////////////////////////
template <typename T>
class [[nodiscard]] Rect2
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Check if `point` is inside the rectangle's area
    ///
    /// This check is non-inclusive: points on the right or bottom
    /// edge are considered outside the rectangle.
    ///
    /// \see `intersects`, `findIntersection`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr bool contains(const Vec2<T> point) const
    {
        const auto [minX, minY, maxX, maxY] = priv::getRectBounds(position, size);
        return (point.x >= minX) && (point.x < maxX) && (point.y >= minY) && (point.y < maxY);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Check if the rectangle's area overlaps `other`'s area
    ///
    /// Cheaper than `findIntersection` when the intersection itself is not needed.
    /// Rectangles that only touch along an edge do not intersect.
    ///
    /// \see `contains`, `findIntersection`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr bool intersects(const Rect2& other) const
    {
        const auto r0 = priv::getRectBounds(position, size);
        const auto r1 = priv::getRectBounds(other.position, other.size);

        return ZA_MAX(r0.minX, r1.minX) < ZA_MIN(r0.maxX, r1.maxX) && ZA_MAX(r0.minY, r1.minY) < ZA_MIN(r0.maxY, r1.maxY);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Convert to `Rect2<int>`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Rect2<int> toRect2i() const
    {
        return {position.toVec2i(), size.toVec2i()};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Convert to `Rect2<float>`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Rect2<float> toRect2f() const
    {
        return {position.toVec2f(), size.toVec2f()};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Convert to `Rect2<unsigned int>`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Rect2<unsigned int> toRect2u() const
    {
        return {position.toVec2u(), size.toVec2u()};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Convert to `Rect2<za::SizeT>`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Rect2<za::SizeT> toRect2uz() const
    {
        return {position.toVec2uz(), size.toVec2uz()};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Strict member-wise equality
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool operator==(const Rect2<T>& rhs) const = default;


    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of a specific anchor point within the rectangle
    ///
    /// `factors` are normalized in the range `[0, 1]`: `(0, 0)` is the top-left,
    /// `(1, 1)` is the bottom-right, `(0.5, 0.5)` is the center.
    ///
    /// For integral `T`, the scaled size is computed in `float` and truncated,
    /// so it is exact only for sizes up to `2^24`. Prefer the named anchor
    /// getters (e.g. `getCenter()`), which are always exact and cheaper.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr Vec2<T> getAnchorPoint(const Vec2f factors) const
    {
        return position + getScaledSize(factors);
    }


    ////////////////////////////////////////////////////////////
#define ZA_PRIV_DEFINE_RECT_ANCHOR_GETTER(name, offsetX, offsetY)                             \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr Vec2<T> name() const \
    {                                                                                         \
        return {position.x + (offsetX), position.y + (offsetY)};                              \
    }

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the top-left anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_GETTER(getTopLeft, T{0}, T{0})

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the top-center anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_GETTER(getTopCenter, half(size.x), T{0})

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the top-right anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_GETTER(getTopRight, size.x, T{0})

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the center-left anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_GETTER(getCenterLeft, T{0}, half(size.y))

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the center anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_GETTER(getCenter, half(size.x), half(size.y))

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the center-right anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_GETTER(getCenterRight, size.x, half(size.y))

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the bottom-left anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_GETTER(getBottomLeft, T{0}, size.y)

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the bottom-center anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_GETTER(getBottomCenter, half(size.x), size.y)

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the bottom-right anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_GETTER(getBottomRight, size.x, size.y)

#undef ZA_PRIV_DEFINE_RECT_ANCHOR_GETTER


    ////////////////////////////////////////////////////////////
    /// \brief Get the offset to apply so that the given anchor lands on the current top-left
    ///
    /// Useful for positioning the rectangle relative to one of its anchor points.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr Vec2<T> getAnchorPointOffset(const Vec2f factors) const
    {
        return -getScaledSize(factors);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Get the X coordinate of the left edge (i.e. `position.x`)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr T getLeft() const
    {
        return position.x;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Get the X coordinate of the right edge (i.e. `position.x + size.x`)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr T getRight() const
    {
        return position.x + size.x;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Get the Y coordinate of the top edge (i.e. `position.y`)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr T getTop() const
    {
        return position.y;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Get the Y coordinate of the bottom edge (i.e. `position.y + size.y`)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr T getBottom() const
    {
        return position.y + size.y;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Move the rectangle so that the anchor identified by `factors` lands on `newPosition`
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] constexpr void setAnchorPoint(const Vec2f factors, const Vec2<T> newPosition)
    {
        position = newPosition - getScaledSize(factors);
    }


    ////////////////////////////////////////////////////////////
#define ZA_PRIV_DEFINE_RECT_ANCHOR_SETTER(name, offsetX, offsetY)                  \
    [[gnu::always_inline, gnu::flatten]] constexpr void name(const Vec2<T> newPos) \
    {                                                                              \
        position = {newPos.x - (offsetX), newPos.y - (offsetY)};                   \
    }

    ////////////////////////////////////////////////////////////
    /// \brief Move the rectangle so that the top-left anchor lands on `newPos`
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_SETTER(setTopLeft, T{0}, T{0})

    ////////////////////////////////////////////////////////////
    /// \brief Move the rectangle so that the top-center anchor lands on `newPos`
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_SETTER(setTopCenter, half(size.x), T{0})

    ////////////////////////////////////////////////////////////
    /// \brief Move the rectangle so that the top-right anchor lands on `newPos`
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_SETTER(setTopRight, size.x, T{0})

    ////////////////////////////////////////////////////////////
    /// \brief Move the rectangle so that the center-left anchor lands on `newPos`
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_SETTER(setCenterLeft, T{0}, half(size.y))

    ////////////////////////////////////////////////////////////
    /// \brief Move the rectangle so that the center anchor lands on `newPos`
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_SETTER(setCenter, half(size.x), half(size.y))

    ////////////////////////////////////////////////////////////
    /// \brief Move the rectangle so that the center-right anchor lands on `newPos`
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_SETTER(setCenterRight, size.x, half(size.y))

    ////////////////////////////////////////////////////////////
    /// \brief Move the rectangle so that the bottom-left anchor lands on `newPos`
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_SETTER(setBottomLeft, T{0}, size.y)

    ////////////////////////////////////////////////////////////
    /// \brief Move the rectangle so that the bottom-center anchor lands on `newPos`
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_SETTER(setBottomCenter, half(size.x), size.y)

    ////////////////////////////////////////////////////////////
    /// \brief Move the rectangle so that the bottom-right anchor lands on `newPos`
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_RECT_ANCHOR_SETTER(setBottomRight, size.x, size.y)

#undef ZA_PRIV_DEFINE_RECT_ANCHOR_SETTER


    ////////////////////////////////////////////////////////////
    /// \brief Set the X coordinate of the left edge (assigns `position.x`)
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] constexpr void setLeft(const T newCoordinate)
    {
        position.x = newCoordinate;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Set the X coordinate of the right edge (adjusts `position.x` to keep `size.x`)
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] constexpr void setRight(const T newCoordinate)
    {
        position.x = newCoordinate - size.x;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Set the Y coordinate of the top edge (assigns `position.y`)
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] constexpr void setTop(const T newCoordinate)
    {
        position.y = newCoordinate;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Set the Y coordinate of the bottom edge (adjusts `position.y` to keep `size.y`)
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] constexpr void setBottom(const T newCoordinate)
    {
        position.y = newCoordinate - size.y;
    }


    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    Vec2<T> position; //!< Position of the top-left corner of the rectangle
    Vec2<T> size;     //!< Size of the rectangle


private:
    ////////////////////////////////////////////////////////////
    /// \brief Half of `value`: exact for floating-point `T`, truncated towards zero for integral `T`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] static constexpr T half(const T value)
    {
        if constexpr (ZA_IS_FLOATING_POINT(T))
            return value * T{0.5};
        else
            return value / T{2};
    }


    ////////////////////////////////////////////////////////////
    /// \brief `size` scaled component-wise by `factors`
    ///
    /// Computed natively for floating-point `T`, and in `float` (then truncated) for integral `T`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr Vec2<T> getScaledSize(const Vec2f factors) const
    {
        if constexpr (ZA_IS_SAME(T, float))
            return size.componentWiseMul(factors);
        else if constexpr (ZA_IS_FLOATING_POINT(T))
            return size.componentWiseMul(factors.template to<Vec2<T>>());
        else
            return size.toVec2f().componentWiseMul(factors).template to<Vec2<T>>();
    }
};

// Aliases for the most common types
using Rect2i  = Rect2<int>;
using Rect2f  = Rect2<float>;
using Rect2u  = Rect2<unsigned int>;
using Rect2uz = Rect2<za::SizeT>;

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::Rect2
/// \ingroup system
///
/// A rectangle is defined by its top-left corner (`position`) and its
/// `size`. Member variables are public and can be accessed directly,
/// just like in `Vec2` and `Vec3`.
///
/// Convenience accessors are provided for the four edges (`getLeft()`,
/// `getTop()`, `getRight()`, `getBottom()` and their setters) and for
/// anchor points (`getCenter()`, `getTopLeft()`, `setBottomRight()`, etc.).
/// Intersection testing is provided by `intersects()`, and the
/// intersection rectangle itself by `za::findIntersection`.
///
/// Boundary rules:
/// \li The left and top edges are included in the rectangle's area
/// \li The right and bottom edges are excluded from the rectangle's area
///
/// So `za::Rect2i({0, 0}, {1, 1})` and `za::Rect2i({1, 1}, {1, 1})` don't intersect.
///
/// Type aliases are provided for the common instantiations:
/// \li `za::Rect2i`  -> `za::Rect2<int>`
/// \li `za::Rect2f`  -> `za::Rect2<float>`
/// \li `za::Rect2u`  -> `za::Rect2<unsigned int>`
/// \li `za::Rect2uz` -> `za::Rect2<za::SizeT>`
///
/// Usage example:
/// \code
/// // Define a rectangle, located at (0, 0) with a size of 20x5
/// za::Rect2i r1({0, 0}, {20, 5});
///
/// // Define another rectangle, located at (4, 2) with a size of 18x10
/// za::Vec2i position(4, 2);
/// za::Vec2i size(18, 10);
/// za::Rect2i r2(position, size);
///
/// // Test intersections with the point (3, 1)
/// bool b1 = r1.contains({3, 1}); // true
/// bool b2 = r2.contains({3, 1}); // false
///
/// // Test the intersection between r1 and r2
/// za::Optional<za::Rect2i> result = za::findIntersection(r1, r2);
/// // result.hasValue() == true
/// // result.value() == za::Rect2i({4, 2}, {16, 3})
/// \endcode
///
////////////////////////////////////////////////////////////
