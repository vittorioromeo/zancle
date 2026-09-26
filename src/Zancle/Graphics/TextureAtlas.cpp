// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/TextureAtlas.hpp"

#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/TextureAtlasUtils.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/RectPacker.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"


namespace za
{
////////////////////////////////////////////////////////////
TextureAtlas::TextureAtlas(Texture&& atlasTexture) :
    m_atlasTexture(ZA_MOVE(atlasTexture)),
    m_rectPacker(m_atlasTexture.getSize())
{
    // The atlas owns the entire surface: clear it so that entry padding and
    // uncovered regions hold transparent black instead of undefined contents
    // (which linear filtering would blend into the entries' edges)
    if (!m_atlasTexture.clear())
        priv::errMsg("Failed to clear texture atlas texture on construction");
}


////////////////////////////////////////////////////////////
za::Optional<Rect2f> TextureAtlas::add(const za::U8* pixels, Vec2u size, Vec2u padding)
{
    return TextureAtlasUtils::add(m_atlasTexture, m_rectPacker, padding, pixels, size);
}


////////////////////////////////////////////////////////////
za::Optional<Rect2f> TextureAtlas::add(const Image& image, Vec2u padding)
{
    return TextureAtlasUtils::add(m_atlasTexture, m_rectPacker, padding, image);
}


////////////////////////////////////////////////////////////
za::Optional<Rect2f> TextureAtlas::add(const Texture& texture, Vec2u padding)
{
    return TextureAtlasUtils::add(m_atlasTexture, m_rectPacker, padding, texture);
}


////////////////////////////////////////////////////////////
Texture& TextureAtlas::getTexture()
{
    return m_atlasTexture;
}


////////////////////////////////////////////////////////////
const Texture& TextureAtlas::getTexture() const
{
    return m_atlasTexture;
}


////////////////////////////////////////////////////////////
RectPacker& TextureAtlas::getRectPacker()
{
    return m_rectPacker;
}


////////////////////////////////////////////////////////////
const RectPacker& TextureAtlas::getRectPacker() const
{
    return m_rectPacker;
}

} // namespace za
