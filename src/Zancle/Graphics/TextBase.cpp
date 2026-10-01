// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/TextBase.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/GlyphMappedText.hpp" // IWYU pragma: keep
#include "Zancle/Graphics/Text.hpp"            // IWYU pragma: keep
#include "Zancle/Graphics/Vertex.hpp"

#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
#include "Zancle/Graphics/TextBase.inl" // IWYU pragma: keep


namespace za
{
////////////////////////////////////////////////////////////
void TextBase::setString(const Utf8String& string)
{
    if (m_string == string)
        return;

    m_string             = string;
    m_geometryNeedUpdate = true;
}


////////////////////////////////////////////////////////////
void TextBase::setLetterSpacing(const float spacingFactor)
{
    if (m_letterSpacing == spacingFactor)
        return;

    m_letterSpacing      = spacingFactor;
    m_geometryNeedUpdate = true;
}


////////////////////////////////////////////////////////////
void TextBase::setLineSpacing(const float spacingFactor)
{
    if (m_lineSpacing == spacingFactor)
        return;

    m_lineSpacing        = spacingFactor;
    m_geometryNeedUpdate = true;
}


////////////////////////////////////////////////////////////
void TextBase::setItalic(const bool italic)
{
    if (m_italic == italic)
        return;

    m_italic             = italic;
    m_geometryNeedUpdate = true;
}


////////////////////////////////////////////////////////////
void TextBase::setUnderlined(const bool underlined)
{
    if (m_underlined == underlined)
        return;

    m_underlined         = underlined;
    m_geometryNeedUpdate = true;
}


////////////////////////////////////////////////////////////
void TextBase::setStrikeThrough(const bool strikeThrough)
{
    if (m_strikeThrough == strikeThrough)
        return;

    m_strikeThrough      = strikeThrough;
    m_geometryNeedUpdate = true;
}


////////////////////////////////////////////////////////////
void TextBase::setFillColor(const Color color)
{
    if (color == m_fillColor)
        return;

    m_fillColor = color;

    if (!m_geometryNeedUpdate)
        for (za::SizeT i = m_fillVerticesStartIndex; i < m_vertices.size(); ++i)
            m_vertices[i].color = m_fillColor;
}


////////////////////////////////////////////////////////////
void TextBase::setOutlineColor(const Color color)
{
    if (color == m_outlineColor)
        return;

    m_outlineColor = color;

    if (!m_geometryNeedUpdate)
        for (za::SizeT i = 0; i < m_fillVerticesStartIndex; ++i)
            m_vertices[i].color = m_outlineColor;
}


////////////////////////////////////////////////////////////
void TextBase::setFillColorAlpha(const za::U8 alpha)
{
    if (m_fillColor.a == alpha)
        return;

    m_fillColor.a = alpha;

    if (!m_geometryNeedUpdate)
        for (za::SizeT i = m_fillVerticesStartIndex; i < m_vertices.size(); ++i)
            m_vertices[i].color.a = m_fillColor.a;
}


////////////////////////////////////////////////////////////
void TextBase::setOutlineColorAlpha(const za::U8 alpha)
{
    if (m_outlineColor.a == alpha)
        return;

    m_outlineColor.a = alpha;

    if (!m_geometryNeedUpdate)
        for (za::SizeT i = 0; i < m_fillVerticesStartIndex; ++i)
            m_vertices[i].color.a = m_outlineColor.a;
}


////////////////////////////////////////////////////////////
const Utf8String& TextBase::getString() const
{
    return m_string;
}


////////////////////////////////////////////////////////////
float TextBase::getLetterSpacing() const
{
    return m_letterSpacing;
}


////////////////////////////////////////////////////////////
float TextBase::getLineSpacing() const
{
    return m_lineSpacing;
}


////////////////////////////////////////////////////////////
bool TextBase::isItalic() const
{
    return m_italic;
}


////////////////////////////////////////////////////////////
bool TextBase::isUnderlined() const
{
    return m_underlined;
}


////////////////////////////////////////////////////////////
bool TextBase::isStrikeThrough() const
{
    return m_strikeThrough;
}


////////////////////////////////////////////////////////////
Color TextBase::getFillColor() const
{
    return m_fillColor;
}


////////////////////////////////////////////////////////////
Color TextBase::getOutlineColor() const
{
    return m_outlineColor;
}


////////////////////////////////////////////////////////////
za::U8 TextBase::getFillColorAlpha() const
{
    return m_fillColor.a;
}


////////////////////////////////////////////////////////////
za::U8 TextBase::getOutlineColorAlpha() const
{
    return m_outlineColor.a;
}

} // namespace za


////////////////////////////////////////////////////////////
// Explicit template instantiations of the deducing-this methods
// for the two concrete derived types. Emits the symbols once in
// `zancle-graphics` so that downstream TUs only need the declarations
// in `TextBase.hpp` (no heavy rendering includes). The explicit object
// parameters are spelled with `this`, as MSVC requires.
////////////////////////////////////////////////////////////
#define ZA_PRIV_INSTANTIATE_TEXTBASE_DEDUCING_THIS(TDerived)                                                               \
    template ZA_GRAPHICS_API za::Vec2f za::TextBase::findCharacterPos<TDerived>(this const TDerived&, za::SizeT);          \
    template ZA_GRAPHICS_API const za::Rect2f& za::TextBase::getLocalBounds<TDerived>(this const TDerived&);               \
    template ZA_GRAPHICS_API za::Rect2f za::TextBase::getGlobalBounds<TDerived>(this const TDerived&);                     \
    template ZA_GRAPHICS_API void za::TextBase::draw<TDerived>(this const TDerived&, za::RenderTarget&, za::RenderStates); \
    template ZA_GRAPHICS_API za::ConstVertexSpan za::TextBase::getVertices<TDerived>(this const TDerived&);                \
    template ZA_GRAPHICS_API za::VertexSpan za::TextBase::getVerticesMut<TDerived>(this TDerived&);                        \
    template ZA_GRAPHICS_API void           za::TextBase::ensureGeometryUpdate<TDerived>(this const TDerived&)

ZA_PRIV_INSTANTIATE_TEXTBASE_DEDUCING_THIS(za::Text);
ZA_PRIV_INSTANTIATE_TEXTBASE_DEDUCING_THIS(za::GlyphMappedText);

#undef ZA_PRIV_INSTANTIATE_TEXTBASE_DEDUCING_THIS
