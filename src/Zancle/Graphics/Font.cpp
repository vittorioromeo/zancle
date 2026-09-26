// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Font.hpp"

#include "Zancle/Graphics/FontFace.hpp"
#include "Zancle/Graphics/FontInfo.hpp"
#include "Zancle/Graphics/Glyph.hpp"
#include "Zancle/Graphics/Priv/QuantizeOutlineThickness.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/TextureAtlas.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/IO/InputStream.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/Container/AnkerlUnorderedDense.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"
#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace
{
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline za::U32 bitCastU32(const za::I32 value)
{
    za::U32 result; // NOLINT(cppcoreguidelines-init-variables)
    ZA_MEMCPY(&result, &value, sizeof(value));
    return result;
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline za::U64 combineGlyphTableKey(
    const float    outlineThickness,
    const bool     bold,
    const char32_t index)
{
    return (za::U64{bitCastU32(za::priv::quantizeOutlineThickness(outlineThickness))} << 32) | (za::U64{bold} << 31) | index;
}

} // namespace


namespace za
{
////////////////////////////////////////////////////////////
struct Font::Impl
{
    ////////////////////////////////////////////////////////////
    template <typename K, typename V>
    using MapType = ankerl::unordered_dense::map<K, V>;


    ////////////////////////////////////////////////////////////
    using GlyphTable = MapType</* character size */ unsigned int, MapType</* combined key */ za::U64, Glyph>>;


    ////////////////////////////////////////////////////////////
    explicit Impl(FontFace&& theFontFace, TextureAtlas* theTextureAtlasPtr) :
        fontFace(ZA_MOVE(theFontFace)),
        textureAtlasPtr{theTextureAtlasPtr},
        fallbackTextureAtlas{
            theTextureAtlasPtr == nullptr
                ? za::makeOptional<TextureAtlas>(Texture::create({1024u, 1024u}, {.smooth = true}).value())
                : za::nullOpt}
    {
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] TextureAtlas& getTextureAtlas() const
    {
        return textureAtlasPtr == nullptr ? *fallbackTextureAtlas : *textureAtlasPtr;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] auto loadGlyphImpl(auto&              glyphsByCharacterSize,
                                     const za::U64      key,
                                     const char32_t     codePoint,
                                     const unsigned int characterSize,
                                     const bool         bold,
                                     const float        outlineThickness) const
    {
        const auto optGlyph = fontFace.rasterizeAndPackGlyph(getTextureAtlas(), codePoint, characterSize, bold, outlineThickness);

        if (!optGlyph.hasValue())
        {
            priv::errMsg("Failed to load glyph for code point {}, character size {}, bold {}, outline thickness {}",
                         static_cast<unsigned int>(codePoint),
                         characterSize,
                         bold,
                         outlineThickness);

            // Atlas full (or rasterization failure): cache an empty glyph
            // (renders nothing, zero advance) under the requested key so the
            // failure is recoverable and subsequent frames don't re-rasterize
            // and re-log. An alternative would be growing the atlas, but the
            // fallback atlas is intentionally fixed-size.
            return glyphsByCharacterSize.try_emplace(key, Glyph{});
        }

        return glyphsByCharacterSize.try_emplace(key, *optGlyph);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] const Glyph& getGlyphImpl(
        auto&              glyphsByCharacterSize,
        const za::U64      key,
        const char32_t     codePoint,
        const unsigned int characterSize,
        const bool         bold,
        const float        outlineThickness) const
    {
        if (const auto* it = glyphsByCharacterSize.find(key); it != glyphsByCharacterSize.end()) [[likely]]
            return it->second;

        return loadGlyphImpl(glyphsByCharacterSize, key, codePoint, characterSize, bold, outlineThickness).first->second;
    }


    ////////////////////////////////////////////////////////////
    mutable FontFace                   fontFace;
    TextureAtlas*                      textureAtlasPtr;
    mutable za::Optional<TextureAtlas> fallbackTextureAtlas;
    mutable GlyphTable                 glyphs;
};


////////////////////////////////////////////////////////////
Font::Font(za::PassKey<Font>&&, FontFace&& fontFace, TextureAtlas* textureAtlas) :
    m_impl{za::makeUnique<Impl>(ZA_MOVE(fontFace), textureAtlas)}
{
}


////////////////////////////////////////////////////////////
Font::~Font()                          = default;
Font::Font(Font&&) noexcept            = default;
Font& Font::operator=(Font&&) noexcept = default;


////////////////////////////////////////////////////////////
za::Optional<Font> Font::openFromFile(const Path& filename, TextureAtlas* textureAtlas)
{
    auto optFontFace = FontFace::openFromFile(filename);
    if (!optFontFace.hasValue())
        return za::nullOpt;

    return za::makeOptional<Font>(za::PassKey<Font>{}, ZA_MOVE(*optFontFace), textureAtlas);
}


////////////////////////////////////////////////////////////
za::Optional<Font> Font::openFromMemory(const void* data, za::SizeT sizeInBytes, TextureAtlas* textureAtlas)
{
    auto optFontFace = FontFace::openFromMemory(data, sizeInBytes);
    if (!optFontFace.hasValue())
        return za::nullOpt;

    return za::makeOptional<Font>(za::PassKey<Font>{}, ZA_MOVE(*optFontFace), textureAtlas);
}


////////////////////////////////////////////////////////////
za::Optional<Font> Font::openFromStream(InputStream& stream, TextureAtlas* textureAtlas)
{
    auto optFontFace = FontFace::openFromStream(stream); // Seeks to `0` and logs errors on failure
    if (!optFontFace.hasValue())
        return za::nullOpt;

    return za::makeOptional<Font>(za::PassKey<Font>{}, ZA_MOVE(*optFontFace), textureAtlas);
}


////////////////////////////////////////////////////////////
const FontInfo& Font::getInfo() const
{
    return m_impl->fontFace.getInfo();
}


////////////////////////////////////////////////////////////
Glyph Font::getGlyph(const char32_t codePoint, const unsigned int characterSize, const bool bold, const float outlineThickness) const
{
    return m_impl->getGlyphImpl(m_impl->glyphs[characterSize],
                                combineGlyphTableKey(outlineThickness, bold, codePoint),
                                codePoint,
                                characterSize,
                                bold,
                                outlineThickness);
}


////////////////////////////////////////////////////////////
Font::GlyphPair Font::getFillAndOutlineGlyph(const char32_t     codePoint,
                                             const unsigned int characterSize,
                                             const bool         bold,
                                             const float        outlineThickness) const
{
    ZA_ASSERT(outlineThickness != 0.f);

    auto& glyphsByCharacterSize = m_impl->glyphs[characterSize];

    const auto fillGlyphKey    = combineGlyphTableKey(0.f, bold, codePoint);
    const auto outlineGlyphKey = combineGlyphTableKey(outlineThickness, bold, codePoint);

    const auto* fillGlyphIt = glyphsByCharacterSize.find(fillGlyphKey);
    if (fillGlyphIt == glyphsByCharacterSize.end()) [[unlikely]]
    {
        fillGlyphIt = m_impl->loadGlyphImpl(glyphsByCharacterSize, fillGlyphKey, codePoint, characterSize, bold, 0.f).first;
    }

    const auto* outlineGlyphIt = glyphsByCharacterSize.find(outlineGlyphKey);
    if (outlineGlyphIt == glyphsByCharacterSize.end()) [[unlikely]]
    {
        outlineGlyphIt = m_impl
                             ->loadGlyphImpl(glyphsByCharacterSize, outlineGlyphKey, codePoint, characterSize, bold, outlineThickness)
                             .first;

        fillGlyphIt = glyphsByCharacterSize.find(fillGlyphKey);
        ZA_ASSERT(fillGlyphIt != glyphsByCharacterSize.end());
    }

    return {.fillGlyph = fillGlyphIt->second, .outlineGlyph = outlineGlyphIt->second};
}


////////////////////////////////////////////////////////////
bool Font::hasGlyph(const char32_t codePoint) const
{
    return m_impl->fontFace.hasGlyph(codePoint);
}


////////////////////////////////////////////////////////////
float Font::getKerning(const char32_t first, const char32_t second, const unsigned int characterSize, const bool bold) const
{
    return m_impl->fontFace.getKerning(first, second, characterSize, bold);
}


////////////////////////////////////////////////////////////
float Font::getAscent(unsigned int characterSize) const
{
    return m_impl->fontFace.getAscent(characterSize);
}


////////////////////////////////////////////////////////////
float Font::getDescent(unsigned int characterSize) const
{
    return m_impl->fontFace.getDescent(characterSize);
}


////////////////////////////////////////////////////////////
float Font::getLineSpacing(const unsigned int characterSize) const
{
    return m_impl->fontFace.getLineSpacing(characterSize);
}


////////////////////////////////////////////////////////////
float Font::getUnderlinePosition(const unsigned int characterSize) const
{
    return m_impl->fontFace.getUnderlinePosition(characterSize);
}


////////////////////////////////////////////////////////////
float Font::getUnderlineThickness(const unsigned int characterSize) const
{
    return m_impl->fontFace.getUnderlineThickness(characterSize);
}


////////////////////////////////////////////////////////////
Texture& Font::getTexture()
{
    return m_impl->getTextureAtlas().getTexture();
}


////////////////////////////////////////////////////////////
const Texture& Font::getTexture() const
{
    return m_impl->getTextureAtlas().getTexture();
}

} // namespace za
