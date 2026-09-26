#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/ArrowShapeData.hpp"
#include "Zancle/Graphics/BatchedGeometry.hpp"
#include "Zancle/Graphics/ChevronShapeData.hpp"
#include "Zancle/Graphics/CircleShapeData.hpp"
#include "Zancle/Graphics/CogShapeData.hpp"
#include "Zancle/Graphics/CrossShapeData.hpp"
#include "Zancle/Graphics/CurvedArrowShapeData.hpp"
#include "Zancle/Graphics/DrawIndexedVerticesSettings.hpp"
#include "Zancle/Graphics/DrawVerticesSettings.hpp"
#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/DrawableBatchUtils.hpp"
#include "Zancle/Graphics/EllipseShapeData.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/FontFace.hpp"
#include "Zancle/Graphics/GlyphMappedText.hpp"
#include "Zancle/Graphics/GlyphMappedTextData.hpp"
#include "Zancle/Graphics/GlyphMapping.hpp"
#include "Zancle/Graphics/HeartShapeData.hpp"
#include "Zancle/Graphics/IndexType.hpp"
#include "Zancle/Graphics/PieSliceShapeData.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/RectangleShapeData.hpp"
#include "Zancle/Graphics/RingPieSliceShapeData.hpp"
#include "Zancle/Graphics/RingShapeData.hpp"
#include "Zancle/Graphics/RoundedRectangleShapeData.hpp"
#include "Zancle/Graphics/Shape.hpp"
#include "Zancle/Graphics/ShapeUtils.hpp"
#include "Zancle/Graphics/Sprite.hpp"
#include "Zancle/Graphics/StarShapeData.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/TextUtils.hpp"
#include "Zancle/Graphics/Transform.hpp"
#include "Zancle/Graphics/TrapezoidShapeData.hpp"
#include "Zancle/Graphics/Vertex.hpp"

#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/Vec2.hpp"

