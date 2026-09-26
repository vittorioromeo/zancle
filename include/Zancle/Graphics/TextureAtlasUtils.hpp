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
/// \brief Stateless helpers that pack pixel data into a texture + rect packer pair
///
/// `TextureAtlasUtils` is the building block of
/// `za::TextureAtlas`. It exposes the same `add` operations
/// that the atlas does, but as free static functions taking
/// the target texture and the rect packer by reference. This
/// is convenient when you want to manage the storage and the
/// packer separately, or when you do not want the ownership
/// semantics of `za::TextureAtlas`.
///
////////////////////////////////////////////////////////////
struct ZA_GRAPHICS_API TextureAtlasUtils
{
    ////////////////////////////////////////////////////////////
    /// \brief Pack a raw RGBA pixel block into the target texture
    ///
    /// Asks `rectPacker` for a free region of size
    /// `size + 2 * padding`, uploads `pixels` into it, and
    /// returns the texture rectangle (excluding the padding) of
    /// the newly added image.
    ///
    /// \param targetTexture Texture into which the pixels are uploaded
    /// \param rectPacker    Rect packer used to allocate free regions of `targetTexture`
    /// \param padding       Padding to leave around the image (each side)
    /// \param pixels        Pointer to `size.x * size.y` RGBA pixels
    /// \param size          Width and height of the source pixel block
    ///
    /// \return Texture rectangle of the inserted image, or `za::nullOpt` if no free region was found
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Rect2f> add(
        Texture&      targetTexture,
        RectPacker&   rectPacker,
        Vec2u         padding,
        const za::U8* pixels,
        Vec2u         size);

    ////////////////////////////////////////////////////////////
    /// \brief Pack the pixels of an `za::Image` into the target texture
    ///
    /// \param targetTexture Texture into which the image is uploaded
    /// \param rectPacker    Rect packer used to allocate free regions of `targetTexture`
    /// \param padding       Padding to leave around the image (each side)
    /// \param image         Source image (RGBA)
    ///
    /// \return Texture rectangle of the inserted image, or `za::nullOpt` if no free region was found
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Rect2f> add(Texture& targetTexture, RectPacker& rectPacker, Vec2u padding, const Image& image);

    ////////////////////////////////////////////////////////////
    /// \brief Pack the pixels of another `za::Texture` into the target texture
    ///
    /// Copies `texture` into a free region of `targetTexture`
    /// via a GPU-to-GPU framebuffer blit; the pixels never
    /// leave the GPU.
    ///
    /// \param targetTexture Texture into which the pixels are copied
    /// \param rectPacker    Rect packer used to allocate free regions of `targetTexture`
    /// \param padding       Padding to leave around the image (each side)
    /// \param texture       Source texture (copied on the GPU)
    ///
    /// \return Texture rectangle of the inserted image, or `za::nullOpt` if no free region was found
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Rect2f> add(Texture&       targetTexture,
                                                  RectPacker&    rectPacker,
                                                  Vec2u          padding,
                                                  const Texture& texture);
};

} // namespace za


////////////////////////////////////////////////////////////
/// \struct za::TextureAtlasUtils
/// \ingroup graphics
///
/// `za::TextureAtlasUtils` is the stateless companion of
/// `za::TextureAtlas`. It exposes the same packing operations
/// as free static functions, leaving ownership of the texture
/// and the rect packer to the caller.
///
/// Use it directly when you want to drive the packing process
/// without the convenience class -- for example to share a
/// single rect packer between several textures, or to
/// integrate the atlas into your own resource manager.
///
/// \see `za::TextureAtlas`, `za::Texture`, `za::RectPacker`,
///      `za::Image`
///
////////////////////////////////////////////////////////////
