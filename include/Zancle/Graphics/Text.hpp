#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/TextBase.hpp"
#include "Zancle/Graphics/TextData.hpp"

#include "Zancle/Lifetime/LifetimeDependant.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class Font;
class Texture;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief High-level drawable text backed by `za::Font`
///
/// `za::Text` couples a `za::Font` with a `TextData` payload
/// (string, character size, style, colors, transform, ...) and
/// produces vertex data on demand. Glyphs are rasterized lazily
/// by the underlying font as new characters or sizes are
/// requested.
///
/// For very high-throughput rendering of mostly-static strings,
/// consider `za::GlyphMappedText` instead, which precomputes the
/// glyph layout once and replays it on every draw.
///
////////////////////////////////////////////////////////////
class ZA_GRAPHICS_API Text : public TextBase
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Initialization data for `za::Text`
    ///
    /// Alias for `za::TextData`. Use designated initializers to
    /// configure only the fields you care about (the rest fall
    /// back to sensible defaults).
    ///
    ////////////////////////////////////////////////////////////
    using Data = TextData;

    ////////////////////////////////////////////////////////////
    /// \brief Construct a text from a font and a `Data` initializer
    ///
    /// `font` is referenced (not copied) and must outlive the
    /// text. In debug builds with
    /// `ZA_ENABLE_LIFETIME_TRACKING`, the lifetime relationship
    /// is enforced automatically.
    ///
    /// Note that bitmap fonts are not scalable: they only contain
    /// a fixed set of pre-rendered sizes. If `data.characterSize`
    /// does not match a size the bitmap font supports, the result
    /// may be empty or look incorrect.
    ///
    /// \param font Font to use (referenced for the lifetime of the text)
    /// \param data Initial text properties
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Text(const Font& font, const Data& data);

    ////////////////////////////////////////////////////////////
    /// \brief Disallow construction from a temporary font
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Text(const Font&& font, const Data& data) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~Text();

    ////////////////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    ////////////////////////////////////////////////////////////
    Text(const Text&);

    ////////////////////////////////////////////////////////////
    /// \brief Copy assignment
    ///
    ////////////////////////////////////////////////////////////
    Text& operator=(const Text&);

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    Text(Text&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    Text& operator=(Text&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Set the text's font
    ///
    /// The `font` argument refers to a font that must
    /// exist as long as the text uses it. Indeed, the text
    /// doesn't store its own copy of the font, but rather keeps
    /// a pointer to the one that you passed to this function.
    /// If the font is destroyed and the text tries to
    /// use it, the behavior is undefined.
    ///
    /// \param font New font
    ///
    /// \see `getFont`
    ///
    ////////////////////////////////////////////////////////////
    void setFont(const Font& font);

    ////////////////////////////////////////////////////////////
    /// \brief Disallow setting from a temporary font
    ///
    ////////////////////////////////////////////////////////////
    void setFont(Font&& font) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Set whether the text is bold
    ///
    /// Bold affects which glyph variant is loaded from the font.
    /// By default, text is not bold.
    ///
    /// \param bold True to enable bold, false to disable
    ///
    /// \see `isBold`
    ///
    ////////////////////////////////////////////////////////////
    void setBold(bool bold);

    ////////////////////////////////////////////////////////////
    /// \brief Set the character size
    ///
    /// Note that if the used font is a bitmap font, it is not
    /// scalable, thus not all requested sizes will be available
    /// to use. This needs to be taken into consideration when
    /// setting the character size. If you need to display text
    /// of a certain size, make sure the corresponding bitmap
    /// font that supports that size is used.
    ///
    /// \param size New character size, in pixels
    ///
    /// \see `getCharacterSize`
    ///
    ////////////////////////////////////////////////////////////
    void setCharacterSize(unsigned int size);

    ////////////////////////////////////////////////////////////
    /// \brief Set the thickness of the text's outline
    ///
    /// By default, the outline thickness is 0.
    ///
    /// Be aware that using a negative value for the outline
    /// thickness will cause distorted rendering.
    ///
    /// \param thickness New outline thickness, in pixels
    ///
    /// \see `getOutlineThickness`
    ///
    ////////////////////////////////////////////////////////////
    void setOutlineThickness(float thickness);

    ////////////////////////////////////////////////////////////
    /// \brief Get the text's font
    ///
    /// The returned reference is const, which means that you
    /// cannot modify the font when you get it from this function.
    ///
    /// \return Reference to the text's font
    ///
    /// \see `setFont`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Font& getFont() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get whether the text is bold
    ///
    /// \return True if the text is bold
    ///
    /// \see `setBold`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isBold() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the font this text was created with
    ///
    /// Equivalent to `getFont` -- present so that `Text` and
    /// `GlyphMappedText` can be used interchangeably from
    /// templated code that needs the underlying font.
    ///
    /// \return Reference to the source font
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Font& getFontSource() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the glyph atlas texture used by this text
    ///
    /// Returns the texture into which the underlying `za::Font`
    /// rasterizes glyphs. Useful for drawing custom batches that
    /// share the same atlas as the text, or for debugging by
    /// dumping the atlas to disk.
    ///
    /// \return Reference to the glyph atlas texture
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
    const Font*  m_font{};                //!< Font used to display the string
    unsigned int m_characterSize{};       //!< Base size of characters, in pixels
    float        m_outlineThickness{0.f}; //!< Thickness of the text's outline
    bool         m_bold{false};           //!< Bold characters

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_DEFINE_LIFETIME_DEPENDANT(Font);
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::Text
/// \ingroup graphics
///
/// `za::Text` is a drawable class that allows to easily display
/// some text with custom style and color on a render target.
///
/// It inherits all the functions from `za::Transformable`:
/// position, rotation, scale, origin. It also adds text-specific
/// properties such as the font to use, the character size,
/// the font style (bold, italic, underlined and strike through), the
/// text color, the outline thickness, the outline color, the character
/// spacing, the line spacing and the text to display of course.
/// It also provides convenience functions to calculate the
/// graphical size of the text, or to get the global position
/// of a given character.
///
/// `za::Text` works in combination with the `za::Font` class, which
/// loads and provides the glyphs (visual characters) of a given font.
///
/// The separation of `za::Font` and `za::Text` allows more flexibility
/// and better performances: indeed a `za::Font` is a heavy resource,
/// and any operation on it is slow (often too slow for real-time
/// applications). On the other side, a `za::Text` is a lightweight
/// object which can combine the glyphs data and metrics of a `za::Font`
/// to display any text on a render target.
///
/// It is important to note that the `za::Text` instance doesn't
/// copy the font that it uses, it only keeps a reference to it.
/// Thus, a `za::Font` must not be destructed while it is
/// used by a `za::Text` (i.e. never write a function that
/// uses a local `za::Font` instance for creating a text).
///
/// See also the note on coordinates and undistorted rendering in `za::Transformable`.
///
/// Usage example:
/// \code
/// // Open a font.
/// const auto font = za::Font::openFromFile("arial.ttf").value();
///
/// // Create a text using designated initializers.
/// za::Text text{font, {
///     .string        = "hello",
///     .characterSize = 30u,
///     .fillColor     = za::Color::Red,
///     .bold          = true,
/// }};
///
/// // Draw it.
/// window.draw(text);
/// \endcode
///
/// \see `za::Font`, `za::Transformable`, `za::TextData`,
///      `za::GlyphMappedText`
///
////////////////////////////////////////////////////////////
