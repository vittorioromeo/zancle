#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/Glyph.hpp"

#include "Zancle/Lifetime/LifetimeDependee.hpp"

#include "Zancle/Container/AnkerlUnorderedDense.hpp"

#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class GlyphMappedText;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Settings for `za::FontFace::loadGlyphs`
///
/// Describes which glyphs to rasterize, at which character size,
/// in which style, and with which outline thickness. The
/// `codePoints` / `codePointCount` pair specifies the explicit
/// list of Unicode code points to load.
///
/// The code point list must always include U+0020 (space): text
/// layout unconditionally queries the space glyph to compute
/// whitespace width. If strike-through text will be drawn,
/// U+0078 ('x') is also required, as its bounds anchor the
/// strike-through line. Querying a glyph missing from the
/// mapping at draw time warns (once per code point) and renders
/// the font's missing-glyph placeholder (see
/// `GlyphMapping::getGlyph`).
///
/// The `ascii` and `latin1` static factories build common
/// presets for the printable ASCII and Latin-1 Supplement
/// ranges respectively (both include U+0020 and U+0078).
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] GlyphLoadSettings
{
    const char32_t* codePoints{};     //!< Pointer to the array of code points to rasterize
    za::SizeT       codePointCount{}; //!< Number of code points in `codePoints` // TODO P1: span?

    unsigned int characterSize{30u};    //!< Character size, in pixels (baked into the resulting `GlyphMapping`)
    bool         bold{false};           //!< Whether to load the bold variant
    float        outlineThickness{0.f}; //!< Outline thickness; if non-zero, both fill and outline glyphs are loaded

    ////////////////////////////////////////////////////////////
    /// \brief Build settings that load the printable ASCII range (U+0020 to U+007E)
    ///
    /// \param characterSize    Character size to bake in
    /// \param bold             Whether to load the bold variant
    /// \param outlineThickness Outline thickness (`0` to skip outlines)
    ///
    /// \return Configured `GlyphLoadSettings`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static GlyphLoadSettings ascii(unsigned int characterSize, bool bold, float outlineThickness);

    ////////////////////////////////////////////////////////////
    /// \brief Build settings that load printable ASCII + Latin-1 Supplement (U+0020 to U+00FF)
    ///
    /// \param characterSize    Character size to bake in
    /// \param bold             Whether to load the bold variant
    /// \param outlineThickness Outline thickness (`0` to skip outlines)
    ///
    /// \return Configured `GlyphLoadSettings`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static GlyphLoadSettings latin1(unsigned int characterSize, bool bold, float outlineThickness);
};


////////////////////////////////////////////////////////////
/// \brief Immutable mapping of code points to preloaded glyphs
///
/// Created by `FontFace::loadGlyphs()`. Contains preloaded glyph
/// metrics and cached font metrics. Does not store references to
/// `FontFace` or `Texture` -- those are provided externally.
///
////////////////////////////////////////////////////////////
struct ZA_GRAPHICS_API GlyphMapping
{
    ////////////////////////////////////////////////////////////
    /// \brief A fill glyph paired with its matching outline glyph
    ///
    /// Returned by `getFillAndOutlineGlyph`. Used by
    /// `za::GlyphMappedText` to render outlined text in a single
    /// pass.
    ///
    ////////////////////////////////////////////////////////////
    struct GlyphPair
    {
        const Glyph& fillGlyph;    //!< Glyph used for the filled (interior) part of the character
        const Glyph& outlineGlyph; //!< Glyph used for the outlined (border) part of the character
    };

    ////////////////////////////////////////////////////////////
    /// \brief Get a glyph from the mapping
    ///
    /// Signature matches Font::getGlyph for TextUtils compatibility.
    /// The characterSize/bold/outlineThickness params are baked in
    /// and only verified via debug asserts.
    ///
    /// Querying a code point that was not loaded into the mapping
    /// warns (once per code point) and returns the font's
    /// missing-glyph placeholder (`.notdef`) instead.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Glyph& getGlyph(char32_t     codePoint,
                                        unsigned int theCharacterSize,
                                        bool         theBold,
                                        float        theOutlineThickness) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get both fill and outline glyphs
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] GlyphPair getFillAndOutlineGlyph(char32_t     codePoint,
                                                   unsigned int theCharacterSize,
                                                   bool         theBold,
                                                   float        theOutlineThickness) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get line spacing
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getLineSpacing(unsigned int theCharacterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get ascent
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getAscent(unsigned int theCharacterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get descent
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getDescent(unsigned int theCharacterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get underline position
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getUnderlinePosition(unsigned int theCharacterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get underline thickness
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getUnderlineThickness(unsigned int theCharacterSize) const;

    ////////////////////////////////////////////////////////////
    // Glyph data
    ////////////////////////////////////////////////////////////
    ankerl::unordered_dense::map<char32_t, Glyph> fillGlyphs;    //!< Map of code point → fill glyph (interior)
    ankerl::unordered_dense::map<char32_t, Glyph> outlineGlyphs; //!< Map of code point → outline glyph (border)

    Glyph fallbackFillGlyph;    //!< Placeholder for unmapped code points (the font's `.notdef`)
    Glyph fallbackOutlineGlyph; //!< Placeholder outline for unmapped code points (zero if no outline was loaded)

    mutable ankerl::unordered_dense::set<char32_t> warnedMissingCodePoints; //!< Deduplicates missing-glyph warnings

    ////////////////////////////////////////////////////////////
    // Baked-in parameters
    ////////////////////////////////////////////////////////////
    unsigned int characterSize{};    //!< Character size, in pixels (frozen at load time)
    bool         bold{};             //!< Whether glyphs were rasterized in bold
    float        outlineThickness{}; //!< Outline thickness used at load time

    ////////////////////////////////////////////////////////////
    // Cached font metrics
    ////////////////////////////////////////////////////////////
    float cachedLineSpacing{};        //!< Cached line spacing for `characterSize`
    float cachedAscent{};             //!< Cached ascent for `characterSize`
    float cachedDescent{};            //!< Cached descent for `characterSize`
    float cachedUnderlinePosition{};  //!< Cached underline position for `characterSize`
    float cachedUnderlineThickness{}; //!< Cached underline thickness for `characterSize`

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_DEFINE_LIFETIME_DEPENDEE(GlyphMapping, GlyphMappedText);
};

} // namespace za


////////////////////////////////////////////////////////////
/// \struct za::GlyphMapping
/// \ingroup graphics
///
/// `za::GlyphMapping` is the precomputed glyph table that backs
/// `za::GlyphMappedText`. It is produced by
/// `za::FontFace::loadGlyphs` and contains, for a fixed set of
/// code points / character size / style / outline thickness:
///
/// \li the rasterized fill glyphs (`fillGlyphs`),
/// \li the rasterized outline glyphs (`outlineGlyphs`, if
///     `outlineThickness > 0`),
/// \li the relevant font metrics (line spacing, ascent, descent,
///     underline position and thickness) cached for the
///     character size that was used at load time.
///
/// Once created, the mapping is immutable: trying to query a
/// different character size, bold flag, or outline thickness is
/// a precondition violation. To draw text with a different
/// configuration, build (and cache) a separate `GlyphMapping`.
///
/// `GlyphMapping` does **not** own the texture into which its
/// glyphs were rasterized. The atlas is provided externally by
/// the caller (typically the same one passed to
/// `FontFace::loadGlyphs`) and must outlive any text that uses
/// the mapping.
///
/// \see `za::FontFace`, `za::GlyphMappedText`, `za::TextureAtlas`,
///      `za::GlyphLoadSettings`
///
////////////////////////////////////////////////////////////
