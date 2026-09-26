#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/Glyph.hpp"

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
class InputStream;
class Path;
class GlyphMappedText;
class TextureAtlas;

struct FontInfo;
struct GlyphLoadSettings;
struct GlyphMapping;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Lower-level font handle that exposes metrics and on-demand glyph rasterization
///
/// `za::FontFace` wraps a FreeType face. It owns the font file
/// data and rasterizes glyphs into a `za::TextureAtlas` on demand.
///
/// Most user code should prefer `za::Font`, which combines a
/// `FontFace` with an internal glyph cache and a backing texture.
/// `FontFace` is the building block used by `za::Font` and by the
/// `za::GlyphMappedText` low-level text path; it is exposed
/// publicly for advanced users who want to share a single font
/// face across multiple subsystems or perform custom glyph packing.
///
/// `FontFace` is move-only.
///
////////////////////////////////////////////////////////////
class ZA_GRAPHICS_API FontFace
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~FontFace();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor (`FontFace` is move-only)
    ///
    ////////////////////////////////////////////////////////////
    FontFace(const FontFace& rhs) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment (`FontFace` is move-only)
    ///
    ////////////////////////////////////////////////////////////
    FontFace& operator=(const FontFace& rhs) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    FontFace(FontFace&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment operator
    ///
    ////////////////////////////////////////////////////////////
    FontFace& operator=(FontFace&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Open a font face from a file on disk
    ///
    /// The supported font formats are the same as `za::Font`.
    ///
    /// \param filename Path of the font file to load
    ///
    /// \return `FontFace` on success, `za::nullOpt` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<FontFace> openFromFile(const Path& filename);

    ////////////////////////////////////////////////////////////
    /// \brief Open a font face from an encoded buffer in memory
    ///
    /// The supported font formats are the same as `za::Font`. The
    /// buffer pointed to by `data` must remain valid for the
    /// lifetime of the `FontFace`.
    ///
    /// \param data        Pointer to the encoded font bytes in memory
    /// \param sizeInBytes Size of the data, in bytes
    ///
    /// \return `FontFace` on success, `za::nullOpt` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<FontFace> openFromMemory(const void* data, za::SizeT sizeInBytes);

    ////////////////////////////////////////////////////////////
    /// \brief Open a font face from a custom input stream
    ///
    /// The supported font formats are the same as `za::Font`. The
    /// stream must remain accessible for the lifetime of the
    /// `FontFace`.
    ///
    /// \param stream Source stream to read encoded font data from
    ///
    /// \return `FontFace` on success, `za::nullOpt` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<FontFace> openFromStream(InputStream& stream);

    ////////////////////////////////////////////////////////////
    /// \brief Get general information about the font (family name, etc.)
    ///
    /// \return Reference to the internal `za::FontInfo`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const FontInfo& getInfo() const;

    ////////////////////////////////////////////////////////////
    /// \brief Check whether this font face contains a glyph for a given code point
    ///
    /// \param codePoint Unicode code point to check
    ///
    /// \return `true` if the glyph exists, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool hasGlyph(char32_t codePoint) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the kerning offset between two glyphs
    ///
    /// Kerning is an extra (typically negative) horizontal offset
    /// applied between two consecutive glyphs to make the pair
    /// look more "natural" (e.g. "AV").
    ///
    /// \param first         Unicode code point of the first character
    /// \param second        Unicode code point of the second character
    /// \param characterSize Reference character size, in pixels
    /// \param bold          Whether to query the bold version
    ///
    /// \return Kerning offset, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getKerning(char32_t first, char32_t second, unsigned int characterSize, bool bold) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the ascent (largest distance from baseline to top of any glyph)
    ///
    /// \param characterSize Reference character size, in pixels
    ///
    /// \return Ascent, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getAscent(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the descent (largest distance from baseline to bottom of any glyph)
    ///
    /// The descent shares the same axis as the ascent, so values
    /// below the baseline are negative.
    ///
    /// \param characterSize Reference character size, in pixels
    ///
    /// \return Descent, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getDescent(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the vertical distance between two consecutive lines of text
    ///
    /// \param characterSize Reference character size, in pixels
    ///
    /// \return Line spacing, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getLineSpacing(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the vertical offset of the underline relative to the baseline
    ///
    /// \param characterSize Reference character size, in pixels
    ///
    /// \return Underline position, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getUnderlinePosition(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the vertical thickness of the underline
    ///
    /// \param characterSize Reference character size, in pixels
    ///
    /// \return Underline thickness, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getUnderlineThickness(unsigned int characterSize) const;

    ////////////////////////////////////////////////////////////
    /// \brief Rasterize a single glyph and pack it into a texture atlas
    ///
    /// Renders the requested glyph at the requested character size
    /// and style, then inserts the resulting bitmap into `atlas`.
    /// On success, the returned `za::Glyph` carries the texture
    /// rectangle inside the atlas plus the glyph metrics.
    ///
    /// \param atlas            Atlas to upload the rasterized bitmap into
    /// \param codePoint        Unicode code point to rasterize
    /// \param characterSize    Reference character size, in pixels
    /// \param bold             Whether to rasterize the bold version
    /// \param outlineThickness Outline thickness; pass `0` for the fill glyph
    ///
    /// \return Rasterized `za::Glyph` on success, `za::nullOpt` if the atlas is full
    ///
    /// \note Not thread-safe due to internal FreeType caching.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<Glyph> rasterizeAndPackGlyph(
        TextureAtlas& atlas,
        char32_t      codePoint,
        unsigned int  characterSize,
        bool          bold,
        float         outlineThickness) const;

    ////////////////////////////////////////////////////////////
    /// \brief Rasterize a batch of glyphs into a texture atlas and produce a mapping
    ///
    /// This is the building block of `za::GlyphMappedText`: it
    /// rasterizes a configurable set of glyphs (described by
    /// `settings`) and inserts them into `atlas` in one pass. The
    /// returned `za::GlyphMapping` carries the index → glyph
    /// translation that the rendering path uses.
    ///
    /// If `settings.outlineThickness != 0`, both the fill glyphs
    /// and their matching outline glyphs are loaded.
    ///
    /// \param atlas    Atlas to upload the rasterized bitmaps into
    /// \param settings Glyph load settings (range, character size, style, ...)
    ///
    /// \return `GlyphMapping` on success, `za::nullOpt` if the atlas is full
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<GlyphMapping> loadGlyphs(TextureAtlas& atlas, const GlyphLoadSettings& settings) const;

private:
    ////////////////////////////////////////////////////////////
    /// \brief Open from stream and print errors with custom message
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<FontFace> openFromStreamImpl(InputStream& stream, const char* type);

public:
    ////////////////////////////////////////////////////////////
    /// \private
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit FontFace(za::PassKey<FontFace>&&);

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::UniquePtr<Impl> m_impl;

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_DEFINE_LIFETIME_DEPENDEE(FontFace, GlyphMappedText);
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::FontFace
/// \ingroup graphics
///
/// `za::FontFace` is the lower-level building block of Zancle's
/// text rendering pipeline. It owns a FreeType face plus the
/// font's binary data, and exposes:
///
/// \li global metrics (`getAscent`, `getDescent`, `getLineSpacing`,
///     `getUnderlinePosition`, `getUnderlineThickness`),
/// \li per-glyph queries (`hasGlyph`, `getKerning`),
/// \li glyph rasterization helpers that pack rendered bitmaps
///     into a `za::TextureAtlas`
///     (`rasterizeAndPackGlyph`, `loadGlyphs`).
///
/// `za::FontFace` does not maintain a glyph cache of its own and
/// does not own a backing texture. The atlas it draws into is
/// passed in by the caller. This makes `FontFace` a good fit when
/// you want to share a single atlas across multiple fonts (or
/// across fonts and other resources) and avoid one texture per
/// font.
///
/// Most user code does not need this level of control: prefer
/// `za::Font` (which composes a `FontFace` with an internal cache
/// and atlas) and `za::Text`. `FontFace` is exposed for advanced
/// users and for the `za::GlyphMappedText` low-level text path.
///
/// `za::FontFace` is move-only.
///
/// \see `za::Font`, `za::TextureAtlas`, `za::GlyphMappedText`, `za::Text`
///
////////////////////////////////////////////////////////////
