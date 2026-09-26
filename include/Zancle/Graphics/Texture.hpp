#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/TextureWrapMode.hpp"

#include "Zancle/Lifetime/LifetimeDependee.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
struct Color;
class GlyphMappedText;
class Image;
class InputStream;
class Path;
class TextureAtlas;
class Window;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Optional parameters for `za::Texture::create`
///
/// All members default to sensible values, so the struct can be
/// passed empty (e.g. via designated initializers).
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] TextureCreateSettings
{
    bool            sRgb     = false;                  //!< Whether the texture should be created in sRGB color space
    bool            smooth   = false;                  //!< Whether linear filtering should be enabled
    TextureWrapMode wrapMode = TextureWrapMode::Clamp; //!< Wrap mode used when sampling the texture
};

////////////////////////////////////////////////////////////
/// \brief Optional parameters for `za::Texture::loadFromFile` and friends
///
/// In addition to the `TextureCreateSettings` fields, this struct
/// has an `area` member that lets you load only a sub-rectangle of
/// the source image. Pass an empty `Rect2i` to load the whole
/// image (the default).
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] TextureLoadSettings
{
    bool            sRgb     = false;                  //!< Whether the texture should be created in sRGB color space
    bool            smooth   = false;                  //!< Whether linear filtering should be enabled
    TextureWrapMode wrapMode = TextureWrapMode::Clamp; //!< Wrap mode used when sampling the texture

    Rect2i area = {}; //!< Sub-rectangle of the source image to load (`{}` = full image)
};

