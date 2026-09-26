// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/TextureAtlasUtils.hpp"

#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/Texture.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/RectPacker.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/IntTypes.hpp"


namespace
{
////////////////////////////////////////////////////////////
[[nodiscard]] za::NullOpt fail(const char* what)
{
    za::priv::errMsg("Failed to {}", what);
    return za::nullOpt;
}

} // namespace


namespace za
{
////////////////////////////////////////////////////////////
za::Optional<Rect2f> TextureAtlasUtils::add(Texture& targetTexture, RectPacker& rectPacker, Vec2u padding, const za::U8* pixels, Vec2u size)
{
    // Reserve the full padded region so neighbouring entries are kept at
    // arm's length on every side, then upload the content offset by `padding`
    // so the same number of padding pixels sit between this entry and its
    // top/left and bottom/right neighbours. The padding ring itself is never
    // written: it relies on the atlas texture having been cleared up front
    // (`TextureAtlas` does so on construction; a fresh texture's contents are
    // otherwise undefined, and linear filtering would blend garbage into the
    // entry's edges).
    const auto packedPosition = rectPacker.pack(size + padding * 2u);

    if (!packedPosition.hasValue())
        return fail("pack pixel array rectangle for texture atlas");

    const Vec2u uploadPos = *packedPosition + padding;
    targetTexture.update(pixels, size, uploadPos);

    return za::makeOptional<Rect2f>(uploadPos.to<Vec2f>(), size.to<Vec2f>());
}


////////////////////////////////////////////////////////////
za::Optional<Rect2f> TextureAtlasUtils::add(Texture& targetTexture, RectPacker& rectPacker, Vec2u padding, const Image& image)
{
    return add(targetTexture, rectPacker, padding, image.getPixelsPtr(), image.getSize());
}


////////////////////////////////////////////////////////////
za::Optional<Rect2f> TextureAtlasUtils::add(Texture& targetTexture, RectPacker& rectPacker, Vec2u padding, const Texture& texture)
{
    // See pixel-array overload for the +padding/-padding rationale.
    const auto packedPosition = rectPacker.pack(texture.getSize() + padding * 2u);

    if (!packedPosition.hasValue())
        return fail("pack texture rectangle for texture atlas");

    const Vec2u uploadPos = *packedPosition + padding;

    if (!targetTexture.update(texture, uploadPos))
        return fail("update texture for texture atlas");

    return za::makeOptional<Rect2f>(uploadPos.to<Vec2f>(), texture.getSize().to<Vec2f>());
}

} // namespace za
