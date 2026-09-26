#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/Texture.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/RectPacker.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/IntTypes.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class Image;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Owning texture atlas: a texture plus a rectangle packer
///
/// `za::TextureAtlas` couples a single `za::Texture` with a
/// `za::RectPacker` to let multiple sub-images, glyphs, or
/// textures share one GPU texture. Sharing a texture across
/// many drawables is a prerequisite for effective batching:
/// the renderer can issue a single draw call instead of one
/// per source texture.
///
/// `za::TextureAtlas` owns its texture by value. The atlas
/// texture is created by the caller and moved into the atlas,
/// which clears it to transparent black on construction so that
/// entry padding and uncovered regions hold defined contents;
/// from then on, the atlas takes care of inserting new
/// sub-images via `add` and tracking their positions through
/// the packer.
///
////////////////////////////////////////////////////////////
class [[nodiscard]] ZA_GRAPHICS_API TextureAtlas
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Construct an atlas around an existing texture
    ///
    /// The atlas takes ownership of `atlasTexture`. The texture
    /// must already be created with the desired size and
    /// settings; the atlas only manages packing into it.
    ///
    /// \param atlasTexture Source texture to wrap (moved into the atlas)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit TextureAtlas(Texture&& atlasTexture);

    ////////////////////////////////////////////////////////////
    /// \brief Add a raw RGBA pixel block to the atlas
    ///
    /// Asks the rect packer for a free region of size
    /// `size + 2 * padding`, uploads the pixels into it, and
    /// returns the texture rectangle (excluding the padding) of
    /// the newly added image.
    ///
    /// \param pixels  Pointer to `size.x * size.y` RGBA pixels
    /// \param size    Width and height of the source pixel block
    /// \param padding Padding to leave around the image (each side)
    ///
    /// \return Texture rectangle of the inserted image, or `za::nullOpt` if the atlas is full
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<Rect2f> add(const za::U8* pixels, Vec2u size, Vec2u padding = {});

    ////////////////////////////////////////////////////////////
    /// \brief Add an `za::Image` to the atlas
    ///
    /// \param image   Source image (RGBA)
    /// \param padding Padding to leave around the image (each side)
    ///
    /// \return Texture rectangle of the inserted image, or `za::nullOpt` if the atlas is full
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<Rect2f> add(const Image& image, Vec2u padding = {});

    ////////////////////////////////////////////////////////////
    /// \brief Add the contents of another `za::Texture` to the atlas
    ///
    /// Copies `texture` into a free region of the atlas via a
    /// GPU-to-GPU framebuffer blit; the pixels never leave the
    /// GPU.
    ///
    /// \param texture Source texture (copied on the GPU)
    /// \param padding Padding to leave around the image (each side)
    ///
    /// \return Texture rectangle of the inserted image, or `za::nullOpt` if the atlas is full
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<Rect2f> add(const Texture& texture, Vec2u padding = {});

    ////////////////////////////////////////////////////////////
    /// \brief Access the underlying atlas texture (mutable)
    ///
    /// \return Reference to the texture managed by this atlas
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Texture& getTexture();

    ////////////////////////////////////////////////////////////
    /// \brief Access the underlying atlas texture (const)
    ///
    /// \return Const reference to the texture managed by this atlas
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Texture& getTexture() const;

    ////////////////////////////////////////////////////////////
    /// \brief Access the rectangle packer used by this atlas (mutable)
    ///
    /// Exposed so that advanced users can interact with the
    /// packer directly (e.g. to query free space or to insert
    /// rectangles without uploading pixels).
    ///
    /// \return Reference to the underlying `za::RectPacker`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] RectPacker& getRectPacker();

    ////////////////////////////////////////////////////////////
    /// \brief Access the rectangle packer used by this atlas (const)
    ///
    /// \return Const reference to the underlying `za::RectPacker`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const RectPacker& getRectPacker() const;

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    Texture    m_atlasTexture; //!< Underlying atlas texture (owned by value)
    RectPacker m_rectPacker;   //!< Packer used to allocate free regions of the atlas
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::TextureAtlas
/// \ingroup graphics
///
/// `za::TextureAtlas` is the simplest way to share a single
/// `za::Texture` between many drawables (sprites, shapes, text
/// glyphs, ...). It owns the atlas texture and uses a
/// `za::RectPacker` to find free regions for new sub-images.
///
/// Pack as much as possible into a single atlas: every
/// drawable that samples from the same texture can be
/// auto-batched by `za::RenderTarget`, drastically cutting the
/// number of GPU draw calls per frame.
///
/// Atlases also accept fonts via `za::Font::openFromFile(...,
/// &atlas)` and `za::FontFace::loadGlyphs(atlas, ...)`,
/// allowing glyph rasterization to share storage with the rest
/// of your art.
///
/// \see `za::Texture`, `za::Image`, `za::RectPacker`,
///      `za::Font`, `za::FontFace`, `za::TextureAtlasUtils`
///
////////////////////////////////////////////////////////////


// TODO P0: deprecate in favour of textureatlasutils?
