#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/RenderTextureCreateSettings.hpp"
#include "Zancle/Graphics/TextureWrapMode.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/InPlacePImpl.hpp"
#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class Texture;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Target for off-screen 2D rendering into a texture
///
////////////////////////////////////////////////////////////
class ZA_GRAPHICS_API RenderTexture : public RenderTarget
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~RenderTexture() override;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    RenderTexture(const RenderTexture&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    RenderTexture& operator=(const RenderTexture&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    RenderTexture(RenderTexture&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment operator
    ///
    ////////////////////////////////////////////////////////////
    RenderTexture& operator=(RenderTexture&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Create a render texture of the given size
    ///
    /// Equivalent to calling the two-argument overload with
    /// default `RenderTextureCreateSettings`.
    ///
    /// After creation, the contents of the render texture are
    /// undefined. Call `clear` before drawing to fill it.
    ///
    /// \param size Width and height of the render texture, in pixels
    ///
    /// \return Render texture on success, `za::nullOpt` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<RenderTexture> create(Vec2u size);

    ////////////////////////////////////////////////////////////
    /// \brief Create a render texture of the given size with custom framebuffer settings
    ///
    /// `rtCreateSettings` lets you enable multisampling, request
    /// a depth or stencil attachment, opt into sRGB encoding,
    /// configure smoothing, or set the texture wrap mode. The
    /// defaults match the simple `create(size)` overload.
    ///
    /// After creation, the contents of the render texture are
    /// undefined. Call `clear` before drawing to fill it.
    ///
    /// \param size             Width and height of the render texture, in pixels
    /// \param rtCreateSettings Framebuffer object creation parameters
    ///
    /// \return Render texture on success, `za::nullOpt` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<RenderTexture> create(Vec2u size, const RenderTextureCreateSettings& rtCreateSettings);

    ////////////////////////////////////////////////////////////
    /// \brief Get the maximum anti-aliasing level supported by the system
    ///
    /// \return The maximum anti-aliasing level supported by the system
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int getMaximumSampleCount();

    ////////////////////////////////////////////////////////////
    /// \brief Enable or disable texture smoothing
    ///
    /// This function is similar to `Texture::setSmooth`.
    /// This parameter is disabled by default.
    ///
    /// \param smooth `true` to enable smoothing, `false` to disable it
    ///
    /// \see `isSmooth`
    ///
    ////////////////////////////////////////////////////////////
    void setSmooth(bool smooth);

    ////////////////////////////////////////////////////////////
    /// \brief Tell whether the smooth filtering is enabled or not
    ///
    /// \return `true` if texture smoothing is enabled
    ///
    /// \see `setSmooth`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isSmooth() const;

    ////////////////////////////////////////////////////////////
    /// \brief Set the wrap mode used when sampling the underlying texture
    ///
    /// Same semantics as `za::Texture::setWrapMode`. The default
    /// is `TextureWrapMode::Clamp`.
    ///
    /// \param wrapMode Wrap mode to apply
    ///
    /// \see `getWrapMode`, `za::TextureWrapMode`
    ///
    ////////////////////////////////////////////////////////////
    void setWrapMode(TextureWrapMode wrapMode);

    ////////////////////////////////////////////////////////////
    /// \brief Get the wrap mode currently set on the underlying texture
    ///
    /// \return Active `za::TextureWrapMode`
    ///
    /// \see `setWrapMode`, `za::TextureWrapMode`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] TextureWrapMode getWrapMode() const;

    ////////////////////////////////////////////////////////////
    /// \brief Generate a mipmap using the current texture data
    ///
    /// This function is similar to `Texture::generateMipmap` and operates
    /// on the texture used as the target for drawing.
    /// Be aware that any draw operation may modify the base level image data.
    /// For this reason, calling this function only makes sense after all
    /// drawing is completed and display has been called. Not calling display
    /// after subsequent drawing will lead to undefined behavior if a mipmap
    /// had been previously generated.
    ///
    ////////////////////////////////////////////////////////////
    void generateMipmap();

    ////////////////////////////////////////////////////////////
    /// \brief Activate or deactivate the render-texture for rendering
    ///
    /// This function makes the render-texture's context current for
    /// future OpenGL rendering operations (so you shouldn't care
    /// about it if you're not doing direct OpenGL stuff).
    /// Only one context can be current in a thread, so if you
    /// want to draw OpenGL geometry to another render target
    /// (like a RenderWindow) don't forget to activate it again.
    ///
    /// \param active `true` to activate, `false` to deactivate
    ///
    /// \return `true` if operation was successful, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool setActive(bool active = true) override;

    ////////////////////////////////////////////////////////////
    /// \brief Update the contents of the target texture
    ///
    /// This function updates the target texture with what
    /// has been drawn so far. Like for windows, calling this
    /// function is mandatory at the end of rendering. Not calling
    /// it may leave the texture in an undefined state.
    ///
    ////////////////////////////////////////////////////////////
    RenderTarget::DrawStatistics display();

    ////////////////////////////////////////////////////////////
    /// \brief Return the size of the rendering region of the texture
    ///
    /// The returned value is the size that you passed to
    /// the create function.
    ///
    /// \return Size in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vec2u getSize() const override;

    ////////////////////////////////////////////////////////////
    /// \brief Get a read-only reference to the target texture
    ///
    /// After drawing to the render-texture and calling Display,
    /// you can retrieve the updated texture using this function,
    /// and draw it using a sprite (for example).
    /// The internal `za::Texture` of a render-texture is always the
    /// same instance, so that it is possible to call this function
    /// once and keep a reference to the texture even after it is
    /// modified.
    ///
    /// \return Const reference to the texture
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Texture& getTexture() const;

    ////////////////////////////////////////////////////////////
    /// \private
    ///
    /// \brief Construct from texture
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit RenderTexture(za::PassKey<RenderTexture>&&, Texture&& texture);

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 384> m_impl; //!< Implementation details
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::RenderTexture
/// \ingroup graphics
///
/// `za::RenderTexture` is the little brother of `za::RenderWindow`.
/// It implements the same 2D drawing and OpenGL-related functions
/// (see their base class `za::RenderTarget` for more details),
/// the difference is that the result is stored in an off-screen
/// texture rather than being show in a window.
///
/// Rendering to a texture can be useful in a variety of situations:
/// \li precomputing a complex static texture (like a level's background from multiple tiles)
/// \li applying post-effects to the whole scene with shaders
/// \li creating a sprite from a 3D object rendered with OpenGL
/// \li etc.
///
/// Usage example:
///
/// \code
/// // Create a new render-window
/// auto window = za::RenderWindow::create({.size{800u, 600u}, .title = "Zancle Window"}).value();
///
/// // Create a new render-texture
/// auto renderTexture = za::RenderTexture::create({500, 500}).value();
///
/// // The main loop
/// while (true)
/// {
///    // Event processing
///    // ...
///
///    // Clear the whole texture with red color
///    renderTexture.clear(za::Color::Red);
///
///    // Draw stuff to the texture
///    renderTexture.draw(shape);   // shape is a za::Shape
///    renderTexture.draw(text);    // text is a za::Text
///
///    // We're done drawing to the texture
///    renderTexture.display();
///
///    // Now we start rendering to the window, clear it first
///    window.clear();
///
///    // Draw the texture
///    window.draw(renderTexture.getTexture());
///
///    // End the current frame and display its contents on screen
///    window.display();
/// }
/// \endcode
///
/// Like `za::RenderWindow`, `za::RenderTexture` is still able to render direct
/// OpenGL stuff. It is even possible to mix together OpenGL calls
/// and regular Zancle drawing commands. If you need a depth buffer for
/// 3D rendering, don't forget to request it when calling `RenderTexture::create`.
///
/// \see `za::RenderTarget`, `za::RenderWindow`, `za::View`, `za::Texture`
///
////////////////////////////////////////////////////////////
