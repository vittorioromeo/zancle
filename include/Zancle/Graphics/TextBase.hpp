#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/Priv/TransformableMacros.hpp"
#include "Zancle/Graphics/TransformableMixin.hpp"
#include "Zancle/Graphics/Vertex.hpp"
#include "Zancle/Graphics/VertexSpan.hpp"

#include "Zancle/Mixin/GlobalAnchorPointMixin.hpp"
#include "Zancle/Mixin/LocalAnchorPointMixin.hpp"

#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class RenderTarget;
struct RenderStates;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Base class shared by `za::Text` and `za::GlyphMappedText`
///
/// Holds all data and methods shared between `Text` (legacy
/// lazy-loading path) and `GlyphMappedText` (explicit preloading
/// path). The derived type is deduced at call time via C++23
/// explicit object parameters on the hot methods that need it.
///
/// The deducing-this method templates are *declared* here but
/// *defined* in `TextBase.inl`, force-instantiated for `za::Text`
/// and `za::GlyphMappedText` inside `TextBase.cpp`. This keeps the
/// public header free of heavy rendering includes (`RenderTarget.hpp`,
/// `TextUtils.hpp`, etc.).
///
/// Note: users who further derive from `za::Text` or
/// `za::GlyphMappedText` **must** `#include "Zancle/Graphics/TextBase.inl"`
/// in the TU that defines the derived type, so that per-derived-type
/// template instantiations of `draw`, `findCharacterPos`, ... can
/// be generated. Code that only uses `za::Text` / `za::GlyphMappedText`
/// directly never needs the `.inl`.
///
/// Derived classes must provide:
/// - `const auto& getFontSource() const` -- returns the glyph/metrics provider
///   (a `za::Font&` for `Text`; `GlyphMappedText` returns itself)
/// - `unsigned int getCharacterSize() const`
/// - `float getOutlineThickness() const`
/// - `const Texture& getTexture() const`
/// - `bool isBold() const`
///
////////////////////////////////////////////////////////////
class ZA_GRAPHICS_API TextBase : public TransformableMixin, public GlobalAnchorPointMixin, public LocalAnchorPointMixin
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Set the text's string
    ///
    /// The `string` argument is a `za::Utf8String`. It can be
    /// implicitly constructed from a UTF-8 encoded `const char*`,
    /// `za::StringView`, or `za::String`. For text containing
    /// non-ASCII characters, the source bytes must already be UTF-8.
    ///
    /// \code
    /// text.setString("hello");
    /// text.setString(u8"カタツムリ");
    /// \endcode
    ///
    /// A text's string is empty by default.
    ///
    /// \param string New string
    ///
    /// \see `getString`
    ///
    ////////////////////////////////////////////////////////////
    void setString(const Utf8String& string);

    ////////////////////////////////////////////////////////////
    /// \brief Set the letter spacing factor
    ///
    /// The default spacing between letters is defined by the font.
    /// This factor doesn't directly apply to the existing
    /// spacing between each character, it rather adds a fixed
    /// space between them which is calculated from the font
    /// metrics and the character size.
    /// Note that factors below 1 (including negative numbers) bring
    /// characters closer to each other.
    /// By default the letter spacing factor is 1.
    ///
    /// \param spacingFactor New letter spacing factor
    ///
    /// \see `getLetterSpacing`
    ///
    ////////////////////////////////////////////////////////////
    void setLetterSpacing(float spacingFactor);

    ////////////////////////////////////////////////////////////
    /// \brief Set the line spacing factor
    ///
    /// The default spacing between lines is defined by the font.
    /// This method enables you to set a factor for the spacing
    /// between lines. By default the line spacing factor is 1.
    ///
    /// \param spacingFactor New line spacing factor
    ///
    /// \see `getLineSpacing`
    ///
    ////////////////////////////////////////////////////////////
    void setLineSpacing(float spacingFactor);

    ////////////////////////////////////////////////////////////
    /// \brief Set whether the text is italic
    ///
    /// \param italic True to enable italic, false to disable
    ///
    /// \see `isItalic`
    ///
    ////////////////////////////////////////////////////////////
    void setItalic(bool italic);

    ////////////////////////////////////////////////////////////
    /// \brief Set whether the text is underlined
    ///
    /// \param underlined True to enable underline, false to disable
    ///
    /// \see `isUnderlined`
    ///
    ////////////////////////////////////////////////////////////
    void setUnderlined(bool underlined);

    ////////////////////////////////////////////////////////////
    /// \brief Set whether the text has a strike through
    ///
    /// \param strikeThrough True to enable strike through, false to disable
    ///
    /// \see `isStrikeThrough`
    ///
    ////////////////////////////////////////////////////////////
    void setStrikeThrough(bool strikeThrough);

    ////////////////////////////////////////////////////////////
    /// \brief Set the fill color of the text
    ///
    /// By default, the text's fill color is opaque white.
    /// Setting the fill color to a transparent color with an outline
    /// will cause the outline to be displayed in the fill area of the text.
    ///
    /// \param color New fill color of the text
    ///
    /// \see `getFillColor`
    ///
    ////////////////////////////////////////////////////////////
    void setFillColor(Color color);

    ////////////////////////////////////////////////////////////
    /// \brief Set the outline color of the text
    ///
    /// By default, the text's outline color is opaque black.
    ///
    /// \param color New outline color of the text
    ///
    /// \see `getOutlineColor`
    ///
    ////////////////////////////////////////////////////////////
    void setOutlineColor(Color color);

    ////////////////////////////////////////////////////////////
    /// \brief Set the fill color alpha channel of the text
    ///
    /// \param color New fill color alpha channel of the text
    ///
    /// \see `getFillColorAlpha`
    ///
    ////////////////////////////////////////////////////////////
    void setFillColorAlpha(za::U8 alpha);

    ////////////////////////////////////////////////////////////
    /// \brief Set the outline color alpha channel of the text
    ///
    /// \param color New outline color alpha channel of the text
    ///
    /// \see `getOutlineColorAlpha`
    ///
    ////////////////////////////////////////////////////////////
    void setOutlineColorAlpha(za::U8 alpha);

    ////////////////////////////////////////////////////////////
    /// \brief Get the text's string
    ///
    /// The returned string is a `za::Utf8String`. It implicitly
    /// converts to `za::StringView` for any byte-oriented sink.
    ///
    /// \return Text's string (UTF-8)
    ///
    /// \see `setString`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Utf8String& getString() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the size of the letter spacing factor
    ///
    /// \return Size of the letter spacing factor
    ///
    /// \see `setLetterSpacing`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getLetterSpacing() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the size of the line spacing factor
    ///
    /// \return Size of the line spacing factor
    ///
    /// \see `setLineSpacing`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getLineSpacing() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get whether the text is italic
    ///
    /// \return True if the text is italic
    ///
    /// \see `setItalic`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isItalic() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get whether the text is underlined
    ///
    /// \return True if the text is underlined
    ///
    /// \see `setUnderlined`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isUnderlined() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get whether the text has a strike through
    ///
    /// \return True if the text has a strike through
    ///
    /// \see `setStrikeThrough`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isStrikeThrough() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the fill color of the text
    ///
    /// \return Fill color of the text
    ///
    /// \see `setFillColor`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Color getFillColor() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the outline color of the text
    ///
    /// \return Outline color of the text
    ///
    /// \see `setOutlineColor`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Color getOutlineColor() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the fill color alpha channel of the text
    ///
    /// \return Fill color alpha channel of the text
    ///
    /// \see `setFillColorAlpha`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::U8 getFillColorAlpha() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the outline color alpha channel of the text
    ///
    /// \return Outline color alpha channel of the text
    ///
    /// \see `setOutlineColorAlpha`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::U8 getOutlineColorAlpha() const;

    ////////////////////////////////////////////////////////////
    /// \brief Return the position of the `index`-th character
    ///
    /// This function computes the visual position of a character
    /// from its index in the string. The returned position is
    /// in global coordinates (translation, rotation, scale and
    /// origin are applied).
    /// If `index` is out of range, the position of the end of
    /// the string is returned.
    ///
    /// \param index Index of the character
    ///
    /// \return Position of the character
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
    [[nodiscard]] Vec2f findCharacterPos(this const Self& self, za::SizeT index);


    ////////////////////////////////////////////////////////////
    /// \brief Get the local bounding rectangle of the entity
    ///
    /// The returned rectangle is in local coordinates, which means
    /// that it ignores the transformations (translation, rotation,
    /// scale, etc...) that are applied to the entity.
    ///
    /// \return Local bounding rectangle of the entity
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
    [[nodiscard]] const Rect2f& getLocalBounds(this const Self& self);

    ////////////////////////////////////////////////////////////
    /// \brief Get the global bounding rectangle of the entity
    ///
    /// The returned rectangle is in global coordinates, which means
    /// that it takes into account the transformations (translation,
    /// rotation, scale, etc...) that are applied to the entity.
    ///
    /// \return Global bounding rectangle of the entity
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
    [[nodiscard]] Rect2f getGlobalBounds(this const Self& self);

    ////////////////////////////////////////////////////////////
    /// \brief Draw the text to a render target
    ///
    /// \param target Render target to draw to
    /// \param states Current render states
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
    void draw(this const Self& self, RenderTarget& target, RenderStates states);

    ////////////////////////////////////////////////////////////
    /// \brief Get a read-only span to the text's vertices.
    ///
    /// The vertices are recalculated, if necessary, when this function
    /// is invoked. Therefore it is not thread-safe.
    ///
    /// The returned span fulfills the following properties:
    ///
    /// - The subrange `[0, getFillVerticesStartIndex()]` contains
    ///   all the text outline vertices, if any.
    ///
    /// - The remaining subrange contains all text fill vertices, if any.
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
    [[nodiscard]] ConstVertexSpan getVertices(this const Self& self);

    ////////////////////////////////////////////////////////////
    /// \brief Get a mutable span to the text's vertices.
    ///
    /// \see `getVertices`
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
    [[nodiscard]] VertexSpan getVerticesMut(this Self& self);

    ////////////////////////////////////////////////////////////
    /// \brief Get the index where the fill vertices start in `getVertices()`
    ///
    /// The vertex array returned by `getVertices` is laid out as
    /// `[outline vertices ... fill vertices]`. This function
    /// returns the index of the first fill vertex, which is also
    /// the count of outline vertices.
    ///
    /// \return Start index of the fill vertex sub-range
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] za::SizeT getFillVerticesStartIndex() const
    {
        return m_fillVerticesStartIndex;
    }

