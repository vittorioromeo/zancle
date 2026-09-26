#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Lifetime/LifetimeDependee.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"
#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class FontFace;
class InputStream;
class Path;
class Text;
class Texture;
class TextureAtlas;

struct FontInfo;
struct Glyph;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Class for loading and manipulating character fonts
///
////////////////////////////////////////////////////////////
class ZA_GRAPHICS_API Font
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~Font();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    Font(const Font& rhs) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    Font& operator=(const Font& rhs) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    Font(Font&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    Font& operator=(Font&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Open a font from a file on disk
    ///
    /// The supported font formats are: TrueType, Type 1, CFF,
    /// OpenType, SFNT, X11 PCF, Windows FNT, BDF, PFR and Type 42.
    /// Note that this function knows nothing about the fonts
    /// installed on the user's system: you must always provide an
    /// explicit file path.
    ///
    /// If `textureAtlas` is `nullptr`, the font allocates and owns
    /// its own glyph texture. Passing a shared atlas lets multiple
    /// fonts (and other resources) pack their glyphs into the same
    /// GPU texture, which is more efficient when batching.
    ///
    /// \warning Zancle does not preload all the font data in this
    /// function. The file must remain accessible for as long as the
    /// `za::Font` object lives, since glyphs are loaded on demand.
    ///
    /// \param filename     Path of the font file to load
    /// \param textureAtlas Optional shared glyph atlas
    ///
    /// \return Font on success, `za::nullOpt` on failure
    ///
    /// \see `openFromMemory`, `openFromStream`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Font> openFromFile(const Path& filename, TextureAtlas* textureAtlas = nullptr);

    ////////////////////////////////////////////////////////////
    /// \brief Open a font from a file held in memory
    ///
    /// Same supported formats as `openFromFile`. The buffer
    /// pointed to by `data` must remain valid for as long as the
    /// `za::Font` object lives, since glyphs are loaded on demand.
    ///
    /// \param data         Pointer to the encoded font bytes in memory
    /// \param sizeInBytes  Size of the data, in bytes
    /// \param textureAtlas Optional shared glyph atlas
    ///
    /// \return Font on success, `za::nullOpt` on failure
    ///
    /// \see `openFromFile`, `openFromStream`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Font> openFromMemory(const void*   data,
                                                           za::SizeT     sizeInBytes,
                                                           TextureAtlas* textureAtlas = nullptr);

    ////////////////////////////////////////////////////////////
    /// \brief Open a font from a custom input stream
    ///
    /// Same supported formats as `openFromFile`. The stream must
    /// remain accessible for as long as the `za::Font` object
    /// lives, since glyphs are loaded on demand.
    ///
    /// \param stream       Source stream to read encoded font data from
    /// \param textureAtlas Optional shared glyph atlas
    ///
    /// \return Font on success, `za::nullOpt` on failure
    ///
    /// \see `openFromFile`, `openFromMemory`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Font> openFromStream(InputStream& stream, TextureAtlas* textureAtlas = nullptr);

    ////////////////////////////////////////////////////////////
    /// \brief Get the font information
    ///
    /// \return A structure that holds the font information
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const FontInfo& getInfo() const;

    ////////////////////////////////////////////////////////////
    /// \brief Retrieve a glyph of the font
    ///
    /// If the font is a bitmap font, not all character sizes
    /// might be available. If the glyph is not available at the
    /// requested size, an empty glyph is returned.
    ///
    /// You may want to use `hasGlyph` to determine if the
    /// glyph exists before requesting it. If the glyph does not
    /// exist, a font specific default is returned.
    ///
    /// Be aware that using a negative value for the outline
    /// thickness will cause distorted rendering.
    ///
    /// \param codePoint        Unicode code point of the character to get
    /// \param characterSize    Reference character size
    /// \param bold             Retrieve the bold version or the regular one?
    /// \param outlineThickness Thickness of outline (when != 0 the glyph will not be filled)
    ///
    /// \return Copy of the glyph corresponding to `codePoint` and `characterSize`
    ///
    /// \note The glyph is returned by value: the font's internal glyph cache
    /// can grow and relocate its entries on any later glyph load, so
    /// references into it would not be stable.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Glyph getGlyph(char32_t codePoint, unsigned int characterSize, bool bold, float outlineThickness) const;

    ////////////////////////////////////////////////////////////
    /// \brief A fill glyph paired with its matching outline glyph
    ///
    /// Used by `za::Text` to render outlined text in a single pass.
    ///
    ////////////////////////////////////////////////////////////
    struct GlyphPair
    {
        const Glyph& fillGlyph;    //!< Glyph used for the filled (interior) part of the character
        const Glyph& outlineGlyph; //!< Glyph used for the outlined (border) part of the character
    };

    ////////////////////////////////////////////////////////////
    /// \brief Retrieve a fill glyph and its matching outline glyph in one call
    ///
    /// Equivalent to two `getGlyph` calls (with `outlineThickness == 0`
    /// and `outlineThickness != 0` respectively), but a single
    /// dictionary lookup, which is faster when both halves are needed.
    ///
    /// \param codePoint        Unicode code point of the character to get
    /// \param characterSize    Reference character size
    /// \param bold             Retrieve the bold versions or the regular ones
    /// \param outlineThickness Thickness of the outline (must be non-zero for the outline glyph to be meaningful)
    ///
    /// \return Pair of references to the fill and outline glyphs
    ///
    /// \see `getGlyph`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] GlyphPair getFillAndOutlineGlyph(char32_t     codePoint,
                                                   unsigned int characterSize,
                                                   bool         bold,
                                                   float        outlineThickness) const;

    ////////////////////////////////////////////////////////////
    /// \brief Determine if this font has a glyph representing the requested code point
    ///
    /// Most fonts only include a very limited selection of glyphs from
    /// specific Unicode subsets, like Latin, Cyrillic, or Asian characters.
    ///
    /// While code points without representation will return a font specific
    /// default character, it might be useful to verify whether specific
    /// code points are included to determine whether a font is suited
    /// to display text in a specific language.
    ///
    /// \param codePoint Unicode code point to check
    ///
    /// \return `true` if the codepoint has a glyph representation, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool hasGlyph(char32_t codePoint) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the kerning offset of two glyphs
    ///
    /// The kerning is an extra offset (negative) to apply between two
    /// glyphs when rendering them, to make the pair look more "natural".
    /// For example, the pair "AV" have a special kerning to make them
    /// closer than other characters. Most of the glyphs pairs have a
    /// kerning offset of zero, though.
    ///
    /// \param first         Unicode code point of the first character
    /// \param second        Unicode code point of the second character
    /// \param characterSize Reference character size
    /// \param bold          Retrieve the bold version or the regular one?
    ///
    /// \return Kerning value for `first` and `second`, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getKerning(char32_t first, char32_t second, unsigned int characterSize, bool bold) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the ascent
    ///
    /// The ascent is the largest distance between the baseline and
    /// the top of all glyphs in the font.
    ///
    /// Be aware that there is no uniform definition of how the
    /// ascent is calculated. It can vary from font to font.
    ///
    /// \param characterSize Reference character size
    ///
    /// \return Ascent, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getAscent(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the descent
    ///
    /// The descent is the largest distance between the baseline and
    /// the bottom of all glyphs in the font.
    ///
    /// Be aware that there is no uniform definition of how the
    /// descent is calculated. It can vary from font to font.
    ///
    /// The descent shares the same coordinate system as the
    /// ascent. This means that it will be negative for distances
    /// below the baseline.
    ///
    /// \param characterSize Reference character size
    ///
    /// \return Descent, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getDescent(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the line spacing
    ///
    /// Line spacing is the vertical offset to apply between two
    /// consecutive lines of text.
    ///
    /// \param characterSize Reference character size
    ///
    /// \return Line spacing, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getLineSpacing(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the position of the underline
    ///
    /// Underline position is the vertical offset to apply between the
    /// baseline and the underline.
    ///
    /// \param characterSize Reference character size
    ///
    /// \return Underline position, in pixels
    ///
    /// \see `getUnderlineThickness`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getUnderlinePosition(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the thickness of the underline
    ///
    /// Underline thickness is the vertical size of the underline.
    ///
    /// \param characterSize Reference character size
    ///
    /// \return Underline thickness, in pixels
    ///
    /// \see `getUnderlinePosition`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getUnderlineThickness(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Retrieve the texture that backs the rasterized glyphs
    ///
    /// If this font owns its glyph atlas, the returned texture is
    /// that atlas. If a shared `za::TextureAtlas` was passed to the
    /// open function, the returned texture is the shared atlas
    /// texture. New glyph entries accumulate in the texture as they
    /// are rasterized on demand, but the texture itself has a fixed
    /// size (the font-owned fallback atlas is 1024x1024 and does
    /// not grow).
    ///
    /// This accessor exists mostly for advanced uses (custom
    /// rendering, debugging, dumping the glyph atlas) -- regular
    /// text drawing goes through `za::Text` and never needs it.
    ///
    /// \return Reference to the glyph texture
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Texture& getTexture();

    ////////////////////////////////////////////////////////////
    /// \brief `const` overload of `getTexture`
    ///
    /// \return Const reference to the glyph texture
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Texture& getTexture() const;

    ////////////////////////////////////////////////////////////
    /// \private
    ///
    /// \brief Create a font from font handles and a family name
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit Font(za::PassKey<Font>&&, FontFace&& fontFace, TextureAtlas* textureAtlas);

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::UniquePtr<Impl> m_impl; //!< Implementation details
    // TODO P0: does this need address stability?

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_DEFINE_LIFETIME_DEPENDEE(Font, Text);
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::Font
/// \ingroup graphics
///
/// `za::Font` represents a font that has been opened from a file,
/// from memory, or from a custom stream. The most common font
/// formats are supported -- see `openFromFile` for the full list.
///
/// Once opened, a `za::Font` instance exposes three kinds of
/// information about the font:
/// \li Global metrics, such as ascent, descent, and line spacing
/// \li Per-glyph metrics, such as bounding box, advance, and kerning
/// \li Rasterized pixel representations of individual glyphs
///
/// Fonts on their own are not very useful: they hold the font data
/// but do not draw anything. To render text, pair a `za::Font` with
/// a `za::Text` object, which combines the glyph data and metrics
/// with a string and a transform to produce vertices for the
/// renderer. The same font can be bound to as many `za::Text`
/// instances as needed.
///
/// `za::Font` is a heavy resource: opening it allocates an internal
/// glyph cache, and rasterizing glyphs touches FreeType. Use
/// references (or, ideally, a shared `za::TextureAtlas`) rather
/// than copies whenever possible.
///
/// Lifetime: in Zancle, `za::Text` does not copy the font; it only
/// references it via `za::Font&`. The font must therefore outlive
/// every `za::Text` that uses it. Debug builds with
/// `ZA_ENABLE_LIFETIME_TRACKING` will detect violations
/// automatically.
///
/// Usage example:
/// \code
/// // Open a new font.
/// const auto font = za::Font::openFromFile("arial.ttf").value();
///
/// // Create a text from the font using designated initializers.
/// const za::Text text1{font, {.string = "Hello",  .characterSize = 30u}};
/// const za::Text text2{font, {.string = "World!", .characterSize = 50u, .italic = true}};
/// \endcode
///
/// Apart from opening font files and passing them to `za::Text`,
/// you usually do not need to interact with this class directly.
/// The `getInfo`, `getGlyph`, `getKerning`, and metric accessors
/// can be useful for advanced custom rendering.
///
/// Note that bitmap fonts are not scalable: they only contain a
/// fixed set of pre-rendered sizes. If `za::Text` requests a size
/// that the bitmap font does not provide, the result may be empty
/// or look incorrect.
///
/// \see `za::Text`, `za::FontFace`, `za::TextureAtlas`
///
////////////////////////////////////////////////////////////
