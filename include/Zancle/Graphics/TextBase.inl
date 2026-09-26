#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/DrawQuadsSettings.hpp"
#include "Zancle/Graphics/Glyph.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/TextBase.hpp"
#include "Zancle/Graphics/TextUtils.hpp"
#include "Zancle/Graphics/VertexSpan.hpp"

#include "Zancle/String/Utf8StringCodepoints.hpp" // IWYU pragma: keep

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Base/SizeT.hpp"


namespace za
{
////////////////////////////////////////////////////////////
template <typename Self>
Vec2f TextBase::findCharacterPos(this const Self& self, za::SizeT index)
{
    const auto& fontSource = self.getFontSource();
    const auto  charSize   = self.getCharacterSize();
    const bool  isBold     = self.isBold();

    const auto [whitespaceWidth,
                letterSpacing,
                lineSpacing] = TextUtils::precomputeSpacingConstants(fontSource, isBold, charSize, self.m_letterSpacing, self.m_lineSpacing);

    Vec2f     characterPos;
    char32_t  prevChar = 0;
    za::SizeT i        = 0u;

    for (const char32_t curChar : self.m_string.codepoints())
    {
        if (i++ >= index)
            break;

        // Skip the \r char to match the layout code, which skips it before any
        // kerning accumulation and does not let it become `prevChar` (see
        // `TextUtils::createTextGeometryAndGetBounds`)
        if (curChar == U'\r')
            continue;

        characterPos.x += fontSource.getKerning(prevChar, curChar, charSize, isBold);
        prevChar = curChar;

        switch (curChar)
        {
            case U' ':
                characterPos.x += whitespaceWidth;
                continue;
            case U'\t':
                characterPos.x += whitespaceWidth * 4;
                continue;
            case U'\n':
                characterPos.y += lineSpacing;
                characterPos.x = 0;
                continue;
        }

        characterPos.x += fontSource.getGlyph(curChar, charSize, isBold, 0.f).advance + letterSpacing;
    }

    return self.getTransform().transformPoint(characterPos);
}


////////////////////////////////////////////////////////////
template <typename Self>
const Rect2f& TextBase::getLocalBounds(this const Self& self)
{
    self.ensureGeometryUpdate();
    return self.m_bounds;
}


////////////////////////////////////////////////////////////
template <typename Self>
Rect2f TextBase::getGlobalBounds(this const Self& self)
{
    return self.getTransform().transformRect(self.getLocalBounds());
}


////////////////////////////////////////////////////////////
template <typename Self>
void TextBase::draw(this const Self& self, RenderTarget& target, RenderStates states)
{
    states.transform *= self.getTransform();
    states.texture = &self.getTexture();

    self.ensureGeometryUpdate();

    target.drawQuads(
        za::DrawQuadsSettings{
            .vertexSpan    = self.m_vertices,
            .primitiveType = PrimitiveType::Triangles,
        },
        states);
}


////////////////////////////////////////////////////////////
template <typename Self>
ConstVertexSpan TextBase::getVertices(this const Self& self)
{
    self.ensureGeometryUpdate();
    return {self.m_vertices.data(), self.m_vertices.size()};
}


////////////////////////////////////////////////////////////
template <typename Self>
VertexSpan TextBase::getVerticesMut(this Self& self)
{
    self.ensureGeometryUpdate();
    return {self.m_vertices.data(), self.m_vertices.size()};
}


////////////////////////////////////////////////////////////
template <typename Self>
void TextBase::ensureGeometryUpdate(this const Self& self)
{
    if (!self.m_geometryNeedUpdate)
        return;

    self.m_geometryNeedUpdate = false;

    self.m_vertices.clear();
    self.m_fillVerticesStartIndex = 0u;
    self.m_bounds                 = {};

    if (self.m_string.empty())
        return;

    const auto& fontSource       = self.getFontSource();
    const auto  charSize         = self.getCharacterSize();
    const float outlineThickness = self.getOutlineThickness();

    const auto fillQuadCount = TextUtils::precomputeTextQuadCount(self.m_string, self.m_underlined, self.m_strikeThrough);
    const auto outlineQuadCount = outlineThickness == 0.f ? 0u : fillQuadCount;

    const za::SizeT outlineVertexCount = outlineQuadCount * 4u;
    const za::SizeT fillVertexCount    = fillQuadCount * 4u;

    self.m_vertices.resize(outlineVertexCount + fillVertexCount);
    self.m_fillVerticesStartIndex = outlineVertexCount;

    const TextUtils::TextLayoutInputs layoutInputs{
        .bold             = self.isBold(),
        .italic           = self.m_italic,
        .underlined       = self.m_underlined,
        .strikeThrough    = self.m_strikeThrough,
        .characterSize    = charSize,
        .letterSpacing    = self.m_letterSpacing,
        .lineSpacing      = self.m_lineSpacing,
        .outlineThickness = outlineThickness,
    };

    self.m_bounds = TextUtils::createTextGeometryAndGetBounds<
        true>(outlineVertexCount,
              fontSource,
              self.m_string,
              layoutInputs,
              [&self] [[gnu::always_inline,
                        gnu::flatten]] (za::SizeT & idx,
                                        const float lineLength,
                                        const float lineTop,
                                        const float offset,
                                        const float thickness,
                                        const float outlineT,
                                        const bool  isOutline)
    {
        return TextUtils::addLine(self.m_vertices.data(),
                                  idx,
                                  lineLength,
                                  lineTop,
                                  isOutline ? self.m_outlineColor : self.m_fillColor,
                                  offset,
                                  thickness,
                                  outlineT);
    },
              [&self] [[gnu::always_inline,
                        gnu::flatten]] (za::SizeT & idx, const Vec2f pos, const Glyph& glyph, const float shear, const bool isOutline)
    {
        return TextUtils::addGlyphQuad(self.m_vertices.data(),
                                       idx,
                                       pos,
                                       isOutline ? self.m_outlineColor : self.m_fillColor,
                                       glyph,
                                       shear);
    });
}

} // namespace za