protected:
    ////////////////////////////////////////////////////////////
    /// \brief Construct from common text data fields
    ///
    ////////////////////////////////////////////////////////////
    template <typename TData>
    explicit TextBase(const TData& data) :
        m_string(data.string),
        m_letterSpacing(data.letterSpacing),
        m_lineSpacing(data.lineSpacing),
        m_fillColor(data.fillColor),
        m_outlineColor(data.outlineColor),
        position{data.position},
        scale{data.scale},
        origin{data.origin},
        rotation{data.rotation},
        m_italic(data.italic),
        m_underlined(data.underlined),
        m_strikeThrough(data.strikeThrough),
        m_geometryNeedUpdate{true}
    {
    }

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~TextBase() = default;

    ////////////////////////////////////////////////////////////
    /// \brief Copy operations
    ///
    ////////////////////////////////////////////////////////////
    TextBase(const TextBase&)            = default;
    TextBase& operator=(const TextBase&) = default;

    ////////////////////////////////////////////////////////////
    /// \brief Move operations
    ///
    ////////////////////////////////////////////////////////////
    TextBase(TextBase&&) noexcept            = default;
    TextBase& operator=(TextBase&&) noexcept = default;

    ////////////////////////////////////////////////////////////
    /// \brief Make sure the text's geometry is updated
    ///
    /// All the attributes related to rendering are cached, such
    /// that the geometry is only updated when necessary.
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self>
    void ensureGeometryUpdate(this const Self& self);

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    Utf8String m_string; //!< String to display

    mutable za::Vector<Vertex> m_vertices;                 //!< Vertex array containing the outline and fill geometry
    mutable Rect2f             m_bounds;                   //!< Bounding rectangle of the text (in local coordinates)
    mutable za::SizeT          m_fillVerticesStartIndex{}; //!< Index in the vertex array where the fill vertices start

    float m_letterSpacing{1.f};         //!< Spacing factor between letters
    float m_lineSpacing{1.f};           //!< Spacing factor between lines
    Color m_fillColor{Color::White};    //!< Text fill color
    Color m_outlineColor{Color::Black}; //!< Text outline color

