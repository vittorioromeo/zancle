#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Math/Floor.hpp"

#include "Zancle/Trait/IsIntegral.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Whether `GlobalAnchorPointMixin` can move a `T`
///
/// Requires either an accessible `position` data member that a `Vec2f`
/// offset can be added to, or `getPosition()` and `setPosition()`.
///
////////////////////////////////////////////////////////////
template <typename T>
concept GlobalAnchorPositionable = requires(T& t, const Vec2f offset) { t.position += offset; } || requires(T& t) {
    t.setPosition(t.getPosition());
};

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Mixin to get and set an object's position through anchor points of its global bounds
///
/// See the class documentation at the end of this file.
///
////////////////////////////////////////////////////////////
struct GlobalAnchorPointMixin
{
    ////////////////////////////////////////////////////////////
    /// \brief World coordinates of an anchor point given normalized `factors` in `[0, 1]`
    ///
    /// `(0, 0)` is the top-left, `(1, 1)` is the bottom-right.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr auto getGlobalAnchorPoint(this const auto& self, const Vec2f factors)
    {
        return self.getGlobalBounds().getAnchorPoint(factors);
    }


////////////////////////////////////////////////////////////
#define ZA_PRIV_DEFINE_MIXIN_GETTER(name, rectGetter)                                                            \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr auto name(this auto const& self) \
    {                                                                                                            \
        return self.getGlobalBounds().rectGetter();                                                              \
    }

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the top-left anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getGlobalTopLeft, getTopLeft);

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the top-center anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getGlobalTopCenter, getTopCenter);

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the top-right anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getGlobalTopRight, getTopRight);

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the center-left anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getGlobalCenterLeft, getCenterLeft);

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the center anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getGlobalCenter, getCenter);

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the center-right anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getGlobalCenterRight, getCenterRight);

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the bottom-left anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getGlobalBottomLeft, getBottomLeft);

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the bottom-center anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getGlobalBottomCenter, getBottomCenter);

    ////////////////////////////////////////////////////////////
    /// \brief Get the world position of the bottom-right anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getGlobalBottomRight, getBottomRight);

