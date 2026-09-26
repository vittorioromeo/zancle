#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/Priv/TransformableMacros.hpp"
#include "Zancle/Graphics/TransformableMixin.hpp"

#include "Zancle/Mixin/GlobalAnchorPointMixin.hpp"
#include "Zancle/Mixin/LocalAnchorPointMixin.hpp"

#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Base/RequireDesignatedInitializers.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Geometry that can render a texture, with its
///        own transformations, color, etc.
///
////////////////////////////////////////////////////////////
struct ZA_GRAPHICS_API Sprite : TransformableMixin, GlobalAnchorPointMixin, LocalAnchorPointMixin
{
    ////////////////////////////////////////////////////////////
    /// \brief Get the local bounding rectangle of the entity
    ///
    /// The returned rectangle is in local coordinates, which means
    /// that it ignores the transformations (translation, rotation,
    /// scale, etc...) that are applied to the entity.
    /// In other words, this function returns the bounds of the
    /// entity in the entity's coordinate system.
    ///
    /// \return Local bounding rectangle of the entity
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] Rect2f getLocalBounds() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the global bounding rectangle of the entity
    ///
    /// The returned rectangle is in global coordinates, which means
    /// that it takes into account the transformations (translation,
    /// rotation, scale, etc...) that are applied to the entity.
    /// In other words, this function returns the bounds of the
    /// sprite in the global 2D world's coordinate system.
    ///
    /// \return Global bounding rectangle of the entity
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] Rect2f getGlobalBounds() const;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    ZA_REQUIRE_DESIGNATED_INITIALIZERS;

    ZA_DEFINE_TRANSFORMABLE_DATA_MEMBERS;
    Rect2f textureRect;         //!< Rectangle defining the area of the source texture to display
    Color  color{Color::White}; //!< Color of the sprite
};

} // namespace za


////////////////////////////////////////////////////////////
/// \struct za::Sprite
/// \ingroup graphics
///
/// `za::Sprite` is a lightweight aggregate that knows how to
/// draw a textured quad with its own transform and color tint.
/// It is the building block for displaying images on screen.
///
/// Like all transformable types in Zancle, `za::Sprite` exposes
/// the standard public members `position`, `scale`, `origin`,
/// and `rotation` (via `ZA_DEFINE_TRANSFORMABLE_DATA_MEMBERS`),
/// plus the sprite-specific `textureRect` (sub-rectangle of the
/// source texture to display) and `color` (modulation tint).
///
/// **Lifetime safety:** unlike upstream SFML, `za::Sprite` does
/// **not** store a pointer to its texture. The texture is
/// supplied at draw time -- either as an extra argument to
/// `za::RenderTarget::draw` or via `za::RenderStates::texture`.
/// This eliminates the classic "white square" problem where a
/// sprite outlives its texture.
///
/// Because `za::Sprite` is an aggregate, it is best constructed
/// inline with C++20 designated initializers.
///
/// See also the note on coordinates and undistorted rendering
/// in `za::Transformable`.
///
/// Usage example:
/// \code
/// // Load a texture.
/// const auto texture = za::Texture::loadFromFile("texture.png").value();
///
/// // Create a sprite that displays a sub-rectangle of the texture
/// // with a custom color tint.
/// const za::Sprite sprite{
///     .position    = {100.f, 25.f},
///     .textureRect = {{10.f, 10.f}, {50.f, 30.f}},
///     .color       = {255u, 255u, 255u, 200u},
/// };
///
/// // Draw it. The texture is passed at draw time via render states.
/// window.draw(sprite, {.texture = &texture});
/// \endcode
///
/// \see `za::Texture`, `za::Transformable`, `za::RenderStates`
///
////////////////////////////////////////////////////////////
