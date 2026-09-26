#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/Glyph.hpp"
#include "Zancle/Graphics/GlyphMappedTextData.hpp"
#include "Zancle/Graphics/GlyphMapping.hpp"
#include "Zancle/Graphics/TextBase.hpp"

#include "Zancle/Lifetime/LifetimeDependant.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class FontFace;
class Texture;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Text rendered using a preloaded GlyphMapping
///
/// Unlike `za::Text`, this class does not lazily rasterize glyphs.
/// All glyphs must be preloaded into a `GlyphMapping` via
/// `FontFace::loadGlyphs()` before constructing this object.
///
/// Character size and outline thickness are baked into the mapping
/// and cannot be changed after construction.
///
////////////////////////////////////////////////////////////
class ZA_GRAPHICS_API GlyphMappedText : public TextBase
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Initialization data for `za::GlyphMappedText`
    ///
    /// Alias for `za::GlyphMappedTextData`. Unlike `za::TextData`,
    /// it omits `characterSize`, `outlineThickness`, and `bold`,
    /// which are baked into the associated `za::GlyphMapping`.
    ///
    ////////////////////////////////////////////////////////////
    using Data = GlyphMappedTextData;

    ////////////////////////////////////////////////////////////
    /// \brief Construct a glyph-mapped text from preloaded resources
    ///
    /// All glyphs that the text needs must already be present in
    /// `glyphMapping`. The font face is referenced for kerning
    /// queries, the texture is referenced as the glyph atlas, and
    /// the mapping is referenced for the glyph layout. None of
    /// these are copied: they must outlive the text.
    ///
    /// \param fontFace     Font face used for kerning queries (referenced)
    /// \param texture      Glyph atlas texture (referenced)
    /// \param glyphMapping Precomputed glyph mapping (referenced)
    /// \param data         Initial text properties
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] GlyphMappedText(const FontFace&     fontFace,
                                  const Texture&      texture,
                                  const GlyphMapping& glyphMapping,
                                  const Data&         data);

    ////////////////////////////////////////////////////////////
    /// \brief Disallow construction from a temporary font face
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] GlyphMappedText(FontFace&&         fontFace,
                                  const Texture&     texture,
                                  const GlyphMapping glyphMapping,
                                  const Data&        data) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Disallow construction from a temporary texture
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] GlyphMappedText(const FontFace&    fontFace,
                                  Texture&&          texture,
                                  const GlyphMapping glyphMapping,
                                  const Data&        data) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Disallow construction from a temporary glyph mapping
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] GlyphMappedText(const FontFace& fontFace,
                                  const Texture&  texture,
                                  GlyphMapping&&  glyphMapping,
                                  const Data&     data) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~GlyphMappedText();

    ////////////////////////////////////////////////////////////
    /// \brief Copy operations
    ///
    ////////////////////////////////////////////////////////////
    GlyphMappedText(const GlyphMappedText&);
    GlyphMappedText& operator=(const GlyphMappedText&);

    ////////////////////////////////////////////////////////////
    /// \brief Move operations
    ///
    ////////////////////////////////////////////////////////////
    GlyphMappedText(GlyphMappedText&&) noexcept;
    GlyphMappedText& operator=(GlyphMappedText&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Replace the underlying font face, texture, and glyph mapping
    ///
    /// All three parameters are referenced (not copied) and must
    /// outlive the text. The next draw will use the new resources.
    ///
    /// \param fontFace     New font face (for kerning queries)
    /// \param texture      New glyph atlas texture
    /// \param glyphMapping New precomputed glyph mapping
    ///
    ////////////////////////////////////////////////////////////
    void setGlyphMapping(const FontFace& fontFace, const Texture& texture, const GlyphMapping& glyphMapping);

    ////////////////////////////////////////////////////////////
    /// \brief Disallow setting from a temporary glyph mapping
    ///
    ////////////////////////////////////////////////////////////
    void setGlyphMapping(const FontFace& fontFace, const Texture& texture, GlyphMapping&& glyphMapping) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Get the glyph mapping currently used by this text
    ///
    /// \return Reference to the underlying `za::GlyphMapping`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const GlyphMapping& getGlyphMapping() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get whether the text is bold
    ///
    /// Bold is baked into the GlyphMapping and cannot be changed.
    ///
    /// \return True if the mapping was created with bold
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isBold() const;

    ////////////////////////////////////////////////////////////
    /// \brief CRTP hook returning the "font source" -- in this case `*this`
    ///
    /// `za::TextBase` calls this from templated code that needs
    /// to access the glyph metrics provider. For
    /// `GlyphMappedText` the metrics live on the object itself
    /// (forwarded to the underlying `za::FontFace` and
    /// `za::GlyphMapping`), so the implementation simply returns
    /// `*this`.
    ///
    /// \return Reference to `*this`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const GlyphMappedText& getFontSource() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get kerning between two code points
    ///
    /// Delegates to the stored FontFace.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getKerning(char32_t first, char32_t second, unsigned int characterSize, bool bold) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get a glyph from the mapping
    ///
    /// Delegates to the stored GlyphMapping.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Glyph& getGlyph(char32_t codePoint, unsigned int characterSize, bool bold, float outlineThickness) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get both fill and outline glyphs
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] GlyphMapping::GlyphPair getFillAndOutlineGlyph(
        char32_t     codePoint,
        unsigned int characterSize,
        bool         bold,
        float        outlineThickness) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get line spacing
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getLineSpacing(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get ascent
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getAscent(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get descent
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getDescent(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get underline position
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getUnderlinePosition(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get underline thickness
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getUnderlineThickness(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the glyph atlas texture currently used by this text
    ///
    /// \return Reference to the atlas texture
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Texture& getTexture() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the character size
    ///
    /// \return Size of the characters, in pixels
    ///
    /// \see `setCharacterSize`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getCharacterSize() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the outline thickness of the text
    ///
    /// \return Outline thickness of the text, in pixels
    ///
    /// \see `setOutlineThickness`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getOutlineThickness() const;

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    const FontFace*     m_fontFace{};     //!< Font face for kerning queries
    const Texture*      m_texture{};      //!< Atlas texture for rendering
    const GlyphMapping* m_glyphMapping{}; //!< Glyph mapping used to display the string

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_DEFINE_LIFETIME_DEPENDANT(FontFace);
    ZA_DEFINE_LIFETIME_DEPENDANT(Texture);
    ZA_DEFINE_LIFETIME_DEPENDANT(GlyphMapping);
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::GlyphMappedText
/// \ingroup graphics
///
/// `za::GlyphMappedText` is the high-performance text drawable
/// for static or frequently-redrawn strings. Where `za::Text`
/// rasterizes glyphs lazily as new code points or sizes are
/// requested, `GlyphMappedText` requires the caller to preload
/// all glyphs into a `za::GlyphMapping` (via
/// `za::FontFace::loadGlyphs`) up front. After that, drawing is
/// reduced to laying out vertices over a known glyph table -- no
/// glyph rasterization and no atlas updates. (Kerning queries
/// still go through `za::FontFace`: a kerning-cache miss performs
/// two FreeType glyph loads to compute the pair's kerning.)
///
/// Because the character size, the bold flag, and the outline
/// thickness are baked into the `GlyphMapping` itself, those
/// fields are intentionally absent from `za::GlyphMappedTextData`.
/// To draw text with a different size or style, build (and
/// cache) a separate `GlyphMapping`.
///
/// `za::GlyphMappedText` borrows references to:
/// \li a `za::FontFace` (for kerning queries),
/// \li a `za::Texture` (the glyph atlas),
/// \li a `za::GlyphMapping` (the precomputed glyph layout).
///
/// All three must outlive the text object. In debug builds with
/// `ZA_ENABLE_LIFETIME_TRACKING`, lifetime violations are
/// detected automatically.
///
/// \see `za::Text`, `za::Font`, `za::FontFace`,
///      `za::GlyphMapping`, `za::TextureAtlas`
///
////////////////////////////////////////////////////////////
