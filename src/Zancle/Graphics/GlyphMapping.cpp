// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/GlyphMapping.hpp"

#include "Zancle/Graphics/Glyph.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Base/Assert.hpp"


namespace
{
// clang-format off
////////////////////////////////////////////////////////////
// Printable ASCII (95) + Latin-1 Supplement U+00A0..U+00FF (96) = 191 total
constexpr char32_t printableLatin1Chars[191] = {
    // ASCII U+0020..U+007E
    U' ', U'!', U'"', U'#',  U'$', U'%', U'&', U'\'', U'(', U')', U'*', U'+', U',', U'-', U'.', U'/', U'0', U'1', U'2',
    U'3', U'4', U'5', U'6',  U'7', U'8', U'9', U':',  U';', U'<', U'=', U'>', U'?', U'@', U'A', U'B', U'C', U'D', U'E',
    U'F', U'G', U'H', U'I',  U'J', U'K', U'L', U'M',  U'N', U'O', U'P', U'Q', U'R', U'S', U'T', U'U', U'V', U'W', U'X',
    U'Y', U'Z', U'[', U'\\', U']', U'^', U'_', U'`',  U'a', U'b', U'c', U'd', U'e', U'f', U'g', U'h', U'i', U'j', U'k',
    U'l', U'm', U'n', U'o',  U'p', U'q', U'r', U's',  U't', U'u', U'v', U'w', U'x', U'y', U'z', U'{', U'|', U'}', U'~',

    // Latin-1 Supplement U+00A0..U+00FF (skipping C1 controls U+0080..U+009F)
    U'\u00A0', U'\u00A1', U'\u00A2', U'\u00A3', U'\u00A4', U'\u00A5', U'\u00A6', U'\u00A7',
    U'\u00A8', U'\u00A9', U'\u00AA', U'\u00AB', U'\u00AC', U'\u00AD', U'\u00AE', U'\u00AF',
    U'\u00B0', U'\u00B1', U'\u00B2', U'\u00B3', U'\u00B4', U'\u00B5', U'\u00B6', U'\u00B7',
    U'\u00B8', U'\u00B9', U'\u00BA', U'\u00BB', U'\u00BC', U'\u00BD', U'\u00BE', U'\u00BF',
    U'\u00C0', U'\u00C1', U'\u00C2', U'\u00C3', U'\u00C4', U'\u00C5', U'\u00C6', U'\u00C7',
    U'\u00C8', U'\u00C9', U'\u00CA', U'\u00CB', U'\u00CC', U'\u00CD', U'\u00CE', U'\u00CF',
    U'\u00D0', U'\u00D1', U'\u00D2', U'\u00D3', U'\u00D4', U'\u00D5', U'\u00D6', U'\u00D7',
    U'\u00D8', U'\u00D9', U'\u00DA', U'\u00DB', U'\u00DC', U'\u00DD', U'\u00DE', U'\u00DF',
    U'\u00E0', U'\u00E1', U'\u00E2', U'\u00E3', U'\u00E4', U'\u00E5', U'\u00E6', U'\u00E7',
    U'\u00E8', U'\u00E9', U'\u00EA', U'\u00EB', U'\u00EC', U'\u00ED', U'\u00EE', U'\u00EF',
    U'\u00F0', U'\u00F1', U'\u00F2', U'\u00F3', U'\u00F4', U'\u00F5', U'\u00F6', U'\u00F7',
    U'\u00F8', U'\u00F9', U'\u00FA', U'\u00FB', U'\u00FC', U'\u00FD', U'\u00FE', U'\u00FF'
};
// clang-format on

} // namespace

