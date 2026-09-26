#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/BlendMode.hpp"
#include "Zancle/Graphics/StencilMode.hpp"
#include "Zancle/Graphics/Transform.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/Base/RequireDesignatedInitializers.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class Shader;
class Texture;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Define the states used for drawing to a `RenderTarget`
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_GRAPHICS_API RenderStates
{
    ZA_REQUIRE_DESIGNATED_INITIALIZERS;

    BlendMode   blendMode{};   //!< Blending mode
    StencilMode stencilMode{}; //!< Stencil mode

    // NOLINTNEXTLINE(readability-redundant-member-init)
    Transform transform{}; //!< Transform

    View view{}; //!< View (computed from the render target's size if unset)

    const Texture* texture{nullptr}; //!< Texture
    const Shader*  shader{nullptr};  //!< Shader

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr bool operator==(const RenderStates&) const = default;
};

} // namespace za


////////////////////////////////////////////////////////////
/// \struct za::RenderStates
/// \ingroup graphics
///
/// `za::RenderStates` is the bag of state passed to every
/// `za::RenderTarget::draw` call. It is an aggregate of six
/// public members:
///
/// \li `blendMode`   -- how pixels of the object are blended with the background
/// \li `stencilMode` -- how pixels of the object interact with the stencil buffer
/// \li `transform`   -- how the object is positioned, rotated, and scaled
/// \li `view`        -- which view (camera) is active for this draw call
/// \li `texture`     -- which texture is sampled by the shader
/// \li `shader`      -- which custom shader runs (if any)
///
/// High-level objects such as `za::Sprite`, `za::Shape`, and
/// `za::Text` may need some of these to be set up before drawing.
/// In particular, `za::Sprite` and `za::Shape` do not own a
/// texture in Zancle: you must either pass the texture as a draw
/// argument or assign it to `RenderStates::texture` yourself.
///
/// The transform is special: drawables that have their own
/// `transform` (sprites, texts, shapes, ...) combine their local
/// transform with the one passed in `RenderStates`, so a "global"
/// transform composes naturally on top of every per-object
/// transform.
///
/// `za::RenderStates` is an aggregate, so it is most often
/// constructed inline with C++20 designated initializers:
///
/// \code
/// // Texture via render states, defaults for everything else.
/// window.draw(sprite, {.texture = &texture});
///
/// // Custom shader, defaults for everything else.
/// window.draw(sprite, {.texture = &texture, .shader = &myShader});
///
/// // Custom view + custom blend mode.
/// window.draw(shape, {.blendMode = za::BlendAdd, .view = myView});
/// \endcode
///
/// \see `za::RenderTarget`, `za::BlendMode`, `za::StencilMode`,
///      `za::Transform`, `za::View`, `za::Shader`, `za::Texture`
///
////////////////////////////////////////////////////////////