////////////////////////////////////////////////////////////
/// \brief Image living on the graphics card that can be used for drawing
///
////////////////////////////////////////////////////////////
class ZA_GRAPHICS_API Texture
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~Texture();

    ////////////////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    ////////////////////////////////////////////////////////////
    Texture(const Texture& rhs);

    ////////////////////////////////////////////////////////////
    /// \brief Copy assignment operator
    ///
    ////////////////////////////////////////////////////////////
    Texture& operator=(const Texture&);

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    Texture(Texture&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment operator
    ///
    ////////////////////////////////////////////////////////////
    Texture& operator=(Texture&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Create an empty texture of the given size
    ///
    /// The contents of a freshly created texture are undefined.
    /// Use `update` to upload pixel data, or `clear` to fill the
    /// texture with a uniform color.
    ///
    /// \param size     Width and height of the texture
    /// \param settings Texture create settings (sRGB, smoothing, wrap mode)
    ///
    /// \return Texture on success, `za::nullOpt` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Texture> create(Vec2u size, const TextureCreateSettings& settings = {});

    ////////////////////////////////////////////////////////////
    /// \brief Load a texture from an image file on disk
    ///
    /// `settings.area` can be used to load only a sub-rectangle of
    /// the source image. An `area` with a zero-sized dimension (the
    /// default), or one that already covers the whole image, loads
    /// the entire image. Otherwise the rectangle is clipped to the
    /// image bounds (negative `position` components are treated as
    /// `0`; `size` components must be positive, debug-asserted).
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with `getMaximumSize`.
    ///
    /// \param filename Path of the image file to load
    /// \param settings Texture load settings (sRGB, smoothing, wrap mode, sub-area)
    ///
    /// \return Texture on success, `za::nullOpt` on failure
    ///
    /// \see `loadFromMemory`, `loadFromStream`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Texture> loadFromFile(const Path& filename, const TextureLoadSettings& settings = {});

    ////////////////////////////////////////////////////////////
    /// \brief Load a texture from an image file held in memory
    ///
    /// Decodes the image bytes pointed to by `data`/`size` (PNG,
    /// JPEG, etc.) and uploads the result to the GPU. See
    /// `loadFromFile` for the meaning of `settings.area`.
    ///
    /// \param data     Pointer to the encoded file bytes in memory
    /// \param size     Size of the data, in bytes
    /// \param settings Texture load settings (sRGB, smoothing, wrap mode, sub-area)
    ///
    /// \return Texture on success, `za::nullOpt` on failure
    ///
    /// \see `loadFromFile`, `loadFromStream`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Texture> loadFromMemory(const void*                data,
                                                              za::SizeT                  size,
                                                              const TextureLoadSettings& settings = {});

    ////////////////////////////////////////////////////////////
    /// \brief Load a texture from a custom input stream
    ///
    /// Decodes the image bytes read from `stream` and uploads the
    /// result to the GPU. See `loadFromFile` for the meaning of
    /// `settings.area`.
    ///
    /// \param stream   Source stream to read encoded image data from
    /// \param settings Texture load settings (sRGB, smoothing, wrap mode, sub-area)
    ///
    /// \return Texture on success, `za::nullOpt` on failure
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Texture> loadFromStream(InputStream& stream, const TextureLoadSettings& settings = {});

    ////////////////////////////////////////////////////////////
    /// \brief Load a texture from an existing `za::Image`
    ///
    /// Uploads the pixels of `image` to the GPU. See `loadFromFile`
    /// for the meaning of `settings.area`. This is the preferred
    /// path when you need to perform CPU-side processing on the
    /// pixels before uploading them.
    ///
    /// \param image    Image whose pixels will be uploaded
    /// \param settings Texture load settings (sRGB, smoothing, wrap mode, sub-area)
    ///
    /// \return Texture on success, `za::nullOpt` on failure
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `loadFromStream`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Texture> loadFromImage(const Image& image, const TextureLoadSettings& settings = {});

    ////////////////////////////////////////////////////////////
    /// \brief Return the size of the texture
    ///
    /// \return Size in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vec2u getSize() const;

    ////////////////////////////////////////////////////////////
    /// \brief Copy the texture pixels to an image
    ///
    /// This function performs a slow operation that downloads
    /// the texture's pixels from the graphics card and copies
    /// them to a new image, potentially applying transformations
    /// to pixels if necessary (texture may be padded).
    ///
    /// \return Image containing the texture's pixels
    ///
    /// \see `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Image copyToImage() const;

    ////////////////////////////////////////////////////////////
    /// \brief Fill the whole texture with `Color::Transparent`
    ///
    /// Equivalent to `clear(Color::Transparent)`.
    ///
    /// \return `true` on success, `false` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool clear();

    ////////////////////////////////////////////////////////////
    /// \brief Fill the whole texture with `color` on the GPU
    ///
    /// Every texel is overwritten: like a full-texture `update`,
    /// this invalidates any generated mipmap and breaks in-flight
    /// batched draws referencing this texture.
    ///
    /// \param color Fill color
    ///
    /// \return `true` on success, `false` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool clear(Color color);

    ////////////////////////////////////////////////////////////
    /// \brief Update the whole texture from an array of pixels
    ///
    /// The pixel array is assumed to have the same size as
    /// the texture, and to contain 32-bits RGBA pixels.
    ///
    /// No additional check is performed on the size of the pixel
    /// array. Passing invalid arguments will lead to an undefined
    /// behavior.
    ///
    /// `pixels` must not be `nullptr` (asserted in debug builds;
    /// undefined behavior in release builds).
    ///
    /// \param pixels Array of pixels to copy to the texture
    ///
    ////////////////////////////////////////////////////////////
    void update(const za::U8* pixels);

    ////////////////////////////////////////////////////////////
    /// \brief Update a part of the texture from an array of pixels
    ///
    /// The size of the pixel array must match the `size` argument,
    /// and it must contain 32-bits RGBA pixels.
    ///
    /// No additional check is performed on the size of the pixel
    /// array or the bounds of the area to update. Passing invalid
    /// arguments will lead to an undefined behavior.
    ///
    /// `pixels` must not be `nullptr` (asserted in debug builds;
    /// undefined behavior in release builds).
    ///
    /// \param pixels Array of pixels to copy to the texture
    /// \param size   Width and height of the pixel region contained in `pixels`
    /// \param dest   Coordinates of the destination position
    ///
    ////////////////////////////////////////////////////////////
    void update(const za::U8* pixels, Vec2u size, Vec2u dest);

    ////////////////////////////////////////////////////////////
    /// \brief Update a part of this texture from another texture
    ///
    /// No additional check is performed on the size of the texture.
    /// Passing an invalid combination of texture size and destination
    /// will lead to an undefined behavior.
    ///
    /// This function does nothing if either texture was not
    /// previously created.
    ///
    /// \param texture Source texture to copy to this texture
    /// \param dest    Coordinates of the destination position
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool update(const Texture& texture, Vec2u dest = {0u, 0u});

    ////////////////////////////////////////////////////////////
    /// \brief Update a part of the texture from an image
    ///
    /// No additional check is performed on the size of the image.
    /// Passing an invalid combination of image size and destination
    /// will lead to an undefined behavior.
    ///
    /// This function does nothing if the texture was not
    /// previously created.
    ///
    /// \param image Image to copy to the texture
    /// \param dest  Coordinates of the destination position
    ///
    ////////////////////////////////////////////////////////////
    void update(const Image& image, Vec2u dest = {0u, 0u});

    ////////////////////////////////////////////////////////////
    /// \brief Update a part of the texture from the contents of a window
    ///
    /// No additional check is performed on the size of the window.
    /// Passing an invalid combination of window size and destination
    /// will lead to an undefined behavior.
    ///
    /// Must be called before `window.display()`.
    ///
    /// As a side effect, `window`'s OpenGL context is made current
    /// and remains current after this function returns.
    ///
    /// This function does nothing if either the texture or the window
    /// was not previously created.
    ///
    /// \param window Window to copy to the texture
    /// \param dest   Coordinates of the destination position
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool update(const Window& window, Vec2u dest = {0u, 0u});

    ////////////////////////////////////////////////////////////
    /// \brief Enable or disable the smooth filter
    ///
    /// When the filter is activated, the texture appears smoother
    /// so that pixels are less noticeable. However if you want
    /// the texture to look exactly the same as its source file,
    /// you should leave it disabled.
    /// The smooth filter is disabled by default.
    ///
    /// \param smooth `true` to enable smoothing, `false` to disable it
    ///
    /// \see `isSmooth`
    ///
    ////////////////////////////////////////////////////////////
    void setSmooth(bool smooth);

    ////////////////////////////////////////////////////////////
    /// \brief Tell whether the smooth filter is enabled or not
    ///
    /// \return `true` if smoothing is enabled, `false` if it is disabled
    ///
    /// \see `setSmooth`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isSmooth() const;

    ////////////////////////////////////////////////////////////
    /// \brief Tell whether the texture source is converted from sRGB or not
    ///
    /// \return `true` if the texture source is converted from sRGB, `false` if not
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isSrgb() const;

    ////////////////////////////////////////////////////////////
    /// \brief Set the wrap mode used when sampling texture coordinates outside `[0, 1]`
    ///
    /// The wrap mode determines what happens when texture
    /// coordinates fall outside the canonical `[0, 1]` range:
    /// they can be clamped to the edge texel, repeated, or
    /// mirror-repeated. See `za::TextureWrapMode` for details.
    ///
    /// The default wrap mode is `TextureWrapMode::Clamp`.
    ///
    /// \param wrapMode New wrap mode to apply
    ///
    /// \see `getWrapMode`, `za::TextureWrapMode`
    ///
    ////////////////////////////////////////////////////////////
    void setWrapMode(TextureWrapMode wrapMode);

    ////////////////////////////////////////////////////////////
    /// \brief Get the wrap mode currently set on this texture
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
    /// Mipmaps are pre-computed chains of optimized textures. Each
    /// level of texture in a mipmap is generated by halving each of
    /// the previous level's dimensions. This is done until the final
    /// level has the size of 1x1. The textures generated in this process may
    /// make use of more advanced filters which might improve the visual quality
    /// of textures when they are applied to objects much smaller than they are.
    /// This is known as minification. Because fewer texels (texture elements)
    /// have to be sampled from when heavily minified, usage of mipmaps
    /// can also improve rendering performance in certain scenarios.
    ///
    /// Mipmap data is only valid from the time it is generated until
    /// the next time the base level image is modified, at which point
    /// this function will have to be called again to regenerate it.
    ///
    ////////////////////////////////////////////////////////////
    void generateMipmap();

    ////////////////////////////////////////////////////////////
    /// \brief Swap the contents of this texture with those of another
    ///
    /// \param right Instance to swap with
    ///
    ////////////////////////////////////////////////////////////
    void swap(Texture& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Get the underlying OpenGL handle of the texture.
    ///
    /// You shouldn't need to use this function, unless you have
    /// very specific stuff to implement that Zancle doesn't support,
    /// or implement a temporary workaround until a bug is fixed.
    ///
    /// \return OpenGL handle of the texture or 0 if not yet created
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getNativeHandle() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get a rectangle covering the entire texture
    ///
    /// This function is useful to conveniently initialize `za::Sprite`
    /// objects that are intended to be used with this texture.
    ///
    /// \return Rectangle covering the entire texture, from {0, 0} to {width, height}
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Rect2f getRect() const;

    ////////////////////////////////////////////////////////////
    /// \brief Bind a texture for rendering
    ///
    /// This function is not part of the graphics API, it mustn't be
    /// used when drawing Zancle entities. It must be used only if you
    /// mix `za::Texture` with OpenGL code.
    ///
    /// \code
    /// za::Texture t1, t2;
    /// ...
    /// t1.bind();
    /// // draw OpenGL stuff that use t1...
    /// t2.bind();
    /// // draw OpenGL stuff that use t2...
    /// za::Texture::unbind();
    /// // draw OpenGL stuff that use no texture...
    /// \endcode
    ///
    ////////////////////////////////////////////////////////////
    void bind() const;

    ////////////////////////////////////////////////////////////
    /// \brief Unbind any currently bound texture
    ///
    /// Counterpart of `bind`. Call this after you are done with
    /// raw OpenGL drawing that used a `za::Texture` so that the
    /// next Zancle draw call starts from a clean texture binding.
    ///
    ////////////////////////////////////////////////////////////
    static void unbind();

    ////////////////////////////////////////////////////////////
    /// \brief Get the maximum texture size allowed
    ///
    /// This maximum size is defined by the graphics driver.
    /// You can expect a value of 512 pixels for low-end graphics
    /// card, and up to 8192 pixels or more for newer hardware.
    ///
    /// \return Maximum size allowed for textures, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int getMaximumSize();

private:
    friend class Text;
    friend class RenderTexture;
    friend class RenderTarget;
    friend struct StatesCache;

public:
    ////////////////////////////////////////////////////////////
    /// \private
    ///
    /// \brief Default constructor
    ///
    /// Creates an empty texture.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Texture(za::PassKey<Texture>&&, Vec2u size, unsigned int texture, bool sRgb);

private:
    ////////////////////////////////////////////////////////////
    /// \brief Invalidate the mipmap if one exists
    ///
    /// This also resets the texture's minifying function.
    /// This function is mainly for internal use by RenderTexture.
    ///
    ////////////////////////////////////////////////////////////
    void invalidateMipmap();

    ////////////////////////////////////////////////////////////
    /// \brief Destroy the underlying OpenGL texture, if any
    ///
    /// Deletes the GL texture on the shared context and unbinds
    /// it from the current texture unit if it was bound. Used by
    /// both the destructor and move assignment.
    ///
    ////////////////////////////////////////////////////////////
    void destroyGlTexture();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    Vec2u           m_size;            //!< Public texture size
    unsigned int    m_texture{};       //!< Internal texture identifier
    bool            m_isSmooth{};      //!< Status of the smooth filter
    bool            m_sRgb{};          //!< Should the texture source be converted from sRGB?
    TextureWrapMode m_wrapMode{};      //!< Active wrap mode for sampling outside `[0, 1]`
    bool            m_fboAttachment{}; //!< Is this texture owned by a framebuffer object?
    bool            m_hasMipmap{};     //!< Has the mipmap been generated?
    unsigned int    m_cacheId;         //!< Unique number that identifies the texture to the render target's cache

    za::U32 m_destructiveGeneration{0}; //!< Bumped on every non-additive mutation (autobatch invalidation)

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_DEFINE_LIFETIME_DEPENDEE(Texture, GlyphMappedText);
};

////////////////////////////////////////////////////////////
/// \brief Swap the contents of one texture with those of another
///
/// \param lhs First instance to swap
/// \param rhs Second instance to swap
///
////////////////////////////////////////////////////////////
ZA_GRAPHICS_API void swap(Texture& lhs, Texture& rhs) noexcept;

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::Texture
/// \ingroup graphics
///
/// `za::Texture` stores pixels that can be drawn, with a sprite
/// for example. A texture lives in the graphics card memory,
/// therefore it is very fast to draw a texture to a render target,
/// or copy a render target to a texture (the graphics card can
/// access both directly).
///
/// Being stored in the graphics card memory has some drawbacks.
/// A texture cannot be manipulated as freely as a `za::Image`,
/// you need to prepare the pixels first and then upload them
/// to the texture in a single operation (see `Texture::update`).
///
/// `za::Texture` makes it easy to convert from/to `za::Image`, but
/// keep in mind that these calls require transfers between
/// the graphics card and the central memory, therefore they are
/// slow operations.
///
/// A texture can be loaded from an image, but also directly
/// from a file/memory/stream. The necessary shortcuts are defined
/// so that you don't need an image first for the most common cases.
/// However, if you want to perform some modifications on the pixels
/// before creating the final texture, you can load your file to a
/// `za::Image`, do whatever you need with the pixels, and then call
/// `Texture::loadFromImage`.
///
/// Since they live in the graphics card memory, the pixels of a texture
/// cannot be accessed without a slow copy first. And they cannot be
/// accessed individually. Therefore, if you need to read the texture's
/// pixels (like for pixel-perfect collisions), it is recommended to
/// store the collision information separately, for example in an array
/// of booleans.
///
/// Like `za::Image`, `za::Texture` can handle a unique internal
/// representation of pixels, which is RGBA 32 bits. This means
/// that a pixel must be composed of 8 bit red, green, blue and
/// alpha channels -- just like a `za::Color`.
///
/// When providing texture data from an image file or memory, it can
/// either be stored in a linear color space or an sRGB color space.
/// Most digital images account for gamma correction already, so they
/// would need to be "uncorrected" back to linear color space before
/// being processed by the hardware. The hardware can automatically
/// convert it from the sRGB color space to a linear color space when
/// it gets sampled. When the rendered image gets output to the final
/// framebuffer, it gets converted back to sRGB.
///
/// This option is only useful in conjunction with an sRGB capable
/// framebuffer. This can be requested during window creation.
///
/// Usage example:
/// \code
/// // The most common use of `za::Texture`: drawing a sprite.
///
/// // Load a texture from a file. The factory returns an
/// // `Optional<Texture>`; `.value()` extracts it (and throws on
/// // failure). For graceful error handling, inspect the optional.
/// const auto texture = za::Texture::loadFromFile("texture.png").value();
///
/// // Build a sprite from the texture and draw it. In Zancle, the
/// // texture is passed to `draw` rather than stored on the sprite,
/// // so the sprite cannot accidentally outlive the texture.
/// const za::Sprite sprite{.textureRect = texture.getRect()};
/// window.draw(sprite, {.texture = &texture});
/// \endcode
///
/// \code
/// // Another common use of `za::Texture`: streaming pixel data
/// // (e.g. video frames or procedurally generated content).
///
/// // Create an empty texture.
/// auto texture = za::Texture::create({640u, 480u}).value();
///
/// while (window.isOpen()) // the main loop
/// {
///     // ...
///
///     // Update the texture with a fresh chunk of pixels.
///     const za::U8* pixels = /* next frame */;
///     texture.update(pixels);
///
///     // Draw the texture.
///     window.draw(texture);
///
///     // ...
/// }
/// \endcode
///
/// Like `za::Shader` that can be used as a raw OpenGL shader,
/// `za::Texture` can also be used directly as a raw texture for
/// custom OpenGL geometry.
/// \code
/// texture.bind();
/// ... render OpenGL geometry ...
/// za::Texture::unbind();
/// \endcode
///
/// \see `za::Sprite`, `za::Image`, `za::RenderTexture`
///
////////////////////////////////////////////////////////////