namespace za
{
////////////////////////////////////////////////////////////
GlyphLoadSettings GlyphLoadSettings::ascii(const unsigned int characterSize, const bool bold, const float outlineThickness)
{
    return {
        .codePoints       = printableLatin1Chars,
        .codePointCount   = 95,
        .characterSize    = characterSize,
        .bold             = bold,
        .outlineThickness = outlineThickness,
    };
}


////////////////////////////////////////////////////////////
GlyphLoadSettings GlyphLoadSettings::latin1(const unsigned int characterSize, const bool bold, const float outlineThickness)
{
    return {
        .codePoints       = printableLatin1Chars,
        .codePointCount   = 191,
        .characterSize    = characterSize,
        .bold             = bold,
        .outlineThickness = outlineThickness,
    };
}


////////////////////////////////////////////////////////////
const Glyph& GlyphMapping::getGlyph(const char32_t                      codePoint,
                                    [[maybe_unused]] const unsigned int theCharacterSize,
                                    [[maybe_unused]] const bool         theBold,
                                    const float                         theOutlineThickness) const
{
    ZA_ASSERT(theCharacterSize == characterSize);
    ZA_ASSERT(theBold == bold);
    ZA_ASSERT(theOutlineThickness == 0.f || theOutlineThickness == outlineThickness);

    const auto& map = (theOutlineThickness != 0.f) ? outlineGlyphs : fillGlyphs;

    if (const auto* it = map.find(codePoint); it != map.end())
        return it->second;

    // Unmapped code point: warn once per code point and render the font's
    // missing-glyph placeholder instead of hard failure
    if (warnedMissingCodePoints.insert(codePoint).second)
        priv::errMsg("Code point U+{:x} is not present in the glyph mapping, rendering missing-glyph placeholder",
                     static_cast<unsigned int>(codePoint));

    return (theOutlineThickness != 0.f) ? fallbackOutlineGlyph : fallbackFillGlyph;
}


////////////////////////////////////////////////////////////
GlyphMapping::GlyphPair GlyphMapping::getFillAndOutlineGlyph(
    const char32_t                      codePoint,
    [[maybe_unused]] const unsigned int theCharacterSize,
    [[maybe_unused]] const bool         theBold,
    [[maybe_unused]] const float        theOutlineThickness) const
{
    ZA_ASSERT(theCharacterSize == characterSize);
    ZA_ASSERT(theBold == bold);
    ZA_ASSERT(theOutlineThickness != 0.f && theOutlineThickness == outlineThickness);

    const auto* fillIt    = fillGlyphs.find(codePoint);
    const auto* outlineIt = outlineGlyphs.find(codePoint);

    if (fillIt == fillGlyphs.end() || outlineIt == outlineGlyphs.end()) [[unlikely]]
    {
        // Unmapped code point: warn once per code point and render the font's
        // missing-glyph placeholder instead of hard failure (see `getGlyph`)
        if (warnedMissingCodePoints.insert(codePoint).second)
            priv::errMsg("Code point U+{:x} is not present in the glyph mapping, rendering missing-glyph placeholder",
                         static_cast<unsigned int>(codePoint));

        return {
            .fillGlyph    = fillIt == fillGlyphs.end() ? fallbackFillGlyph : fillIt->second,
            .outlineGlyph = outlineIt == outlineGlyphs.end() ? fallbackOutlineGlyph : outlineIt->second,
        };
    }

    return {
        .fillGlyph    = fillIt->second,
        .outlineGlyph = outlineIt->second,
    };
}


////////////////////////////////////////////////////////////
float GlyphMapping::getLineSpacing([[maybe_unused]] const unsigned int theCharacterSize) const
{
    ZA_ASSERT(theCharacterSize == characterSize);
    return cachedLineSpacing;
}


////////////////////////////////////////////////////////////
float GlyphMapping::getAscent([[maybe_unused]] const unsigned int theCharacterSize) const
{
    ZA_ASSERT(theCharacterSize == characterSize);
    return cachedAscent;
}


////////////////////////////////////////////////////////////
float GlyphMapping::getDescent([[maybe_unused]] const unsigned int theCharacterSize) const
{
    ZA_ASSERT(theCharacterSize == characterSize);
    return cachedDescent;
}


////////////////////////////////////////////////////////////
float GlyphMapping::getUnderlinePosition([[maybe_unused]] const unsigned int theCharacterSize) const
{
    ZA_ASSERT(theCharacterSize == characterSize);
    return cachedUnderlinePosition;
}


////////////////////////////////////////////////////////////
float GlyphMapping::getUnderlineThickness([[maybe_unused]] const unsigned int theCharacterSize) const
{
    ZA_ASSERT(theCharacterSize == characterSize);
    return cachedUnderlineThickness;
}


} // namespace za