#undef ZA_PRIV_DEFINE_MIXIN_GETTER


    ////////////////////////////////////////////////////////////
    /// \brief World X coordinate of the left edge
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getGlobalLeft(this const auto& self)
    {
        return self.getGlobalBounds().getLeft();
    }


    ////////////////////////////////////////////////////////////
    /// \brief World X coordinate of the right edge
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getGlobalRight(this const auto& self)
    {
        return self.getGlobalBounds().getRight();
    }


    ////////////////////////////////////////////////////////////
    /// \brief World Y coordinate of the top edge
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getGlobalTop(this const auto& self)
    {
        return self.getGlobalBounds().getTop();
    }


    ////////////////////////////////////////////////////////////
    /// \brief World Y coordinate of the bottom edge
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getGlobalBottom(this const auto& self)
    {
        return self.getGlobalBounds().getBottom();
    }


    ////////////////////////////////////////////////////////////
    /// \brief World X coordinate of the center
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getGlobalCenterX(this const auto& self)
    {
        return self.getGlobalBounds().getCenter().x;
    }


    ////////////////////////////////////////////////////////////
    /// \brief World Y coordinate of the center
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getGlobalCenterY(this const auto& self)
    {
        return self.getGlobalBounds().getCenter().y;
    }


    ////////////////////////////////////////////////////////////
    /// \brief World width of the object
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getGlobalWidth(this const auto& self)
    {
        return self.getGlobalBounds().size.x;
    }


    ////////////////////////////////////////////////////////////
    /// \brief World height of the object
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getGlobalHeight(this const auto& self)
    {
        return self.getGlobalBounds().size.y;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Move the object so the anchor point at `factors` lands at `newPosition`
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
        requires priv::GlobalAnchorPositionable<Self>
    [[gnu::always_inline, gnu::flatten]]
    inline constexpr void setGlobalAnchorPoint(this Self& self, const Vec2f factors, const Vec2f newPosition)
    {
        addPositionImpl(self, newPosition - self.getGlobalBounds().getAnchorPoint(factors));
    }


////////////////////////////////////////////////////////////
#define ZA_PRIV_DEFINE_MIXIN_SETTER(name, rectGetter)                                                         \
    template <typename Self>                                                                                  \
        requires priv::GlobalAnchorPositionable<Self>                                                         \
    [[gnu::always_inline, gnu::flatten]] inline constexpr void name(this Self& self, const Vec2f newPosition) \
    {                                                                                                         \
        addPositionImpl(self, newPosition - self.getGlobalBounds().rectGetter());                             \
    }

    ////////////////////////////////////////////////////////////
    /// \brief Set the world position of the top-left anchor
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_SETTER(setGlobalTopLeft, getTopLeft);

    ////////////////////////////////////////////////////////////
    /// \brief Set the world position of the top-center anchor
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_SETTER(setGlobalTopCenter, getTopCenter);

    ////////////////////////////////////////////////////////////
    /// \brief Set the world position of the top-right anchor
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_SETTER(setGlobalTopRight, getTopRight);

    ////////////////////////////////////////////////////////////
    /// \brief Set the world position of the center-left anchor
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_SETTER(setGlobalCenterLeft, getCenterLeft);

    ////////////////////////////////////////////////////////////
    /// \brief Set the world position of the center anchor
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_SETTER(setGlobalCenter, getCenter);

    ////////////////////////////////////////////////////////////
    /// \brief Set the world position of the center-right anchor
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_SETTER(setGlobalCenterRight, getCenterRight);

    ////////////////////////////////////////////////////////////
    /// \brief Set the world position of the bottom-left anchor
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_SETTER(setGlobalBottomLeft, getBottomLeft);

    ////////////////////////////////////////////////////////////
    /// \brief Set the world position of the bottom-center anchor
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_SETTER(setGlobalBottomCenter, getBottomCenter);

    ////////////////////////////////////////////////////////////
    /// \brief Set the world position of the bottom-right anchor
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_SETTER(setGlobalBottomRight, getBottomRight);

#undef ZA_PRIV_DEFINE_MIXIN_SETTER


    ////////////////////////////////////////////////////////////
    /// \brief Move the object horizontally so its left edge lands at `newCoordinate`
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
        requires priv::GlobalAnchorPositionable<Self>
    [[gnu::always_inline, gnu::flatten]]
    inline constexpr void setGlobalLeft(this Self& self, const float newCoordinate)
    {
        addPositionImpl(self, Vec2f{newCoordinate - self.getGlobalBounds().getLeft(), 0.f});
    }


    ////////////////////////////////////////////////////////////
    /// \brief Move the object horizontally so its right edge lands at `newCoordinate`
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
        requires priv::GlobalAnchorPositionable<Self>
    [[gnu::always_inline, gnu::flatten]]
    inline constexpr void setGlobalRight(this Self& self, const float newCoordinate)
    {
        addPositionImpl(self, Vec2f{newCoordinate - self.getGlobalBounds().getRight(), 0.f});
    }


    ////////////////////////////////////////////////////////////
    /// \brief Move the object vertically so its top edge lands at `newCoordinate`
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
        requires priv::GlobalAnchorPositionable<Self>
    [[gnu::always_inline, gnu::flatten]]
    inline constexpr void setGlobalTop(this Self& self, const float newCoordinate)
    {
        addPositionImpl(self, Vec2f{0.f, newCoordinate - self.getGlobalBounds().getTop()});
    }


    ////////////////////////////////////////////////////////////
    /// \brief Move the object vertically so its bottom edge lands at `newCoordinate`
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
        requires priv::GlobalAnchorPositionable<Self>
    [[gnu::always_inline, gnu::flatten]]
    inline constexpr void setGlobalBottom(this Self& self, const float newCoordinate)
    {
        addPositionImpl(self, Vec2f{0.f, newCoordinate - self.getGlobalBounds().getBottom()});
    }


    ////////////////////////////////////////////////////////////
    /// \brief Move the object horizontally so its center lands at `newCoordinate`
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
        requires priv::GlobalAnchorPositionable<Self>
    [[gnu::always_inline, gnu::flatten]]
    inline constexpr void setGlobalCenterX(this Self& self, const float newCoordinate)
    {
        addPositionImpl(self, Vec2f{newCoordinate - self.getGlobalBounds().getCenter().x, 0.f});
    }


    ////////////////////////////////////////////////////////////
    /// \brief Move the object vertically so its center lands at `newCoordinate`
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
        requires priv::GlobalAnchorPositionable<Self>
    [[gnu::always_inline, gnu::flatten]]
    inline constexpr void setGlobalCenterY(this Self& self, const float newCoordinate)
    {
        addPositionImpl(self, Vec2f{0.f, newCoordinate - self.getGlobalBounds().getCenter().y});
    }

private:
    ////////////////////////////////////////////////////////////
    /// \brief Helper that implicitly converts a `Vec2f` to any `Vec2<U>`
    ///
    /// Used by `addPositionImpl` to feed a computed `Vec2f` position back
    /// into the inheriting class's `setPosition()`, whatever coordinate
    /// type it uses.
    ///
    /// Integral coordinates are rounded to the nearest integer (halves
    /// upwards), so the error is at most half a unit and does not depend
    /// on the sign of the coordinate, unlike a truncating conversion.
    ///
    ////////////////////////////////////////////////////////////
    struct AutoConvertingVec2f
    {
        Vec2f data;

        template <typename U>
        [[nodiscard, gnu::always_inline, gnu::flatten]] constexpr operator Vec2<U>() const
        {
            if constexpr (isIntegral<U>)
                return Vec2<U>{static_cast<U>(ZA_MATH_FLOORF(data.x + 0.5f)),
                               static_cast<U>(ZA_MATH_FLOORF(data.y + 0.5f))};
            else
                return data.to<Vec2<U>>();
        }
    };


    ////////////////////////////////////////////////////////////
    /// \brief Add a world-space `offset` to the inheriting object's position
    ///
    /// Mutates an accessible `position` data member in place if there is
    /// one, otherwise calls `setPosition(getPosition() + offset)` (see
    /// `priv::GlobalAnchorPositionable`).
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
    [[gnu::always_inline, gnu::flatten]]
    static inline constexpr void addPositionImpl(Self& self, const Vec2f offset)
    {
        if constexpr (requires { self.position += offset; })
            self.position += offset;
        else
            self.setPosition(AutoConvertingVec2f{self.getPosition().toVec2f() + offset});
    }
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::GlobalAnchorPointMixin
/// \ingroup system
///
/// Utility mixin providing convenient functions to get and set the
/// position of an object through anchor points of its global (world)
/// bounds:
///
/// - Corners, edge centers, and the center, e.g. `getGlobalTopLeft()`,
///   `setGlobalBottomRight(p)`, `getGlobalCenterLeft()`, `setGlobalCenter(p)`.
///
/// - Arbitrary anchor points, via normalized factors in `[0, 1]`:
///   `getGlobalAnchorPoint(factors)`, `setGlobalAnchorPoint(factors, p)`.
///
/// - Individual coordinates: `getGlobalLeft()`, `setGlobalRight(x)`,
///   `getGlobalCenterX()`, `setGlobalCenterY(y)`, etc.
///
/// - Size: `getGlobalWidth()`, `getGlobalHeight()`.
///
/// Setters move the object without resizing it, so its global bounds
/// keep their size (this also holds for rotated or scaled objects,
/// whose global bounds are their axis-aligned bounding box).
///
/// To use this mixin, inherit from it publicly, e.g.:
/// `struct MyObject : za::TransformableMixin, za::GlobalAnchorPointMixin`
///
/// The inheriting class must provide `getGlobalBounds()`. The setters
/// also need either an accessible `za::Vec2f position` data member, or
/// `getPosition()` and `setPosition()`: otherwise they do not compile.
/// Integral positions (e.g. `za::Vec2i` window positions) are rounded
/// to the nearest integer.
///
/// \see `za::LocalAnchorPointMixin`, `za::TransformableMixin`
///
////////////////////////////////////////////////////////////