#include "Zancle/Math/Ceil.hpp"
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/FloatEpsilon.hpp"
#include "Zancle/Math/MinMax.hpp"
#include "Zancle/Math/MinMaxMacros.hpp"
#include "Zancle/Math/Remainder.hpp"
#include "Zancle/Math/SinCosLookup.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/AssertAndAssume.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace
{
////////////////////////////////////////////////////////////
// Finalize outline vertices that were generated in LOCAL space: map
// `outlineTextureRect` over the outline's own local bounds (mirroring
// `Shape::updateOutlineTexCoords`, which maps over the bounds of the outline
// vertex range), then transform positions into world space and apply the
// outline color. Generating outlines in local space keeps the thickness and
// miters in local units (they scale with the shape's transform) and makes
// mirrored transforms unable to flip the side the outline grows on --
// matching the retained `za::Shape` pipeline exactly.
[[gnu::always_inline]] inline void finalizeLocalOutlineVertices(
    const auto&          descriptor,
    const za::Transform& transform,
    za::Vertex* const    outlineVertexPtr,
    const za::SizeT      outlineVertexCount)
{
    ZA_ASSERT(outlineVertexCount > 0u);

    const za::Vertex* const end = outlineVertexPtr + outlineVertexCount;

    // Outline-local bounds scan (retained parity: `Shape` maps outline UVs
    // over the outline vertex range's own bounds, not the fill's)
    za::Vec2f boundsMin = outlineVertexPtr[0].position;
    za::Vec2f boundsMax = outlineVertexPtr[0].position;

    for (const za::Vertex* vertex = outlineVertexPtr + 1; vertex != end; ++vertex)
    {
        boundsMin.x = ZA_MIN(boundsMin.x, vertex->position.x);
        boundsMin.y = ZA_MIN(boundsMin.y, vertex->position.y);
        boundsMax.x = ZA_MAX(boundsMax.x, vertex->position.x);
        boundsMax.y = ZA_MAX(boundsMax.y, vertex->position.y);
    }

    const za::Vec2f boundsSize       = boundsMax - boundsMin;
    const bool      degenerateBounds = boundsSize.x <= 0.f || boundsSize.y <= 0.f;

    for (za::Vertex* vertex = outlineVertexPtr; vertex != end; ++vertex)
    {
        const za::Vec2f localPoint = vertex->position;

        vertex->texCoords = degenerateBounds ? descriptor.outlineTextureRect.position
                                             : descriptor.outlineTextureRect.position +
                                                   descriptor.outlineTextureRect.size.componentWiseMul(
                                                       (localPoint - boundsMin).componentWiseDiv(boundsSize));

        vertex->position = transform.transformPoint(localPoint);
        vertex->color    = descriptor.outlineColor;
    }
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline void generateRingVertices(
    const za::Rect2f& textureRect,
    const za::Color   fillColor,
    const float       outerRadius,
    const float       innerRadius,
    auto&&            fTransform,
    const za::SizeT   numArcPoints,
    const float       startRadians,
    const float       angleStep,
    const za::Vec2f   invLocalBoundsSize,
    za::Vertex* const fillVertexPtr)
{
    for (unsigned int i = 0u; i < numArcPoints; ++i)
    {
        const auto [outerPoint,
                    innerPoint] = za::ShapeUtils::computeRingPointsFromAngleStep(i, startRadians, angleStep, outerRadius, innerRadius);

        // Outer vertex of the pair
        const za::Vec2f ratioO   = outerPoint.componentWiseMul(invLocalBoundsSize);
        fillVertexPtr[2 * i + 0] = {
            .position  = fTransform(outerPoint),
            .color     = fillColor,
            .texCoords = textureRect.position + textureRect.size.componentWiseMul(ratioO),
        };

        // Inner vertex of the pair
        const za::Vec2f ratioI   = innerPoint.componentWiseMul(invLocalBoundsSize);
        fillVertexPtr[2 * i + 1] = {
            .position  = fTransform(innerPoint),
            .color     = fillColor,
            .texCoords = textureRect.position + textureRect.size.componentWiseMul(ratioI),
        };
    }
}


////////////////////////////////////////////////////////////
// Adapter combining GlyphMapping (glyph/metrics) + FontFace (kerning) into a single font source
struct GlyphMappingWithKerning
{
    const za::GlyphMapping& mapping;
    const za::FontFace&     fontFace;

    [[nodiscard]] const za::Glyph& getGlyph(char32_t cp, unsigned int cs, bool b, float ot) const
    {
        return mapping.getGlyph(cp, cs, b, ot);
    }

    [[nodiscard]] za::GlyphMapping::GlyphPair getFillAndOutlineGlyph(char32_t cp, unsigned int cs, bool b, float ot) const
    {
        return mapping.getFillAndOutlineGlyph(cp, cs, b, ot);
    }

    [[nodiscard]] float getKerning(char32_t first, char32_t second, unsigned int cs, bool b) const
    {
        return fontFace.getKerning(first, second, cs, b);
    }

    [[nodiscard]] float getLineSpacing(unsigned int cs) const
    {
        return mapping.getLineSpacing(cs);
    }

    [[nodiscard]] float getAscent(unsigned int cs) const
    {
        return mapping.getAscent(cs);
    }

    [[nodiscard]] float getDescent(unsigned int cs) const
    {
        return mapping.getDescent(cs);
    }

    [[nodiscard]] float getUnderlinePosition(unsigned int cs) const
    {
        return mapping.getUnderlinePosition(cs);
    }

    [[nodiscard]] float getUnderlineThickness(unsigned int cs) const
    {
        return mapping.getUnderlineThickness(cs);
    }
};

} // namespace


namespace za::priv
{
////////////////////////////////////////////////////////////
template <typename TStorage>
void DrawableBatchImpl<TStorage>::add(const DrawVerticesSettings& settings)
{
    const auto& vertexSpan = settings.vertexSpan;
    const auto  type       = settings.primitiveType;

    if (vertexSpan.isNullOrEmpty())
        return;

    const auto* vertexData  = vertexSpan.data();
    const auto  vertexCount = vertexSpan.size();

    IndexType numTrianglesInStripOrFan = 0u; // Only used for triangle strips and triangle fans
    za::SizeT numIndicesToGenerate     = 0u;

    switch (type)
    {
        case PrimitiveType::Triangles:
        {
            ZA_ASSERT(vertexCount % 3u == 0u);
            numIndicesToGenerate = vertexCount;
            break;
        }

        case PrimitiveType::TriangleStrip:
        case PrimitiveType::TriangleFan:
        {
            ZA_ASSERT(vertexCount >= 3u);

            if (vertexCount < 3u) // Release guard: `vertexCount - 2u` underflows below
                return;

            numTrianglesInStripOrFan = static_cast<IndexType>(vertexCount - 2u);
            numIndicesToGenerate     = static_cast<za::SizeT>(numTrianglesInStripOrFan) * 3u;
            break;
        }

        default:
            ZA_ASSERT(false && "Unsupported primitive type");
            return;
    }

    if (numIndicesToGenerate == 0u)
        return;

    const IndexType firstNewVertexIndex = m_storage.getNumVertices();

    ZA_MEMCPY(m_storage.reserveMoreVertices(vertexCount), vertexData, vertexCount * sizeof(Vertex));
    m_storage.commitMoreVertices(vertexCount);

    IndexType* dstIndices = m_storage.reserveMoreIndices(numIndicesToGenerate);

    if (type == PrimitiveType::Triangles)
    {
        ZA_ASSERT(numIndicesToGenerate == vertexCount);

        for (za::SizeT i = 0u; i < numIndicesToGenerate; ++i)
            *dstIndices++ = firstNewVertexIndex + static_cast<IndexType>(i);
    }
    else if (type == PrimitiveType::TriangleStrip)
    {
        for (IndexType i = 0u; i < numTrianglesInStripOrFan; ++i)
            DrawableBatchUtils::appendTriangleStripIndices(dstIndices, firstNewVertexIndex, i);
    }
    else
    {
        ZA_ASSERT(type == PrimitiveType::TriangleFan);

        for (IndexType i = 0u; i < numTrianglesInStripOrFan; ++i)
            DrawableBatchUtils::appendTriangleFanIndices(dstIndices,
                                                         firstNewVertexIndex,
                                                         /* second vertex relative index */ i + 1u);
    }

    m_storage.commitMoreIndices(numIndicesToGenerate);
}


////////////////////////////////////////////////////////////
template <typename TStorage>
void DrawableBatchImpl<TStorage>::add(const DrawIndexedVerticesSettings& settings)
{
    const auto& vertexSpan = settings.vertexSpan;
    const auto& indexSpan  = settings.indexSpan;
    const auto  type       = settings.primitiveType;

    if (vertexSpan.isNullOrEmpty() || indexSpan.isNullOrEmpty())
        return;

    const auto* vertexData  = vertexSpan.data();
    const auto  vertexCount = vertexSpan.size();

    const auto* indexData  = indexSpan.data();
    const auto  indexCount = indexSpan.size();

    IndexType numTrianglesInStripOrFan = 0u; // Only used for triangle strips and triangle fans
    za::SizeT numIndicesToGenerate     = 0u;

    // Type-specific assertions. The core logic of copying indices is type-agnostic.
    switch (type)
    {
        case PrimitiveType::Triangles:
        {
            ZA_ASSERT(indexCount % 3u == 0u);
            numIndicesToGenerate = indexCount;
            break;
        }

        case PrimitiveType::TriangleStrip:
        case PrimitiveType::TriangleFan:
        {
            ZA_ASSERT(indexCount == 0u || indexCount >= 3u);

            if (indexCount < 3u) // Release guard: `indexCount - 2u` underflows below
                return;

            numTrianglesInStripOrFan = static_cast<IndexType>(indexCount - 2u);
            numIndicesToGenerate     = static_cast<za::SizeT>(numTrianglesInStripOrFan) * 3u;
            break;
        }

        default:
            ZA_ASSERT(false && "Unsupported primitive type");
            return;
    }

    if (numIndicesToGenerate == 0u)
        return;

#ifdef ZA_DEBUG
    for (za::SizeT i = 0u; i < indexCount; ++i)
        ZA_ASSERT(indexData[i] < static_cast<IndexType>(vertexCount));
#endif

    const IndexType firstNewVertexIndex = m_storage.getNumVertices();

    ZA_MEMCPY(m_storage.reserveMoreVertices(vertexCount), vertexData, vertexCount * sizeof(Vertex));
    m_storage.commitMoreVertices(vertexCount);

    IndexType* dstIndices = m_storage.reserveMoreIndices(numIndicesToGenerate);

    if (type == PrimitiveType::Triangles)
    {
        for (za::SizeT i = 0u; i < indexCount; ++i)
            *dstIndices++ = firstNewVertexIndex + indexData[i];
    }
    else if (type == PrimitiveType::TriangleStrip)
    {
        for (IndexType k = 0u; k < numTrianglesInStripOrFan; ++k) // `k` is the triangle index within the strip
        {
            // Get the global indices for the `k-th`, `(k+1)-th`, and (`k+2)-th` vertices of the strip
            const IndexType vGlobalK0 = firstNewVertexIndex + indexData[k + 0u];
            const IndexType vGlobalK1 = firstNewVertexIndex + indexData[k + 1u];
            const IndexType vGlobalK2 = firstNewVertexIndex + indexData[k + 2u];

            if ((k % 2u) == 0u) // Even triangle: `(V_k, V_{k+1}, V_{k+2})`
            {
                *dstIndices++ = vGlobalK0;
                *dstIndices++ = vGlobalK1;
                *dstIndices++ = vGlobalK2;
            }
            else // Odd triangle: `(V_{k+2}, V_{k+1}, V_k)` to maintain winding, consistent with `appendTriangleStripIndices`
            {
                *dstIndices++ = vGlobalK2;
                *dstIndices++ = vGlobalK1;
                *dstIndices++ = vGlobalK0;
            }
        }
    }
    else
    {
        ZA_ASSERT(type == PrimitiveType::TriangleFan);

        const IndexType vCenterGlobal = firstNewVertexIndex + indexData[0];

        for (IndexType k = 0u; k < numTrianglesInStripOrFan; ++k) // `k` is the triangle index within the fan
        {
            // Triangles are `(Center, V_{k+1}, V_{k+2})`
            const IndexType vGlobalK1 = firstNewVertexIndex + indexData[k + 1u];
            const IndexType vGlobalK2 = firstNewVertexIndex + indexData[k + 2u];

            *dstIndices++ = vCenterGlobal;
            *dstIndices++ = vGlobalK1;
            *dstIndices++ = vGlobalK2;
        }
    }

    m_storage.commitMoreIndices(numIndicesToGenerate);
}


////////////////////////////////////////////////////////////
template <typename TStorage>
void DrawableBatchImpl<TStorage>::addTextImpl(const auto& text)
{
    const auto [data, size] = text.getVertices();
    ZA_ASSERT(size % 4u == 0);

    const auto numQuads = static_cast<IndexType>(size / 4u);

    DrawableBatchUtils::appendTextIndicesAndVertices(text.getTransform(),
                                                     data,
                                                     numQuads,
                                                     m_storage.getNumVertices(),
                                                     m_storage.reserveMoreIndices(6u * numQuads),
                                                     m_storage.reserveMoreVertices(4u * numQuads));

    m_storage.commitMoreIndices(6u * numQuads);
    m_storage.commitMoreVertices(4u * numQuads);
}


////////////////////////////////////////////////////////////
template <typename TStorage>
void DrawableBatchImpl<TStorage>::add(const Text& text)
{
    addTextImpl(text);
}


////////////////////////////////////////////////////////////
template <typename TStorage>
void DrawableBatchImpl<TStorage>::add(const GlyphMappedText& text)
{
    addTextImpl(text);
}


////////////////////////////////////////////////////////////
template <typename TStorage>
void DrawableBatchImpl<TStorage>::add(const Sprite& sprite)
{
    DrawableBatchUtils::appendSpriteIndicesAndVertices(sprite,
                                                       m_storage.getNumVertices(),
                                                       m_storage.reserveMoreIndices(6u),
                                                       m_storage.reserveMoreVertices(4u));

    m_storage.commitMoreIndices(6u);
    m_storage.commitMoreVertices(4u);
}


////////////////////////////////////////////////////////////
template <typename TStorage>
void DrawableBatchImpl<TStorage>::addShapeFill(const Transform& transform, const Vertex* data, const za::SizeT size)
{
    if (size < 3u) [[unlikely]]
        return;

    const za::SizeT indexCount = 3u * (size - 2u);

    DrawableBatchUtils::appendShapeFillIndicesAndVertices(transform,
                                                          data,
                                                          static_cast<IndexType>(size),
                                                          m_storage.getNumVertices(),
                                                          m_storage.reserveMoreIndices(indexCount),
                                                          m_storage.reserveMoreVertices(size));

    m_storage.commitMoreIndices(indexCount);
    m_storage.commitMoreVertices(size);
}


////////////////////////////////////////////////////////////
template <typename TStorage>
void DrawableBatchImpl<TStorage>::addShapeOutline(const Transform& transform, const Vertex* data, const za::SizeT size)
{
    if (size < 3u) [[unlikely]]
        return;

    const za::SizeT indexCount = 3u * (size - 2u);

    DrawableBatchUtils::appendShapeOutlineIndicesAndVertices(transform,
                                                             data,
                                                             static_cast<IndexType>(size),
                                                             m_storage.getNumVertices(),
                                                             m_storage.reserveMoreIndices(indexCount),
                                                             m_storage.reserveMoreVertices(size));

    m_storage.commitMoreIndices(indexCount);
    m_storage.commitMoreVertices(size);
}


////////////////////////////////////////////////////////////
template <typename TStorage>
void DrawableBatchImpl<TStorage>::add(const Shape& shape)
{
    const auto transform = shape.getTransform();

    const auto [fillData, fillSize]       = shape.getFillVertices();
    const auto [outlineData, outlineSize] = shape.getOutlineVertices();

    addShapeFill(transform, fillData, fillSize);
    addShapeOutline(transform, outlineData, outlineSize);
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::drawTriangleFanShapeFromPoints(
    const za::SizeT    nPoints,
    const auto&        descriptor,
    auto&&             pointFn,
    const Vec2f* const localApex)
{
    if (nPoints < 3u) [[unlikely]]
        return {};

    const auto transform = Transform::fromPositionScaleOriginRotation(descriptor.position,
                                                                      descriptor.scale,
                                                                      descriptor.origin,
                                                                      descriptor.rotation);

    // TODO P1: improve, also add to RenderTarget

    const za::SizeT fillVertexCount = nPoints + 2u;                    // +2 for center and repeated first point
    const IndexType firstFillVertexIndex = m_storage.getNumVertices(); // index of 1st fill vertex (center of the triangle fan)

    Vertex* const fillVertexPtr = m_storage.reserveMoreVertices(fillVertexCount);
    m_storage.commitMoreVertices(fillVertexCount);

    //
    // Update fill vertex positions and color, compute LOCAL-space inside bounds.
    // Texture coordinates must be derived from the untransformed (local) geometry so that
    // the texture rotates/scales together with the shape (matching `Shape::updateTexCoords`
    // and the batch ring path). Each vertex's local point is stashed in `texCoords` and
    // remapped into the texture rect by the pass below.
    const Vec2f firstLocalPoint = pointFn(0u);

    fillVertexPtr[1].position  = transform.transformPoint(firstLocalPoint); // first point
    fillVertexPtr[1].color     = descriptor.fillColor;
    fillVertexPtr[1].texCoords = firstLocalPoint;

    za::Vec2f fillBoundsPosition = firstLocalPoint; // left and top

    float fillBoundsMaxX = firstLocalPoint.x; // right
    float fillBoundsMaxY = firstLocalPoint.y; // bottom

    for (unsigned int i = 1u; i < nPoints; ++i)
    {
        Vertex& v = fillVertexPtr[1u + i];

        const Vec2f localPoint = pointFn(i);

        v.position  = transform.transformPoint(localPoint);
        v.color     = descriptor.fillColor;
        v.texCoords = localPoint;

        fillBoundsPosition.x = ZA_MIN(fillBoundsPosition.x, localPoint.x);
        fillBoundsPosition.y = ZA_MIN(fillBoundsPosition.y, localPoint.y);

        fillBoundsMaxX = ZA_MAX(fillBoundsMaxX, localPoint.x);
        fillBoundsMaxY = ZA_MAX(fillBoundsMaxY, localPoint.y);
    }

    const za::Vec2f fillBoundsSize{fillBoundsMaxX - fillBoundsPosition.x, fillBoundsMaxY - fillBoundsPosition.y};

    // Fan apex: if the caller supplied a local apex (needed for non-convex shapes whose bbox
    // center may lie outside the polygon), use it; otherwise fall back to the local bbox
    // center (valid for convex/centrally-symmetric shapes). Transformed alongside the geometry.
    const Vec2f localApexPoint = (localApex != nullptr) ? *localApex : fillBoundsPosition + fillBoundsSize / 2.f;

    fillVertexPtr[0].position  = transform.transformPoint(localApexPoint);
    fillVertexPtr[0].color     = descriptor.fillColor;
    fillVertexPtr[0].texCoords = localApexPoint;

    fillVertexPtr[1u + nPoints].position  = fillVertexPtr[1].position; // repeated first point
    fillVertexPtr[1u + nPoints].color     = descriptor.fillColor;
    fillVertexPtr[1u + nPoints].texCoords = firstLocalPoint;

    //
    // Maps the stashed local-space points into the fill texture rect. Deferred
    // into a lambda: when an outline is generated, the outline generation reads
    // the stashed local points first, so the stash must stay alive until then.
    const auto mapFillTexCoords =
        [&descriptor, fillBoundsPosition, fillBoundsSize](Vertex* const baseVertexPtr, const za::SizeT count)
    {
        if (fillBoundsSize.x <= 0.f || fillBoundsSize.y <= 0.f) [[unlikely]]
            return; // Degenerate fill: keep the deterministic stashed values

        const Vertex* end = baseVertexPtr + count;
        for (Vertex* vertex = baseVertexPtr; vertex != end; ++vertex)
        {
            const Vec2f ratio = (vertex->texCoords - fillBoundsPosition).componentWiseDiv(fillBoundsSize);
            vertex->texCoords = descriptor.textureRect.position + descriptor.textureRect.size.componentWiseMul(ratio);
        }
    };

    const za::SizeT fillIndexCount = 3u * (fillVertexCount - 2u);

    IndexType* indexPtr = m_storage.reserveMoreIndices(fillIndexCount);

    for (IndexType i = 1u; i < fillVertexCount - 1u; ++i)
        DrawableBatchUtils::appendTriangleFanIndices(indexPtr, firstFillVertexIndex, i);

    m_storage.commitMoreIndices(fillIndexCount);

    //
    // Update outline if needed
    if (descriptor.outlineThickness == 0.f)
    {
        mapFillTexCoords(fillVertexPtr, fillVertexCount);
        return {.fill = {fillVertexPtr, fillVertexCount}, .outline = {}};
    }

    const za::SizeT outlineVertexCount = (nPoints + 1u) * 2u;

    const IndexType firstOutlineVertexIndex = m_storage.getNumVertices();

    Vertex* const outlineVertexPtr = m_storage.reserveMoreVertices(outlineVertexCount);
    m_storage.commitMoreVertices(outlineVertexCount);

    // Cannot use `fillVertexPtr` here as the outline reserve may have invalidated the pointer
    Vertex* const fillBasePtr = outlineVertexPtr - fillVertexCount;

    // Generate the outline from the LOCAL-space fill perimeter (still stashed in
    // the fill vertices' `texCoords`), writing LOCAL outline positions;
    // `finalizeLocalOutlineVertices` maps UVs and transforms to world space below
    ShapeUtils::updateOutlineImpl(descriptor.outlineThickness,
                                  [fillPerimeterPtr = fillBasePtr + 1u] // Skip the first vertex (center point)
                                  [[gnu::always_inline, gnu::flatten]] (const za::SizeT i)
    { return fillPerimeterPtr[i].texCoords; },
                                  outlineVertexPtr,
                                  nPoints,
                                  descriptor.miterLimit);

    // The outline generation has consumed the stash: map the fill tex coords now
    mapFillTexCoords(fillBasePtr, fillVertexCount);

    //
    // Update outline colors, outline tex coords, and world-space positions
    finalizeLocalOutlineVertices(descriptor, transform, outlineVertexPtr, outlineVertexCount);

    const za::SizeT outlineIndexCount = 3u * (outlineVertexCount - 2u);

    auto* outlineIndexPtr = m_storage.reserveMoreIndices(outlineIndexCount);

    // Outline vertices are strip-laid (inner/outer pairs); use strip indices
    // so the winding stays consistent across all triangles -- matches the
    // immediate `PrimitiveType::TriangleStrip` rendering path.
    for (IndexType i = 0u; i < outlineVertexCount - 2; ++i)
        DrawableBatchUtils::appendTriangleStripIndices(outlineIndexPtr, firstOutlineVertexIndex, i);

    m_storage.commitMoreIndices(outlineIndexCount);

    return {
        .fill    = {outlineVertexPtr - fillVertexCount, fillVertexCount},
        .outline = {outlineVertexPtr, outlineVertexCount},
    };
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const ArrowShapeData& sdArrow)
{
    if (!sdArrow.hasVisibleGeometry()) [[unlikely]]
        return {};

    // Fan apex in LOCAL coords: on the arrow's axis of symmetry, inside whichever region
    // (head triangle or shaft rectangle) is wide enough to see every perimeter vertex.
    // The arrow is non-convex (barbs at v1/v5), so only one of the two regions is a valid
    // star-shaped kernel -- pick based on which of head/shaft is wider.
    const Vec2f localApex = sdArrow.headWidth > sdArrow.shaftWidth
                                ? Vec2f{sdArrow.shaftLength + sdArrow.headLength / 3.f, 0.f}
                                : Vec2f{sdArrow.shaftLength * 0.5f, 0.f};

    return drawTriangleFanShapeFromPoints(7u,
                                          sdArrow,
                                          [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i)
    {
        return ShapeUtils::computeArrowPoint(i, sdArrow.shaftLength, sdArrow.shaftWidth, sdArrow.headLength, sdArrow.headWidth);
    },
                                          &localApex);
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const CircleShapeData& sdCircle)
{
    if (!sdCircle.hasVisibleGeometry()) [[unlikely]]
        return {};

    const float angleStep = za::tau / static_cast<float>(sdCircle.pointCount);

    return drawTriangleFanShapeFromPoints(sdCircle.pointCount,
                                          sdCircle,
                                          [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i)
    {
        return ShapeUtils::computeCirclePointFromAngleStep(i, sdCircle.startAngle.asRadians(), angleStep, sdCircle.radius);
    });
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const CurvedArrowShapeData& sd)
{
    if (!sd.hasVisibleGeometry()) [[unlikely]]
        return {};

    const auto adjustedOrigin = sd.origin - Vec2f{sd.outerRadius, sd.outerRadius};

    const auto transform = Transform::fromPositionScaleOriginRotation(sd.position, sd.scale, adjustedOrigin, sd.rotation);

    const float sweepAngleRadians    = sd.sweepAngle.asRadians();
    const float absSweepAngleRadians = za::fabs(sweepAngleRadians);
    const float sweepSign            = (sweepAngleRadians < 0.f) ? -1.f : 1.f;

    // Adjust `pointCount` based on sweep angle for consistent smoothness
    const unsigned int numArcPoints = za::max(2u,
                                              static_cast<unsigned int>(ZA_MATH_CEILF(
                                                  static_cast<float>(sd.pointCount) * (absSweepAngleRadians / za::halfPi))));

    // Reserve memory upfront to avoid pointer invalidation
    const za::SizeT bodyFillVertexCount = 2u * numArcPoints;

    const za::SizeT outlinePerimeterPointCount = numArcPoints + 3u + numArcPoints;

    const za::SizeT outlineVertexCount = sd.outlineThickness != 0.f
                                             ? (outlinePerimeterPointCount + 1u) * 2u // For closed triangle strip
                                             : 0u;

    Vertex* const reservedVertexPtr = m_storage.reserveMoreVertices(bodyFillVertexCount + 3u + outlineVertexCount);

    // Body fill vertices and indices
    const IndexType firstBodyFillVertexIndex = m_storage.getNumVertices();
    Vertex* const   bodyFillVertexPtr        = reservedVertexPtr;

    // Using a bounding box around the center of the curve for simplicity in texcoords for the body part
    // This is consistent with how `generateRingVertices` might calculate them for a full ring
    const Vec2f localBoundsSize    = {2.f * sd.outerRadius, 2.f * sd.outerRadius};
    const Vec2f invLocalBoundsSize = {1.f / localBoundsSize.x, 1.f / localBoundsSize.y};

    const float angleStep    = sweepAngleRadians / static_cast<float>(numArcPoints - 1u);
    const float startRadians = sd.startAngle.asRadians();

    // Shift the ring's natural center `(outerRadius, outerRadius)` to the origin, then apply the
    // user-facing transform. Composing once lets `generateRingVertices` and the head-fill writes
    // emit world-space positions directly, avoiding a second pass over body fill vertices.
    const auto correctionTransform = Transform::fromPosition({-sd.outerRadius, -sd.outerRadius});
    const auto fullTransform       = transform * correctionTransform;

    generateRingVertices(sd.textureRect,
                         sd.fillColor,
                         sd.outerRadius,
                         sd.innerRadius,
                         [&] [[gnu::always_inline, gnu::flatten]] (const Vec2f p)
    { return fullTransform.transformPoint(p); },
                         numArcPoints,
                         startRadians,
                         angleStep,
                         invLocalBoundsSize,
                         bodyFillVertexPtr);

    m_storage.commitMoreVertices(bodyFillVertexCount);

    const za::SizeT numBodySegments    = numArcPoints - 1u;
    const za::SizeT bodyFillIndexCount = numBodySegments * 6u; // 2 triangles per segment, 3 indices per triangle

    IndexType* bodyFillIndexPtr = m_storage.reserveMoreIndices(bodyFillIndexCount);
    for (IndexType i = 0u; i < numBodySegments; ++i)
    {
        const IndexType idxOuterCurr = firstBodyFillVertexIndex + 2u * i + 0u;
        const IndexType idxInnerCurr = firstBodyFillVertexIndex + 2u * i + 1u;
        const IndexType idxOuterNext = firstBodyFillVertexIndex + 2u * (i + 1u) + 0u;
        const IndexType idxInnerNext = firstBodyFillVertexIndex + 2u * (i + 1u) + 1u;

        // Triangle 1 of segment
        *bodyFillIndexPtr++ = idxOuterCurr;
        *bodyFillIndexPtr++ = idxInnerCurr;
        *bodyFillIndexPtr++ = idxOuterNext;

        // Triangle 2 of segment
        *bodyFillIndexPtr++ = idxInnerCurr;
        *bodyFillIndexPtr++ = idxInnerNext;
        *bodyFillIndexPtr++ = idxOuterNext;
    }

    m_storage.commitMoreIndices(bodyFillIndexCount);

    // Head fill vertices and indices
    const IndexType firstHeadFillVertexIndex = m_storage.getNumVertices();
    Vertex* const   headFillVertexPtr        = reservedVertexPtr + bodyFillVertexCount; // After body

    const float endAngleRad = za::positiveRemainder(startRadians + static_cast<float>(numArcPoints - 1u) * angleStep, za::tau);
    const auto [endAngleRadSin, endAngleRadCos] = za::sinCosLookup(endAngleRad);

    // Centerline point at the end of the body's curve (local, untransformed)
    // This uses the ringLocalCenter from `computeRingPointsFromAngleStep` as the reference.
    // Center used by `computeRingPointsFromAngleStep` for its calculations
    const float pathRadius = sd.innerRadius + (sd.outerRadius - sd.innerRadius) * 0.5f;

    const Vec2f ringLocalCenter = {sd.outerRadius, sd.outerRadius};
    const Vec2f headAttachPointLocal = ringLocalCenter + Vec2f{pathRadius * endAngleRadCos, pathRadius * endAngleRadSin};

    // Tangent direction at the end of the curve (unit vector)
    // For angle 'a', tangent is (-sin a, cos a) for CCW.
    // sweepSign ensures tip points 'forward' along directed curve.
    const Vec2f tangentDir = {-endAngleRadSin, endAngleRadCos};

    // Radial direction outwards from the curve's center of curvature at endAngleRad (unit vector)
    const Vec2f radialOutDir = {endAngleRadCos, endAngleRadSin};

    // Assign head vertices with transformation
    headFillVertexPtr[0] = {
        // Tip
        .position  = fullTransform.transformPoint(headAttachPointLocal + tangentDir * (sd.headLength * sweepSign)),
        .color     = sd.fillColor,
        .texCoords = sd.textureRect.position + sd.textureRect.size.componentWiseMul({0.5f, 1.f}) // Example: Mid-top
    };

    headFillVertexPtr[1] = {
        // Outer Barb
        .position  = fullTransform.transformPoint(headAttachPointLocal + radialOutDir * (sd.headWidth / 2.f)),
        .color     = sd.fillColor,
        .texCoords = sd.textureRect.position + sd.textureRect.size.componentWiseMul({0.f, 0.f}) // Example: Top-left
    };

    headFillVertexPtr[2] = {
        // Inner Barb
        .position  = fullTransform.transformPoint(headAttachPointLocal - radialOutDir * (sd.headWidth / 2.f)),
        .color     = sd.fillColor,
        .texCoords = sd.textureRect.position + sd.textureRect.size.componentWiseMul({1.f, 0.f}) // Example: Top-right
    };

    m_storage.commitMoreVertices(3u);

    // Indices for head (3 triangles connecting body end to head points)
    // These indices refer to already transformed vertices in the buffer
    const IndexType endBodyOuterIdx = firstBodyFillVertexIndex + 2u * (numArcPoints - 1u) + 0u;
    const IndexType endBodyInnerIdx = firstBodyFillVertexIndex + 2u * (numArcPoints - 1u) + 1u;

    const IndexType headTipIdx       = firstHeadFillVertexIndex + 0u;
    const IndexType headOuterBarbIdx = firstHeadFillVertexIndex + 1u;
    const IndexType headInnerBarbIdx = firstHeadFillVertexIndex + 2u;

    IndexType* headFillIndexPtr = m_storage.reserveMoreIndices(9u); // 3 triangles

    // Stitch end of body to base of arrowhead
    // Winding should be consistent (e.g., CCW)
    // Triangle 1: (Outer side of body end, Inner side of body end, Outer barb)
    *headFillIndexPtr++ = endBodyOuterIdx;
    *headFillIndexPtr++ = endBodyInnerIdx;
    *headFillIndexPtr++ = headOuterBarbIdx;

    // Triangle 2: (Inner side of body end, Inner barb, Outer barb)
    *headFillIndexPtr++ = endBodyInnerIdx;
    *headFillIndexPtr++ = headInnerBarbIdx;
    *headFillIndexPtr++ = headOuterBarbIdx;

    // Triangle 3: (Outer barb, Inner barb, Tip)
    *headFillIndexPtr++ = headOuterBarbIdx;
    *headFillIndexPtr++ = headInnerBarbIdx;
    *headFillIndexPtr++ = headTipIdx;
    m_storage.commitMoreIndices(9u);

    // Outline generation
    if (sd.outlineThickness != 0.f)
    {
        Vertex* const outlineVertexPtr = reservedVertexPtr + bodyFillVertexCount + 3u; // After body and head fill vertices

        const IndexType firstOutlineVertexIndex = m_storage.getNumVertices();

        // Walk the perimeter CW-visually in screen space (y-down) so that outline normals point
        // inward into the curved-arrow body. This makes `outlineThickness > 0` draw on top of
        // the fill rather than growing the bounds.
        //
        // Order: outer arc (start -> end) -> head (outer barb, tip, inner barb) -> inner arc (end -> start).
        // Perimeter points in LOCAL (ring-local) space, matching the fill's
        // closed forms exactly; `finalizeLocalOutlineVertices` maps UVs and
        // applies `fullTransform` afterwards
        const Vec2f headLocalPoints[3] = {headAttachPointLocal + radialOutDir * (sd.headWidth / 2.f),      // outer barb
                                          headAttachPointLocal + tangentDir * (sd.headLength * sweepSign), // tip
                                          headAttachPointLocal - radialOutDir * (sd.headWidth / 2.f)};     // inner barb

        ShapeUtils::updateOutlineImpl(sd.outlineThickness,
                                      [&](const za::SizeT i)
        {
            // 1. Outer edge of the body (start to end).
            if (i < numArcPoints)
                return ShapeUtils::computeRingPointsFromAngleStep(i, startRadians, angleStep, sd.outerRadius, sd.innerRadius)
                    .outerPoint;

            // 2. Arrowhead vertices: outer barb -> tip -> inner barb.
            if (i < numArcPoints + 3u)
                return headLocalPoints[i - numArcPoints];

            // 3. Inner edge of the body (end back to start).
            const auto innerIdx = 2u * numArcPoints + 2u - static_cast<za::SizeT>(i);
            return ShapeUtils::computeRingPointsFromAngleStep(innerIdx, startRadians, angleStep, sd.outerRadius, sd.innerRadius)
                .innerPoint;
        },
                                      outlineVertexPtr,
                                      outlinePerimeterPointCount,
                                      sd.miterLimit);

        finalizeLocalOutlineVertices(sd, fullTransform, outlineVertexPtr, outlineVertexCount);
        m_storage.commitMoreVertices(outlineVertexCount);

        const za::SizeT outlineIndexCount = 3u * (outlineVertexCount - 2u); // Triangle strip indices
        IndexType*      outlineIndexPtr   = m_storage.reserveMoreIndices(outlineIndexCount);

        for (IndexType i = 0u; i < outlineVertexCount - 2u; ++i)
            DrawableBatchUtils::appendTriangleStripIndices(outlineIndexPtr, firstOutlineVertexIndex, i);

        m_storage.commitMoreIndices(outlineIndexCount);

        // Split body + head (fill) from the outline region
        return {
            .fill    = {bodyFillVertexPtr, bodyFillVertexCount + 3u},
            .outline = {outlineVertexPtr, outlineVertexCount},
        };
    }

    return {
        .fill    = {bodyFillVertexPtr, bodyFillVertexCount + 3u},
        .outline = {},
    };
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const EllipseShapeData& sdEllipse)
{
    if (!sdEllipse.hasVisibleGeometry()) [[unlikely]]
        return {};

    const float angleStep = za::tau / static_cast<float>(sdEllipse.pointCount);

    return drawTriangleFanShapeFromPoints(sdEllipse.pointCount,
                                          sdEllipse,
                                          [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i)
    {
        return ShapeUtils::computeEllipsePointFromAngleStep(i,
                                                            sdEllipse.startAngle.asRadians(),
                                                            angleStep,
                                                            sdEllipse.horizontalRadius,
                                                            sdEllipse.verticalRadius);
    });
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const PieSliceShapeData& sdPieSlice)
{
    if (!sdPieSlice.hasVisibleGeometry()) [[unlikely]]
        return {};

    if (ZA_MATH_FABSF(sdPieSlice.sweepAngle.asRadians()) >= za::tau - ZA_FLOAT_EPSILON)
        return add(CircleShapeData{
            .position           = sdPieSlice.position,
            .scale              = sdPieSlice.scale,
            .origin             = sdPieSlice.origin,
            .rotation           = sdPieSlice.rotation,
            .textureRect        = sdPieSlice.textureRect,
            .outlineTextureRect = sdPieSlice.outlineTextureRect,
            .fillColor          = sdPieSlice.fillColor,
            .outlineColor       = sdPieSlice.outlineColor,
            .outlineThickness   = sdPieSlice.outlineThickness,
            .miterLimit         = sdPieSlice.miterLimit,
            .radius             = sdPieSlice.radius,
            .startAngle         = sdPieSlice.startAngle,
            .pointCount         = za::max(3u, sdPieSlice.pointCount - 2u), // `CircleShapeData` requires >= 3
        });

    const float arcAngleStep = ShapeUtils::computePieSliceArcAngleStep(sdPieSlice.sweepAngle.asRadians(), sdPieSlice.pointCount);

    return drawTriangleFanShapeFromPoints(sdPieSlice.pointCount,
                                          sdPieSlice,
                                          [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i)
    {
        return ShapeUtils::computePieSlicePointFromArcAngleStep(i,
                                                                sdPieSlice.radius,
                                                                arcAngleStep,
                                                                sdPieSlice.startAngle.asRadians());
    });
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const RectangleShapeData& sdRectangle)
{
    if (!sdRectangle.hasVisibleGeometry()) [[unlikely]]
        return {};

    return drawTriangleFanShapeFromPoints(4u, sdRectangle, [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i) {
        return ShapeUtils::computeRectanglePoint(i, sdRectangle.size);
    });
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const RoundedRectangleShapeData& sdRoundedRectangle)
{
    if (!sdRoundedRectangle.hasVisibleGeometry()) [[unlikely]]
        return {};

    return drawTriangleFanShapeFromPoints(sdRoundedRectangle.cornerPointCount * 4u,
                                          sdRoundedRectangle,
                                          [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i)
    {
        return ShapeUtils::computeRoundedRectanglePoint(i,
                                                        sdRoundedRectangle.size,
                                                        sdRoundedRectangle.cornerRadius,
                                                        sdRoundedRectangle.cornerPointCount);
    });
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const RingShapeData& sdRing)
{
    if (!sdRing.hasVisibleGeometry()) [[unlikely]]
        return {};

    const auto
        transform = Transform::fromPositionScaleOriginRotation(sdRing.position, sdRing.scale, sdRing.origin, sdRing.rotation);

    const unsigned int nPoints   = sdRing.pointCount;
    const float        angleStep = za::tau / static_cast<float>(nPoints);

    //
    // Local origin `(0, 0)` is top-left of the bounding box
    // Bounding box size is `(2 * outerRadius, 2 * outerRadius)`
    // Geometric center within local coords is `(outerRadius, outerRadius)`
    const Vec2f localBoundsSize    = {2.f * sdRing.outerRadius, 2.f * sdRing.outerRadius};
    const Vec2f invLocalBoundsSize = {1.f / localBoundsSize.x, 1.f / localBoundsSize.y};

    //
    // Generate fill geometry (triangle strip)
    const za::SizeT fillVertexCount = 2u * nPoints + 2u; // `nPoints` pairs + repeated start pair

    const za::SizeT outlineVerticesPerLoop = nPoints * 2u + 2u; // nPoints original points -> nPoints inner/outer
                                                                // pairs + duplicated start pair for closed loop
    const za::SizeT totalOutlineVertices = sdRing.outlineThickness != 0.f ? outlineVerticesPerLoop * 2u
                                                                          : 0u; // Outer + Inner loop

    const IndexType firstFillVertexIndex = m_storage.getNumVertices();
    Vertex* const   reservedVertexPtr    = m_storage.reserveMoreVertices(fillVertexCount + totalOutlineVertices);
    Vertex* const   fillVertexPtr        = reservedVertexPtr;

    generateRingVertices(sdRing.textureRect,
                         sdRing.fillColor,
                         sdRing.outerRadius,
                         sdRing.innerRadius,
                         [&] [[gnu::always_inline, gnu::flatten]] (const Vec2f p) { return transform.transformPoint(p); },
                         nPoints,
                         sdRing.startAngle.asRadians(),
                         angleStep,
                         invLocalBoundsSize,
                         fillVertexPtr);

    //
    // Repeat first pair to close the strip
    fillVertexPtr[2 * nPoints + 0] = fillVertexPtr[0];
    fillVertexPtr[2 * nPoints + 1] = fillVertexPtr[1];

    m_storage.commitMoreVertices(fillVertexCount);

    //
    // Generate fill indices
    {
        const za::SizeT numFillTriangles = (fillVertexCount - 2u); // A strip of `V` vertices has `V - 2` triangles
        const za::SizeT fillIndexCount   = numFillTriangles * 3u;

        IndexType* fillIndexPtr = m_storage.reserveMoreIndices(fillIndexCount);
        for (IndexType i = 0u; i < numFillTriangles; ++i)
            DrawableBatchUtils::appendTriangleStripIndices(fillIndexPtr, firstFillVertexIndex, i);

        m_storage.commitMoreIndices(fillIndexCount);
    }

    //
    // Update outline if needed
    if (sdRing.outlineThickness == 0.f)
        return {.fill = {fillVertexPtr, fillVertexCount}, .outline = {}};

    const za::SizeT outlineIndicesPerLoop = 3u * (outlineVerticesPerLoop - 2u); // Indices for triangles from the strip
    const za::SizeT totalOutlineIndices   = outlineIndicesPerLoop * 2u;

    const IndexType  firstOutlineVertexIndex = m_storage.getNumVertices();
    Vertex* const    outlineVertexPtr        = reservedVertexPtr + fillVertexCount;
    IndexType* const outlineIndexPtr         = m_storage.reserveMoreIndices(totalOutlineIndices);

    const IndexType firstOuterOutlineLoopVertexIndex = firstOutlineVertexIndex;
    const IndexType firstInnerOutlineLoopVertexIndex = firstOutlineVertexIndex +
                                                       static_cast<IndexType>(outlineVerticesPerLoop);

    const auto generateOutlineHelper =
        [&](Vertex* const      chosenOutlineVertexPtr,
            IndexType* const   chosenOutlineIndexPtr,
            const IndexType    firstChonsenOutlineLoopVertexIndex,
            const unsigned int fillVertexIndexOffset,
            const bool         reverseWalk)
    {
        // Generate outline vertices.
        // The inner outline walks the inner ring in reverse so its normals point away from the
        // hole (into the annulus), matching the outer outline's inward normals. This keeps
        // `outlineThickness > 0` drawing on top of the fill for both loops.
        const auto getBoundaryPoint = [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i)
        {
            ZA_ASSERT_AND_ASSUME(i < nPoints);
            const za::SizeT walked = reverseWalk ? (nPoints - 1u - i) : i;

            // LOCAL-space boundary point (same closed form as the fill pass);
            // `finalizeLocalOutlineVertices` transforms to world space afterwards
            const auto ringPoints = ShapeUtils::computeRingPointsFromAngleStep(walked,
                                                                               sdRing.startAngle.asRadians(),
                                                                               angleStep,
                                                                               sdRing.outerRadius,
                                                                               sdRing.innerRadius);

            return fillVertexIndexOffset == 0u ? ringPoints.outerPoint : ringPoints.innerPoint;
        };

        Vertex* const chosenOutlineVertexStart = chosenOutlineVertexPtr;
        ShapeUtils::updateOutlineImpl(sdRing.outlineThickness,
                                      getBoundaryPoint,
                                      chosenOutlineVertexStart,
                                      nPoints,
                                      sdRing.miterLimit);

        // Generate outline indices. Outline vertices are strip-laid
        // (inner/outer pairs along the ring), so we emit strip-style
        // indices to keep winding consistent across the loop -- matches
        // the immediate `PrimitiveType::TriangleStrip` rendering path.
        IndexType* chosenOutlineIndexPtrStart = chosenOutlineIndexPtr;
        for (IndexType i = 0u; i < outlineVerticesPerLoop - 2u; ++i)
            DrawableBatchUtils::appendTriangleStripIndices(chosenOutlineIndexPtrStart, firstChonsenOutlineLoopVertexIndex, i);
    };

    //
    // Generate outer outline (walked forward: CW-visual, normals point inward into annulus)
    generateOutlineHelper(outlineVertexPtr, outlineIndexPtr, firstOuterOutlineLoopVertexIndex, 0, /* reverseWalk */ false);

    //
    // Generate inner outline (walked backward: CCW-visual, normals point outward from hole into annulus)
    generateOutlineHelper(outlineVertexPtr + outlineVerticesPerLoop,
                          outlineIndexPtr + outlineIndicesPerLoop,
                          firstInnerOutlineLoopVertexIndex,
                          1,
                          /* reverseWalk */ true);

    //
    // Update outline colors, outline tex coords, and world-space positions
    finalizeLocalOutlineVertices(sdRing, transform, outlineVertexPtr, totalOutlineVertices);

    m_storage.commitMoreVertices(totalOutlineVertices);
    m_storage.commitMoreIndices(totalOutlineIndices);

    return {
        .fill    = {outlineVertexPtr - fillVertexCount, fillVertexCount},
        .outline = {outlineVertexPtr, totalOutlineVertices},
    };
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const RingPieSliceShapeData& sdRingPieSlice)
{
    if (!sdRingPieSlice.hasVisibleGeometry()) [[unlikely]]
        return {};

    if (ZA_MATH_FABSF(sdRingPieSlice.sweepAngle.asRadians()) >= za::tau - ZA_FLOAT_EPSILON)
        return add(RingShapeData{
            .position           = sdRingPieSlice.position,
            .scale              = sdRingPieSlice.scale,
            .origin             = sdRingPieSlice.origin,
            .rotation           = sdRingPieSlice.rotation,
            .textureRect        = sdRingPieSlice.textureRect,
            .outlineTextureRect = sdRingPieSlice.outlineTextureRect,
            .fillColor          = sdRingPieSlice.fillColor,
            .outlineColor       = sdRingPieSlice.outlineColor,
            .outlineThickness   = sdRingPieSlice.outlineThickness,
            .miterLimit         = sdRingPieSlice.miterLimit,
            .outerRadius        = sdRingPieSlice.outerRadius,
            .innerRadius        = sdRingPieSlice.innerRadius,
            .startAngle         = sdRingPieSlice.startAngle,
            .pointCount         = za::max(3u, sdRingPieSlice.pointCount - 1u), // `RingShapeData` requires >= 3
        });

    const auto transform = Transform::fromPositionScaleOriginRotation(sdRingPieSlice.position,
                                                                      sdRingPieSlice.scale,
                                                                      sdRingPieSlice.origin,
                                                                      sdRingPieSlice.rotation);

    const float absSweepAngle = ZA_MATH_FABSF(sdRingPieSlice.sweepAngle.asRadians());
    const float sweepRadians  = sdRingPieSlice.sweepAngle.asRadians();
    const float startRadians  = sdRingPieSlice.startAngle.asRadians();

    const unsigned int numArcPoints = za::max(3u,
                                              static_cast<unsigned int>(ZA_MATH_CEILF(
                                                  static_cast<float>(sdRingPieSlice.pointCount) * (absSweepAngle / za::tau))));

    const float angleStep = sweepRadians / static_cast<float>(numArcPoints - 1u);

    //
    // Local origin `(0, 0)` is top-left of the bounding box
    // Bounding box size is `(2 * outerRadius, 2 * outerRadius)`
    // Geometric center within local coords is `(outerRadius, outerRadius)`
    const Vec2f localBoundsSize    = {2.f * sdRingPieSlice.outerRadius, 2.f * sdRingPieSlice.outerRadius};
    const Vec2f invLocalBoundsSize = {1.f / localBoundsSize.x, 1.f / localBoundsSize.y};

    //
    // Generate fill geometry (triangle strip)
    const za::SizeT fillVertexCount = 2u * numArcPoints;

    const za::SizeT numBoundaryPoints    = 2u * numArcPoints;
    const za::SizeT totalOutlineVertices = sdRingPieSlice.outlineThickness != 0.f ? (numBoundaryPoints + 1u) * 2u : 0u;

    const IndexType firstFillVertexIndex = m_storage.getNumVertices();
    Vertex* const   reservedVertexPtr    = m_storage.reserveMoreVertices(fillVertexCount + totalOutlineVertices);
    Vertex* const   fillVertexPtr        = reservedVertexPtr;

    generateRingVertices(sdRingPieSlice.textureRect,
                         sdRingPieSlice.fillColor,
                         sdRingPieSlice.outerRadius,
                         sdRingPieSlice.innerRadius,
                         [&] [[gnu::always_inline, gnu::flatten]] (const Vec2f p) { return transform.transformPoint(p); },
                         numArcPoints,
                         startRadians,
                         angleStep,
                         invLocalBoundsSize,
                         fillVertexPtr);

    m_storage.commitMoreVertices(fillVertexCount);

    //
    // Generate fill indices (`numArcPoints >= 3u` is guaranteed by the `za::max` above)
    {
        const za::SizeT numFillTriangles = (fillVertexCount - 2u); // A strip of `V` vertices has `V - 2` triangles
        const za::SizeT fillIndexCount   = numFillTriangles * 3u;

        IndexType* fillIndexPtr = m_storage.reserveMoreIndices(fillIndexCount);
        for (IndexType i = 0u; i < numFillTriangles; ++i)
            DrawableBatchUtils::appendTriangleStripIndices(fillIndexPtr, firstFillVertexIndex, i);

        m_storage.commitMoreIndices(fillIndexCount);
    }

    //
    // Update outline if needed
    if (sdRingPieSlice.outlineThickness == 0.f)
        return {.fill = {fillVertexPtr, fillVertexCount}, .outline = {}};

    const za::SizeT numOutlineTriangles = totalOutlineVertices - 2u;
    const za::SizeT totalOutlineIndices = numOutlineTriangles * 3u;

    const IndexType firstOutlineVertexIndex = m_storage.getNumVertices();
    Vertex* const   outlineVertexPtr        = reservedVertexPtr + fillVertexCount;

    //
    // Generate outline vertices
    const auto getBoundaryPoint = [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i)
    {
        ZA_ASSERT_AND_ASSUME(i < numBoundaryPoints);

        // LOCAL-space boundary point (same closed form as the fill pass): outer
        // arc walked forward, then inner arc walked backward.
        // `finalizeLocalOutlineVertices` transforms to world space afterwards.
        const bool onOuterArc = i < numArcPoints;
        const auto arcIndex   = onOuterArc ? i : (2u * numArcPoints - 1u - i);

        const auto ringPoints = ShapeUtils::computeRingPointsFromAngleStep(arcIndex,
                                                                           startRadians,
                                                                           angleStep,
                                                                           sdRingPieSlice.outerRadius,
                                                                           sdRingPieSlice.innerRadius);

        return onOuterArc ? ringPoints.outerPoint : ringPoints.innerPoint;
    };

    ShapeUtils::updateOutlineImpl(sdRingPieSlice.outlineThickness,
                                  getBoundaryPoint,  // Lambda providing positions
                                  outlineVertexPtr,  // Output vertex buffer
                                  numBoundaryPoints, // Number of unique points in the loop
                                  sdRingPieSlice.miterLimit);

    //
    // Generate outline indices
    IndexType* outlineIndexPtr = m_storage.reserveMoreIndices(totalOutlineIndices);
    for (IndexType i = 0u; i < numOutlineTriangles; ++i)
        DrawableBatchUtils::appendTriangleStripIndices(outlineIndexPtr, firstOutlineVertexIndex, i);

    //
    // Update outline colors, outline tex coords, and world-space positions
    finalizeLocalOutlineVertices(sdRingPieSlice, transform, outlineVertexPtr, totalOutlineVertices);

    m_storage.commitMoreVertices(totalOutlineVertices);
    m_storage.commitMoreIndices(totalOutlineIndices);

    return {
        .fill    = {outlineVertexPtr - fillVertexCount, fillVertexCount},
        .outline = {outlineVertexPtr, totalOutlineVertices},
    };
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const StarShapeData& sdStar)
{
    if (!sdStar.hasVisibleGeometry()) [[unlikely]]
        return {};

    const auto nPoints = sdStar.pointCount * 2u;

    ZA_ASSERT(nPoints != 0u);
    const float angleStep = za::tau / static_cast<float>(nPoints);

    return drawTriangleFanShapeFromPoints(nPoints, sdStar, [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i) {
        return ShapeUtils::computeStarPointFromAngleStep(i, angleStep, sdStar.outerRadius, sdStar.innerRadius);
    });
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const CrossShapeData& sdCross)
{
    if (!sdCross.hasVisibleGeometry()) [[unlikely]]
        return {};

    return drawTriangleFanShapeFromPoints(12u, sdCross, [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i) {
        return ShapeUtils::computeCrossPoint(i, sdCross.size, sdCross.armThickness);
    });
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const TrapezoidShapeData& sdTrapezoid)
{
    if (!sdTrapezoid.hasVisibleGeometry()) [[unlikely]]
        return {};

    return drawTriangleFanShapeFromPoints(4u, sdTrapezoid, [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i) {
        return ShapeUtils::computeTrapezoidPoint(i, sdTrapezoid.topWidth, sdTrapezoid.bottomWidth, sdTrapezoid.height);
    });
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const ChevronShapeData& sdChevron)
{
    if (!sdChevron.hasVisibleGeometry()) [[unlikely]]
        return {};

    // `computeChevronPoint` clamps `thickness` to `size.y / 2` internally (so the shape cleanly
    // degenerates to a filled triangle when the inner vertices would cross), but we need the
    // same clamped value here to keep the fan apex on the chevron's axis of symmetry.
    const float w = sdChevron.size.x;
    const float h = sdChevron.size.y;
    const float t = ZA_MIN(sdChevron.thickness, h * 0.5f);

    const float innerTipX = w * (1.f - 2.f * t / h);

    // Fan apex in LOCAL coords: midway between inner and outer tip, on the chevron's axis of
    // symmetry. Inside the non-convex chevron for every valid `thickness`.
    const Vec2f localApex{(w + innerTipX) * 0.5f, h * 0.5f};

    return drawTriangleFanShapeFromPoints(6u, sdChevron, [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i) {
        return ShapeUtils::computeChevronPoint(i, sdChevron.size, sdChevron.thickness);
    }, &localApex);
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const HeartShapeData& sdHeart)
{
    if (!sdHeart.hasVisibleGeometry()) [[unlikely]]
        return {};

    return drawTriangleFanShapeFromPoints(sdHeart.pointCount,
                                          sdHeart,
                                          [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i)
    { return ShapeUtils::computeHeartPoint(i, sdHeart.pointCount, sdHeart.size); });
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const CogShapeData& sdCog)
{
    if (!sdCog.hasVisibleGeometry()) [[unlikely]]
        return {};

    return drawTriangleFanShapeFromPoints(4u * sdCog.toothCount,
                                          sdCog,
                                          [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT i)
    {
        return ShapeUtils::computeCogPoint(i, sdCog.toothCount, sdCog.outerRadius, sdCog.innerRadius, sdCog.toothWidthRatio);
    });
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::addTextDataImpl(
    const auto&        glyphSource,
    const auto&        textData,
    const bool         isBold,
    const unsigned int characterSize,
    const float        outlineThickness)
{
    if (textData.string.empty())
        return {};

    const auto fillQuadCount = TextUtils::precomputeTextQuadCount(textData.string, textData.underlined, textData.strikeThrough);
    const auto outlineQuadCount = outlineThickness == 0.f ? 0u : fillQuadCount;

    const auto numQuads = fillQuadCount + outlineQuadCount;

    IndexType* indexPtr  = m_storage.reserveMoreIndices(6u * numQuads);
    const auto nextIndex = m_storage.getNumVertices();

    for (IndexType i = 0u; i < numQuads; ++i)
        DrawableBatchUtils::appendQuadIndices(indexPtr, nextIndex + (i * 4u));

    const auto transform = Transform::fromPositionScaleOriginRotation(textData.position,
                                                                      textData.scale,
                                                                      textData.origin,
                                                                      textData.rotation);

    Vertex* const vertexPtr = m_storage.reserveMoreVertices(4u * numQuads);

    const TextUtils::TextLayoutInputs layoutInputs{
        .bold             = isBold,
        .italic           = textData.italic,
        .underlined       = textData.underlined,
        .strikeThrough    = textData.strikeThrough,
        .characterSize    = characterSize,
        .letterSpacing    = textData.letterSpacing,
        .lineSpacing      = textData.lineSpacing,
        .outlineThickness = outlineThickness,
    };

    TextUtils::createTextGeometryAndGetBounds<
        /* CalculateBounds */ false>(/* outlineVertexCount */ outlineQuadCount * 4u,
                                     glyphSource,
                                     textData.string,
                                     layoutInputs,
                                     [&] [[gnu::always_inline, gnu::flatten]] (za::SizeT & idx,
                                                                               const float lineLength,
                                                                               const float lineTop,
                                                                               const float offset,
                                                                               const float thickness,
                                                                               const float outlineT,
                                                                               const bool  isOutline)
    {
        return TextUtils::addLinePreTransformed(transform,
                                                vertexPtr,
                                                idx,
                                                lineLength,
                                                lineTop,
                                                isOutline ? textData.outlineColor : textData.fillColor,
                                                offset,
                                                thickness,
                                                outlineT);
    },
                                     [&] [[gnu::always_inline, gnu::flatten]] (za::SizeT & idx,
                                                                               const Vec2f  pos,
                                                                               const Glyph& glyph,
                                                                               const float  shear,
                                                                               const bool   isOutline)
    {
        return TextUtils::addGlyphQuadPreTransformed(transform,
                                                     vertexPtr,
                                                     idx,
                                                     pos,
                                                     isOutline ? textData.outlineColor : textData.fillColor,
                                                     glyph,
                                                     shear);
    });

    m_storage.commitMoreIndices(6u * numQuads);
    m_storage.commitMoreVertices(4u * numQuads);

    // `createTextGeometryAndGetBounds` lays outline quads out first, then fill quads.
    return {
        .fill    = {vertexPtr + outlineQuadCount * 4u, fillQuadCount * 4u},
        .outline = {vertexPtr, outlineQuadCount * 4u},
    };
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const Font& font, const TextData& textData)
{
    return addTextDataImpl(font, textData, textData.bold, textData.characterSize, textData.outlineThickness);
}


////////////////////////////////////////////////////////////
template <typename TStorage>
BatchedGeometry DrawableBatchImpl<TStorage>::add(const FontFace&            fontFace,
                                                 const GlyphMapping&        glyphMapping,
                                                 const GlyphMappedTextData& textData)
{
    return addTextDataImpl(GlyphMappingWithKerning{glyphMapping, fontFace},
                           textData,
                           glyphMapping.bold,
                           glyphMapping.characterSize,
                           glyphMapping.outlineThickness);
}

} // namespace za::priv
