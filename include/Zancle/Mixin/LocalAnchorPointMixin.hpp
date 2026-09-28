#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/Priv/Vec2Base.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Mixin to get anchor points of an object's local bounds
///
/// See the class documentation at the end of this file.
///
////////////////////////////////////////////////////////////
struct LocalAnchorPointMixin
{
    ////////////////////////////////////////////////////////////
    /// \brief Local coordinates of an anchor point given normalized `factors` in `[0, 1]`
    ///
    /// `(0, 0)` is the top-left, `(1, 1)` is the bottom-right.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr auto getLocalAnchorPoint(this const auto& self, const Vec2f factors)
    {
        return self.getLocalBounds().getAnchorPoint(factors);
    }


////////////////////////////////////////////////////////////
#define ZA_PRIV_DEFINE_MIXIN_GETTER(name, rectGetter)                                                            \
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr auto name(this auto const& self) \
    {                                                                                                            \
        return self.getLocalBounds().rectGetter();                                                               \
    }

    ////////////////////////////////////////////////////////////
    /// \brief Get the local position of the top-left anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getLocalTopLeft, getTopLeft);

    ////////////////////////////////////////////////////////////
    /// \brief Get the local position of the top-center anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getLocalTopCenter, getTopCenter);

    ////////////////////////////////////////////////////////////
    /// \brief Get the local position of the top-right anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getLocalTopRight, getTopRight);

    ////////////////////////////////////////////////////////////
    /// \brief Get the local position of the center-left anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getLocalCenterLeft, getCenterLeft);

    ////////////////////////////////////////////////////////////
    /// \brief Get the local position of the center anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getLocalCenter, getCenter);

    ////////////////////////////////////////////////////////////
    /// \brief Get the local position of the center-right anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getLocalCenterRight, getCenterRight);

    ////////////////////////////////////////////////////////////
    /// \brief Get the local position of the bottom-left anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getLocalBottomLeft, getBottomLeft);

    ////////////////////////////////////////////////////////////
    /// \brief Get the local position of the bottom-center anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getLocalBottomCenter, getBottomCenter);

    ////////////////////////////////////////////////////////////
    /// \brief Get the local position of the bottom-right anchor point
    ///
    ////////////////////////////////////////////////////////////
    ZA_PRIV_DEFINE_MIXIN_GETTER(getLocalBottomRight, getBottomRight);

#undef ZA_PRIV_DEFINE_MIXIN_GETTER


    ////////////////////////////////////////////////////////////
    /// \brief Local X coordinate of the left edge
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getLocalLeft(this const auto& self)
    {
        return self.getLocalBounds().getLeft();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Local X coordinate of the right edge
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getLocalRight(this const auto& self)
    {
        return self.getLocalBounds().getRight();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Local Y coordinate of the top edge
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getLocalTop(this const auto& self)
    {
        return self.getLocalBounds().getTop();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Local Y coordinate of the bottom edge
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getLocalBottom(this const auto& self)
    {
        return self.getLocalBounds().getBottom();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Local X coordinate of the center
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getLocalCenterX(this const auto& self)
    {
        return self.getLocalBounds().getCenter().x;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Local Y coordinate of the center
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getLocalCenterY(this const auto& self)
    {
        return self.getLocalBounds().getCenter().y;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Local width of the object
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getLocalWidth(this const auto& self)
    {
        return self.getLocalBounds().size.x;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Local height of the object
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]]
    inline constexpr float getLocalHeight(this const auto& self)
    {
        return self.getLocalBounds().size.y;
    }
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::LocalAnchorPointMixin
/// \ingroup system
///
/// Utility mixin providing convenient functions to get anchor points
/// of an object's local bounds, i.e. in the object's own coordinate
/// system, before its transform is applied:
///
/// - Corners, edge centers, and the center, e.g. `getLocalTopLeft()`,
///   `getLocalBottomRight()`, `getLocalCenterLeft()`, `getLocalCenter()`.
///
/// - Arbitrary anchor points, via normalized factors in `[0, 1]`:
///   `getLocalAnchorPoint(factors)`.
///
/// - Individual coordinates and size: `getLocalLeft()`, `getLocalRight()`,
///   `getLocalCenterX()`, `getLocalWidth()`, etc.
///
/// A typical use is choosing a transform origin, e.g.
/// `sprite.origin = sprite.getLocalCenter();`. There are no setters:
/// local bounds are defined by the object's contents, not its position.
/// Use `za::GlobalAnchorPointMixin` to position objects.
///
/// To use this mixin, inherit from it publicly, e.g.:
/// `struct MyObject : za::TransformableMixin, za::LocalAnchorPointMixin`
///
/// The inheriting class must provide `getLocalBounds()`.
///
/// \see `za::GlobalAnchorPointMixin`, `za::TransformableMixin`
///
////////////////////////////////////////////////////////////