public:
    ZA_DEFINE_TRANSFORMABLE_DATA_MEMBERS;

protected:
    bool         m_italic{false};        //!< Italic characters
    bool         m_underlined{false};    //!< Underlined characters
    bool         m_strikeThrough{false}; //!< Strike through characters
    mutable bool m_geometryNeedUpdate{}; //!< Does the geometry need to be recomputed?
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::TextBase
/// \ingroup graphics
///
/// `za::TextBase` is the base shared by `za::Text` and
/// `za::GlyphMappedText`. It implements the parts of the text
/// rendering pipeline that do not depend on the source of the
/// glyphs:
///
/// \li the string itself,
/// \li letter spacing, line spacing, italic/underline/strike,
/// \li fill and outline colors (with separate alpha setters),
/// \li the cached vertex buffer that backs the rendered text,
/// \li the public transformable members,
/// \li bounds queries (`getLocalBounds`, `getGlobalBounds`),
/// \li per-character position lookup (`findCharacterPos`).
///
/// Concrete subclasses provide:
/// \li `getFontSource()` -- returns the glyph/metrics provider: a
///     `za::Font&` for `za::Text` (legacy lazy-loading path), or the
///     `za::GlyphMappedText` itself (which forwards to its
///     `za::GlyphMapping` and `za::FontFace`).
/// \li `getCharacterSize()`, `getOutlineThickness()`, `isBold()`
/// \li `getTexture()` -- the glyph atlas texture.
///
/// `TextBase` is not meant to be used directly. Use `za::Text`
/// or `za::GlyphMappedText`, depending on whether your text is
/// dynamic or precomputed.
///
/// \see `za::Text`, `za::GlyphMappedText`, `za::TextData`
///
////////////////////////////////////////////////////////////
