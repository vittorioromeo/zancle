// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/RenderTarget.hpp"

#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/GLVAOGroup.hpp"
#include "Zancle/GLUtils/Glad.hpp"

#include "Zancle/Graphics/BatchedGeometry.hpp"
#include "Zancle/Graphics/BlendMode.hpp"
#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DrawIndexedVerticesSettings.hpp"
#include "Zancle/Graphics/DrawInstancedIndexedVerticesSettings.hpp"
#include "Zancle/Graphics/DrawInstancedVerticesSettings.hpp"
#include "Zancle/Graphics/DrawPersistentMappedIndexedVerticesSettings.hpp"
#include "Zancle/Graphics/DrawQuadsSettings.hpp"
#include "Zancle/Graphics/DrawTextureSettings.hpp"
#include "Zancle/Graphics/DrawVerticesSettings.hpp"
#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/DrawableBatchUtils.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/GlyphMappedText.hpp"
#include "Zancle/Graphics/GlyphMappedTextData.hpp"
#include "Zancle/Graphics/GlyphMapping.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/IndexType.hpp"
#include "Zancle/Graphics/InstanceAttributeBinder.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/Priv/EnumToGlEnumConversions.hpp"
#include "Zancle/Graphics/Priv/ShapeDataConcept.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/Shader.hpp"
#include "Zancle/Graphics/Shape.hpp"
#include "Zancle/Graphics/Sprite.hpp"
#include "Zancle/Graphics/StencilMode.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/Transform.hpp"
#include "Zancle/Graphics/VAOHandle.hpp"
#include "Zancle/Graphics/VBOHandle.hpp"
#include "Zancle/Graphics/Vertex.hpp"
#include "Zancle/Graphics/VertexBuffer.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Concurrency/Atomic.hpp"

#include "Zancle/Container/Array.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/FunctionRef.hpp"
#include "Zancle/Vocabulary/Span.hpp"

#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/GetArraySize.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/OffsetOf.hpp"
#include "Zancle/Base/ScopeGuard.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsSame.hpp"

#ifdef ZA_OPENGL_ES
    #include "Zancle/Base/Abort.hpp"
#endif


namespace
{
// A nested named namespace is used here to allow unity builds of Zancle.
namespace RenderTargetImpl
{
////////////////////////////////////////////////////////////
// Type alias for a render target or context id
using IdType = unsigned int;


////////////////////////////////////////////////////////////
// Unique identifier, used for identifying RenderTargets when
// tracking the currently active RenderTarget within a given context
constinit za::Atomic<IdType> nextUniqueId{1ul};


////////////////////////////////////////////////////////////
// Invalid/null render target or context id value
constexpr IdType invalidId{0ul};


////////////////////////////////////////////////////////////
// Maximum supported number of render targets or contexts
constexpr za::SizeT maxIdCount{256ul};


////////////////////////////////////////////////////////////
// Map to help us detect whether a different RenderTarget has been activated within a single context
constinit za::Array<za::Atomic<IdType>, maxIdCount> contextRenderTargetMap{};


////////////////////////////////////////////////////////////
// Check if a render target with the given ID is active in the current context
[[nodiscard]] bool isActive(const IdType contextId, const IdType id)
{
    ZA_ASSERT(contextId < maxIdCount);

    const auto renderTargetId = contextRenderTargetMap[contextId].loadSeqCst();
    return (renderTargetId != invalidId) && (renderTargetId == id);
}


////////////////////////////////////////////////////////////
[[gnu::always_inline, gnu::flatten]] inline void streamVerticesToGPU(za::Span<const za::Vertex> vertexSpan)
{
    glCheck(glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(sizeof(za::Vertex) * vertexSpan.size()),
                         vertexSpan.data(),
                         GL_STREAM_DRAW));
}


////////////////////////////////////////////////////////////
[[gnu::always_inline, gnu::flatten]] inline void streamIndicesToGPU(za::Span<const za::IndexType> indexSpan)
{
    glCheck(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(sizeof(za::IndexType) * indexSpan.size()),
                         indexSpan.data(),
                         GL_STREAM_DRAW));
}


////////////////////////////////////////////////////////////
void setupVertexAttribPointers()
{
#define ZA_PRIV_OFFSETOF(...) reinterpret_cast<const void*>(ZA_OFFSETOF(__VA_ARGS__))

    // Hardcoded layout location `0u` for `za_a_position`
    glCheck(glEnableVertexAttribArray(0u));
    glCheck(glVertexAttribPointer(/*      index */ 0u,
                                  /*       size */ 2,
                                  /*       type */ GL_FLOAT,
                                  /* normalized */ GL_FALSE,
                                  /*     stride */ sizeof(za::Vertex),
                                  /*     offset */ ZA_PRIV_OFFSETOF(za::Vertex, position)));

    // Hardcoded layout location `1u` for `za_a_color`
    glCheck(glEnableVertexAttribArray(1u));
    glCheck(glVertexAttribPointer(/*      index */ 1u,
                                  /*       size */ 4,
                                  /*       type */ GL_UNSIGNED_BYTE,
                                  /* normalized */ GL_TRUE,
                                  /*     stride */ sizeof(za::Vertex),
                                  /*     offset */ ZA_PRIV_OFFSETOF(za::Vertex, color)));

    // Hardcoded layout location `2u` for `za_a_texCoord`
    glCheck(glEnableVertexAttribArray(2u));
    glCheck(glVertexAttribPointer(/*      index */ 2u,
                                  /*       size */ 2,
                                  /*       type */ GL_FLOAT,
                                  /* normalized */ GL_FALSE,
                                  /*     stride */ sizeof(za::Vertex),
                                  /*     offset */ ZA_PRIV_OFFSETOF(za::Vertex, texCoords)));

#undef ZA_PRIV_OFFSETOF
}


////////////////////////////////////////////////////////////
// Emits the winding-corrected `(0,1,2)(2,1,3)` pattern per quad -- the same
// convention as `DrawableBatchUtils::appendQuadIndices`, so the immediate
// (`drawQuads`) and batched paths produce identically-wound triangles.
constexpr unsigned int precomputedQuadIndices[]{
#include "Zancle/Graphics/PrecomputedQuadIndices.inl"
};


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr bool isPrimitiveTypeSupportedByBatchStorage(
    const za::PrimitiveType type) noexcept
{
    return type == za::PrimitiveType::Triangles || type == za::PrimitiveType::TriangleStrip ||
           type == za::PrimitiveType::TriangleFan;
}

} // namespace RenderTargetImpl
} // namespace


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Render states cache
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] StatesCache
{
    bool enable{false};      //!< Is the cache enabled?
    bool glStatesSet{false}; //!< Are our internal GL states set yet?

    View lastView; //!< Cached latest view

    bool scissorEnabled{false}; //!< Is scissor testing enabled?
    bool stencilEnabled{false}; //!< Is stencil testing enabled?

    unsigned int lastVaoGroup{0u};          //!< Last bound vertex array object id
    unsigned int lastVaoGroupContextId{0u}; //!< Last bound vertex array object context id

    unsigned int lastVaoGroupAttribStateGen{0u}; //!< Snapshot of `lastVaoGroup`'s `attribStateGen` at the time
                                                 //!< `bindGLObjects` ran. The VAO group bumps that counter
                                                 //!< whenever its `vbo` or `ebo` is replaced (e.g. on persistent
                                                 //!< ring-buffer growth). A mismatch here proves the VAO's
                                                 //!< recorded attribute-to-buffer mapping refers to a buffer
                                                 //!< instance that no longer exists -- forcing `setupDraw` to
                                                 //!< re-issue `glVertexAttribPointer` so the VAO captures the
                                                 //!< live buffer. This is the load-bearing check: the GL spec
                                                 //!< permits `glGenBuffers` to recycle a just-freed id, so an
                                                 //!< id-only comparison would silently miss the swap.

    BlendMode   lastBlendMode{BlendAlpha}; //!< Cached blending mode
    StencilMode lastStencilMode{};         //!< Cached stencil
    za::U64     lastTextureId{0u};         //!< Cached texture

    GLuint lastProgramId{0u}; //!< GL id of the last used shader program

    za::U32 lastTextureBindingsGeneration{0u}; //!< `Shader::m_textureBindingsGeneration` at the last (re)bind
};


////////////////////////////////////////////////////////////
struct [[nodiscard]] RenderTarget::Impl
{
    ////////////////////////////////////////////////////////////
    StatesCache              cache{};  //!< Render states cache
    RenderTargetImpl::IdType id{};     //!< Unique number that identifies the render target
    GLVAOGroup               vaoGroup; //!< Associated VAO, VBO, and EBO (non-persistent storage)

    ////////////////////////////////////////////////////////////
    CPUDrawableBatch cpuAutoBatch; //!< Internal CPU autobatch

#ifndef ZA_OPENGL_ES
    PersistentGPUDrawableBatch gpuAutoBatch; //!< Internal GPU autobatch (3 frame states for CPU/GPU pipelining)
    za::SizeT                  gpuAutoBatchIndexOffset{0u};  //!< Tracks how many indices have been drawn this frame
    za::SizeT                  gpuAutoBatchVertexOffset{0u}; //!< Tracks how many vertices have been drawn this frame
#endif

    ////////////////////////////////////////////////////////////
    explicit Impl() : id(RenderTargetImpl::nextUniqueId.fetchAddRelaxed(1u))
    {
    }

    ////////////////////////////////////////////////////////////
    void bindGLObjects(const GLVAOGroup& theVAOGroup)
    {
        theVAOGroup.bind();

        cache.lastVaoGroup               = theVAOGroup.getId();
        cache.lastVaoGroupContextId      = GraphicsContext::getActiveThreadLocalGlContextId();
        cache.lastVaoGroupAttribStateGen = theVAOGroup.attribStateGen;

        RenderTargetImpl::setupVertexAttribPointers();
    }
};


////////////////////////////////////////////////////////////
decltype(auto) RenderTarget::withCurrentAutobatch(auto&& f)
{
    ZA_ASSERT(m_autoBatchMode != AutoBatchMode::Disabled);

#ifdef ZA_OPENGL_ES
    return f(m_impl->cpuAutoBatch);
#else
    if (m_autoBatchMode == AutoBatchMode::CPUStorage)
        return f(m_impl->cpuAutoBatch);

    ZA_ASSERT(m_autoBatchMode == AutoBatchMode::GPUStorage);
    return f(m_impl->gpuAutoBatch);
#endif
}


////////////////////////////////////////////////////////////
auto RenderTarget::addToAutoBatch(auto&&... xs)
{
    ZA_ASSERT(m_autoBatchMode != AutoBatchMode::Disabled);

    return withCurrentAutobatch([&] [[gnu::always_inline]] (auto& batch)
    {
        const auto prevVertices = batch.getNumVertices();
        ZA_SCOPE_GUARD({ m_numAutoBatchVertices += batch.getNumVertices() - prevVertices; });

        return batch.add(ZA_FORWARD(xs)...);
    });
}


////////////////////////////////////////////////////////////
RenderTarget::RenderTarget(const bool isSrgb) : m_isSrgb{isSrgb}
{
}


////////////////////////////////////////////////////////////
RenderTarget::~RenderTarget()                                  = default;
RenderTarget::RenderTarget(RenderTarget&&) noexcept            = default;
RenderTarget& RenderTarget::operator=(RenderTarget&&) noexcept = default;


////////////////////////////////////////////////////////////
[[nodiscard]] bool RenderTarget::prepare()
{
    if (!setActive(true))
    {
        priv::errMsg("Failed to activate render target in `prepare`");
        return false;
    }

    // Unbind texture to fix RenderTexture preventing clear
    unapplyTexture(); // See https://en.zancle.org/forums/index.php?topic=9350

    // Disable scissor so `clear` always affects the whole target.
    // The next `setupDraw` will automatically re-enable it if the view demands it.
    if (!m_impl->cache.enable || m_impl->cache.scissorEnabled)
    {
        glCheck(glDisable(GL_SCISSOR_TEST));
        m_impl->cache.scissorEnabled = false;

        // Also invalidate the cached view: `applyView` is the only place that
        // re-enables scissor, and `setupDraw` skips it when the view compares
        // equal to the cache -- so an unchanged scissored view would otherwise
        // render unclipped from the second frame onwards.
        m_impl->cache.lastView = View{};
    }

    // Reset the stencil write mask so `clear`/`clearStencil` always affect
    // every stencil bit (`glClear` honors the write mask). Mirror the change
    // in the cached mode so the next stenciled `setupDraw` sees a mismatch
    // and re-applies the user's mask via `applyStencilMode`.
    if (!m_impl->cache.enable || m_impl->cache.lastStencilMode.stencilMask.value != ~0u)
    {
        glCheck(glStencilMask(~0u));
        m_impl->cache.lastStencilMode.stencilMask = StencilValue{~0u};
    }

    return true;
}


////////////////////////////////////////////////////////////
void RenderTarget::clear(const Color color)
{
    // Drain any pending auto-batch first. Otherwise queued CPU-side draws would survive
    // the `glClear` (the batch is just RAM until flushed) and be painted onto the cleared
    // framebuffer at the next flush -- surprising the user who expected `clear` to wipe
    // everything before it.
    flush();

    if (!prepare())
        return;

    glCheck(glClearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT));
}


////////////////////////////////////////////////////////////
void RenderTarget::clearStencil(const StencilValue stencilValue)
{
    flush(); // see comment in `clear(Color)`

    if (!prepare())
        return;

    glCheck(glClearStencil(static_cast<int>(stencilValue.value)));
    glCheck(glClear(GL_STENCIL_BUFFER_BIT));
}


////////////////////////////////////////////////////////////
void RenderTarget::clear(const Color color, const StencilValue stencilValue)
{
    flush(); // see comment in `clear(Color)`

    if (!prepare())
        return;

    glCheck(glClearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f));
    glCheck(glClearStencil(static_cast<int>(stencilValue.value)));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
}


////////////////////////////////////////////////////////////
void RenderTarget::setAutoBatchMode(const AutoBatchMode mode)
{
    if (m_autoBatchMode == mode)
        return;

    flush();
    m_autoBatchMode = mode;
}


////////////////////////////////////////////////////////////
RenderTarget::AutoBatchMode RenderTarget::getAutoBatchMode() const
{
    return m_autoBatchMode;
}


////////////////////////////////////////////////////////////
void RenderTarget::setAutoBatchVertexThreshold(const za::SizeT threshold)
{
    if (m_autoBatchVertexThreshold == threshold)
        return;

    flush();
    m_autoBatchVertexThreshold = threshold;
}


////////////////////////////////////////////////////////////
za::SizeT RenderTarget::getAutoBatchVertexThreshold() const
{
    return m_autoBatchVertexThreshold;
}


////////////////////////////////////////////////////////////
void RenderTarget::reserveAutoBatchTriangles(const za::SizeT triangleCount)
{
    if (triangleCount == 0u || m_autoBatchMode == AutoBatchMode::Disabled)
        return;

    withCurrentAutobatch([&](auto& batch) { batch.reserveTriangles(triangleCount); });
}


////////////////////////////////////////////////////////////
void RenderTarget::reserveAutoBatchQuads(const za::SizeT quadCount)
{
    if (quadCount == 0u || m_autoBatchMode == AutoBatchMode::Disabled)
        return;

    withCurrentAutobatch([&](auto& batch) { batch.reserveQuads(quadCount); });
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const Texture& texture, RenderStates states)
{
    states.texture = &texture;

    if (m_autoBatchMode != AutoBatchMode::Disabled)
    {
        flushIfNeeded(states);
        addToAutoBatch(Sprite{.textureRect = texture.getRect()});
    }
    else
    {
        Vertex buffer[4];
        DrawableBatchUtils::appendPreTransformedSpriteQuadVertices(Transform{}, texture.getRect(), Color::White, buffer);
        draw(buffer, PrimitiveType::TriangleStrip, states);
    }
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const Texture& texture, const DrawTextureSettings& params, RenderStates states)
{
    states.texture = &texture;

    if (m_autoBatchMode != AutoBatchMode::Disabled)
    {
        flushIfNeeded(states);

        addToAutoBatch(Sprite{
            .position    = params.position,
            .scale       = params.scale,
            .origin      = params.origin,
            .rotation    = params.rotation,
            .textureRect = (params.textureRect == Rect2f{}) ? texture.getRect() : params.textureRect,
            .color       = params.color,
        });
    }
    else
    {
        Vertex buffer[4];

        DrawableBatchUtils::appendPreTransformedSpriteQuadVertices(Transform::fromPositionScaleOriginRotation(params.position,
                                                                                                              params.scale,
                                                                                                              params.origin,
                                                                                                              params.rotation),
                                                                   (params.textureRect == Rect2f{}) ? texture.getRect()
                                                                                                    : params.textureRect,
                                                                   params.color,
                                                                   buffer);

        draw(buffer, PrimitiveType::TriangleStrip, states);
    }
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const Sprite& sprite, const RenderStates& states)
{
    ZA_ASSERT(states.texture != nullptr);

    if (m_autoBatchMode != AutoBatchMode::Disabled)
    {
        flushIfNeeded(states);
        addToAutoBatch(sprite);
    }
    else
    {
        Vertex buffer[4];
        DrawableBatchUtils::appendPreTransformedSpriteQuadVertices(sprite.getTransform(), sprite.textureRect, sprite.color, buffer);
        draw(buffer, PrimitiveType::TriangleStrip, states);
    }
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const Shape& shape, RenderStates states)
{
    if (m_autoBatchMode != AutoBatchMode::Disabled)
    {
        flushIfNeeded(states);
        addToAutoBatch(shape);
    }
    else
    {
        states.transform *= shape.getTransform();

        immediateDrawVertices(
            {
                .vertexSpan    = shape.getFillVertices(),
                .primitiveType = PrimitiveType::TriangleFan,
            },
            states);

        if (shape.getOutlineThickness() != 0.f)
            immediateDrawVertices(
                {
                    .vertexSpan    = shape.getOutlineVertices(),
                    .primitiveType = PrimitiveType::TriangleStrip,
                },
                states);
    }
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const Text& text, RenderStates states)
{
    states.texture = &text.getTexture();

    if (m_autoBatchMode != AutoBatchMode::Disabled)
    {
        flushIfNeeded(states);
        addToAutoBatch(text);
    }
    else
    {
        text.draw(*this, states);
    }
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const GlyphMappedText& text, RenderStates states)
{
    states.texture = &text.getTexture();

    if (m_autoBatchMode != AutoBatchMode::Disabled)
    {
        flushIfNeeded(states);
        addToAutoBatch(text);
    }
    else
    {
        text.draw(*this, states);
    }
}


////////////////////////////////////////////////////////////
struct [[nodiscard]] RenderTarget::DrawGuard
{
    RenderTarget& renderTarget;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] explicit DrawGuard(RenderTarget&       theRenderTarget,
                                                         const RenderStates& theRenderStates,
                                                         const GLVAOGroup&   vaoGroup) :
        renderTarget(theRenderTarget)
    {
        renderTarget.setupDraw(vaoGroup, theRenderStates);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] ~DrawGuard()
    {
        renderTarget.cleanupDraw(renderTarget.m_lastRenderStates);
    }
};


////////////////////////////////////////////////////////////
void RenderTarget::immediateDrawVertices(const DrawVerticesSettings& settings, const RenderStates& states)
{
    // Nothing to draw or inactive target
    if (settings.vertexSpan.isNullOrEmpty() || !setActive(true))
        return;

    const DrawGuard drawGuard{*this, states, m_impl->vaoGroup};

    RenderTargetImpl::streamVerticesToGPU(settings.vertexSpan);
    invokePrimitiveDrawCall(settings.primitiveType, 0u, settings.vertexSpan.size());
}


////////////////////////////////////////////////////////////
void RenderTarget::immediateDrawIndexedVertices(const DrawIndexedVerticesSettings& settings, const RenderStates& states)
{
    // Nothing to draw or inactive target
    if (settings.vertexSpan.isNullOrEmpty() || settings.indexSpan.isNullOrEmpty() || !setActive(true))
        return;

    const DrawGuard drawGuard{*this, states, m_impl->vaoGroup};

    RenderTargetImpl::streamVerticesToGPU(settings.vertexSpan);
    RenderTargetImpl::streamIndicesToGPU(settings.indexSpan);

    invokePrimitiveDrawCallIndexed(settings.primitiveType, settings.indexSpan.size(), /* indexOffset */ 0u);
}


////////////////////////////////////////////////////////////
void RenderTarget::immediateDrawPersistentMappedIndexedVertices(
    [[maybe_unused]] const DrawPersistentMappedIndexedVerticesSettings& settings,
    [[maybe_unused]] const RenderStates&                                states)
{
#ifdef ZA_OPENGL_ES
    priv::errMsg("FATAL ERROR: Persistent OpenGL buffers are not available in OpenGL ES");
    za::abort();
#else
    // Nothing to draw or inactive target
    if (settings.indexCount == 0u || !setActive(true))
        return;

    const DrawGuard drawGuard{*this,
                              states,
                              *static_cast<const GLVAOGroup*>(settings.gpuDrawableBatch.m_storage.getVAOGroup())};

    invokePrimitiveDrawCallIndexedBaseVertex(settings.primitiveType, settings.indexCount, settings.indexOffset, settings.vertexOffset);
#endif
}


////////////////////////////////////////////////////////////
void RenderTarget::immediateDrawDrawableBatch(const CPUDrawableBatch& drawableBatch, RenderStates states)
{
    ZA_ASSERT(drawableBatch.m_storage.indices.size() % 3u == 0u);

    states.transform *= drawableBatch.getTransform();

    immediateDrawIndexedVertices(
        {
            .vertexSpan    = drawableBatch.m_storage.vertices,
            .indexSpan     = drawableBatch.m_storage.indices,
            .primitiveType = PrimitiveType::Triangles,
        },
        states);
}


////////////////////////////////////////////////////////////
void RenderTarget::immediateDrawInstancedVertices(const DrawInstancedVerticesSettings&            settings,
                                                  za::FunctionRef<void(InstanceAttributeBinder&)> setupFn,
                                                  const RenderStates&                             states)
{
    // Nothing to draw or inactive target
    if (settings.vertexSpan.isNullOrEmpty() || settings.instanceCount == 0u || !setActive(true))
        return;

    const DrawGuard drawGuard{*this, states, settings.vaoHandle.asVAOGroup()};

    RenderTargetImpl::streamVerticesToGPU(settings.vertexSpan);

    InstanceAttributeBinder iab{settings.instanceCount};
    setupFn(iab);
    iab.applySetups();

    invokeInstancedPrimitiveDrawCall(settings.primitiveType, 0, settings.vertexSpan.size(), settings.instanceCount);
    iab.markDrawSubmitted();
}


////////////////////////////////////////////////////////////
void RenderTarget::immediateDrawInstancedIndexedVertices(const DrawInstancedIndexedVerticesSettings&     settings,
                                                         za::FunctionRef<void(InstanceAttributeBinder&)> setupFn,
                                                         const RenderStates&                             states)
{
    // Nothing to draw or inactive target
    if (settings.vertexSpan.isNullOrEmpty() || settings.indexSpan.isNullOrEmpty() || settings.instanceCount == 0u ||
        !setActive(true))
        return;

    const DrawGuard drawGuard{*this, states, settings.vaoHandle.asVAOGroup()};

    RenderTargetImpl::streamVerticesToGPU(settings.vertexSpan);
    RenderTargetImpl::streamIndicesToGPU(settings.indexSpan);

    InstanceAttributeBinder iab{settings.instanceCount};
    setupFn(iab);
    iab.applySetups();

    invokeInstancedPrimitiveDrawCallIndexed(settings.primitiveType, 0, settings.indexSpan.size(), settings.instanceCount);
    iab.markDrawSubmitted();
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const CPUDrawableBatch& drawableBatch, const RenderStates& states)
{
    if (m_autoBatchMode != AutoBatchMode::Disabled)
        flush();

    immediateDrawDrawableBatch(drawableBatch, states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const PersistentGPUDrawableBatch& drawableBatch, RenderStates states)
{
    // Empty batch: nothing to draw. Also avoids flushing a batch that was
    // never filled, whose persistent buffers were never created/mapped
    // (`flushBytesToGPU` asserts a live buffer object and mapping).
    if (drawableBatch.getNumIndices() == 0u)
        return;

    if (m_autoBatchMode != AutoBatchMode::Disabled)
        flush();

    if (!setActive(true))
        return;

    states.transform *= drawableBatch.getTransform();

    drawableBatch.flushVertexWritesToGPU(drawableBatch.getNumVertices(), 0u);
    drawableBatch.flushIndexWritesToGPU(drawableBatch.getNumIndices(), 0u);

    immediateDrawPersistentMappedIndexedVertices(
        {
            .gpuDrawableBatch = drawableBatch,
            .indexCount       = drawableBatch.getNumIndices(),
            .indexOffset      = 0u,
            .vertexOffset     = 0u,
            .primitiveType    = PrimitiveType::Triangles,
        },
        states);

    drawableBatch.m_storage.commitPendingDrawSubmission();
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const VertexBuffer& vertexBuffer, const RenderStates& states)
{
    draw(vertexBuffer, 0, vertexBuffer.getVertexCount(), states);
}


////////////////////////////////////////////////////////////
void RenderTarget::draw(const VertexBuffer& vertexBuffer,
                        const za::SizeT     firstVertex,
                        za::SizeT           vertexCount,
                        const RenderStates& states)
{
    if (m_autoBatchMode != AutoBatchMode::Disabled)
        flush();

    // Sanity check
    if (firstVertex > vertexBuffer.getVertexCount())
        return;

    // Clamp vertexCount to something that makes sense
    vertexCount = za::min(vertexCount, vertexBuffer.getVertexCount() - firstVertex);

    // Nothing to draw or inactive target

    if (!vertexCount || !vertexBuffer.getNativeHandle() || !setActive(true))
        return;

    const DrawGuard drawGuard{*this, states, m_impl->vaoGroup};

    // Bind vertex buffer
    vertexBuffer.bind();

    // Always enable texture coordinates (needed because different buffer is bound)
    RenderTargetImpl::setupVertexAttribPointers();

    invokePrimitiveDrawCall(vertexBuffer.getPrimitiveType(), firstVertex, vertexCount);

    // Unbind vertex buffer
    VertexBuffer::unbind();

    // Needed to restore attrib pointers on regular VBO
    m_impl->bindGLObjects(m_impl->vaoGroup);
}


////////////////////////////////////////////////////////////
BatchedGeometry RenderTarget::draw(const priv::ShapeDataConcept auto& shapeData, const RenderStates& states)
{
    if (m_autoBatchMode != AutoBatchMode::Disabled)
    {
        flushIfNeeded(states);
        return addToAutoBatch(shapeData);
    }

    m_impl->cpuAutoBatch.clear();

    ZA_SCOPE_GUARD({ immediateDrawDrawableBatch(m_impl->cpuAutoBatch, states); });
    return m_impl->cpuAutoBatch.add(shapeData);
}


////////////////////////////////////////////////////////////
template BatchedGeometry RenderTarget::draw(const ArrowShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const ChevronShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const CircleShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const CogShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const CrossShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const CurvedArrowShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const EllipseShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const HeartShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const PieSliceShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const RectangleShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const RingShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const RingPieSliceShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const RoundedRectangleShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const StarShapeData&, const RenderStates&);
template BatchedGeometry RenderTarget::draw(const TrapezoidShapeData&, const RenderStates&);


////////////////////////////////////////////////////////////
BatchedGeometry RenderTarget::draw(const Font& font, const TextData& textData, RenderStates states)
{
    states.texture = &font.getTexture();

    if (m_autoBatchMode != AutoBatchMode::Disabled)
    {
        flushIfNeeded(states);
        return addToAutoBatch(font, textData);
    }

    m_impl->cpuAutoBatch.clear();

    ZA_SCOPE_GUARD({ immediateDrawDrawableBatch(m_impl->cpuAutoBatch, states); });
    return m_impl->cpuAutoBatch.add(font, textData);
}


////////////////////////////////////////////////////////////
BatchedGeometry RenderTarget::draw(const FontFace&            fontFace,
                                   const GlyphMapping&        glyphMapping,
                                   const GlyphMappedTextData& textData,
                                   const RenderStates&        states)
{
    ZA_ASSERT(states.texture != nullptr);

    if (m_autoBatchMode != AutoBatchMode::Disabled)
    {
        flushIfNeeded(states);
        return addToAutoBatch(fontFace, glyphMapping, textData);
    }

    m_impl->cpuAutoBatch.clear();

    ZA_SCOPE_GUARD({ immediateDrawDrawableBatch(m_impl->cpuAutoBatch, states); });
    return m_impl->cpuAutoBatch.add(fontFace, glyphMapping, textData);
}


////////////////////////////////////////////////////////////
void RenderTarget::drawVertices(const DrawVerticesSettings& settings, const RenderStates& states)
{
    if (RenderTargetImpl::isPrimitiveTypeSupportedByBatchStorage(settings.primitiveType) &&
        m_autoBatchMode != AutoBatchMode::Disabled)
    {
        flushIfNeeded(states);
        addToAutoBatch(settings);
        return;
    }

    flush();
    immediateDrawVertices(settings, states);
}


////////////////////////////////////////////////////////////
void RenderTarget::drawIndexedVertices(const DrawIndexedVerticesSettings& settings, const RenderStates& states)
{
    if (RenderTargetImpl::isPrimitiveTypeSupportedByBatchStorage(settings.primitiveType) &&
        m_autoBatchMode != AutoBatchMode::Disabled)
    {
        flushIfNeeded(states);
        addToAutoBatch(settings);
        return;
    }

    flush();
    immediateDrawIndexedVertices(settings, states);
}


////////////////////////////////////////////////////////////
void RenderTarget::drawQuads(const DrawQuadsSettings& settings, const RenderStates& states)
{
    // Keep the public per-call constants in sync with the precomputed index array.
    static_assert(drawQuadsMaxQuadsPerCall * 6u == za::getArraySize(RenderTargetImpl::precomputedQuadIndices));
    static_assert(drawQuadsMaxVerticesPerCall == drawQuadsMaxQuadsPerCall * 4u);

    const auto totalVertexCount = settings.vertexSpan.size();

    ZA_ASSERT(totalVertexCount % 4u == 0u);

    // Split draws exceeding the precomputed index table into multiple calls
    // (e.g. `Text` with 65'536+ quads); an unsplit call would build an index
    // span past the end of the table.
    for (za::SizeT offset = 0u; offset < totalVertexCount; offset += drawQuadsMaxVerticesPerCall)
    {
        const za::SizeT vertexCount = za::min(totalVertexCount - offset, za::SizeT{drawQuadsMaxVerticesPerCall});

        drawIndexedVertices(
            {
                .vertexSpan    = {settings.vertexSpan.data() + offset, vertexCount},
                .indexSpan     = {RenderTargetImpl::precomputedQuadIndices, vertexCount / 4u * 6u},
                .primitiveType = settings.primitiveType,
            },
            states);
    }
}


////////////////////////////////////////////////////////////
void RenderTarget::drawPersistentMappedIndexedVertices(const DrawPersistentMappedIndexedVerticesSettings& settings,
                                                       const RenderStates&                                states)
{
    flush();
    immediateDrawPersistentMappedIndexedVertices(settings, states);
}


////////////////////////////////////////////////////////////
void RenderTarget::drawInstancedVertices(const DrawInstancedVerticesSettings&            settings,
                                         za::FunctionRef<void(InstanceAttributeBinder&)> setupFn,
                                         const RenderStates&                             states)
{
    flush();
    immediateDrawInstancedVertices(settings, setupFn, states);
}


////////////////////////////////////////////////////////////
void RenderTarget::drawInstancedIndexedVertices(const DrawInstancedIndexedVerticesSettings&     settings,
                                                za::FunctionRef<void(InstanceAttributeBinder&)> setupFn,
                                                const RenderStates&                             states)
{
    flush();
    immediateDrawInstancedIndexedVertices(settings, setupFn, states);
}


////////////////////////////////////////////////////////////
bool RenderTarget::setActive(const bool active)
{
    // Mark this RenderTarget as active or no longer active in the tracking map
    const RenderTargetImpl::IdType contextId = GraphicsContext::getActiveThreadLocalGlContextId();

    // If this RenderTarget is already active on the current GL context, do nothing
    if (const bool isAlreadyActive = RenderTargetImpl::isActive(contextId, m_impl->id);
        (active && isAlreadyActive) || (!active && !isAlreadyActive))
        return true;

    ZA_ASSERT(contextId < RenderTargetImpl::maxIdCount);
    za::Atomic<RenderTargetImpl::IdType>& renderTargetId = RenderTargetImpl::contextRenderTargetMap[contextId];

    [[maybe_unused]] const auto loadedRenderTargetId = renderTargetId.loadSeqCst();

    // Deactivation
    if (!active)
    {
        ZA_ASSERT(loadedRenderTargetId != RenderTargetImpl::invalidId);
        renderTargetId.storeSeqCst(RenderTargetImpl::invalidId);

        m_impl->cache.enable = false;
        return true;
    }

    // The slot held either `invalidId` (first activation in this context)
    // or a different RT id (another RT was last active here, possibly on
    // a different context entirely). In both cases this RT's persistent
    // GL state (cull, depth, color mask, ...) is not guaranteed to match
    // what `cache.glStatesSet` claims -- it was set under whichever
    // context the previous draw ran on, or never set at all. Force a
    // full `resetGLStatesImpl` on the next `setupDraw`.
    ZA_ASSERT(loadedRenderTargetId != m_impl->id);
    renderTargetId.storeSeqCst(m_impl->id);

    m_impl->cache.glStatesSet = false;
    m_impl->cache.enable      = false;
    return true;
}


////////////////////////////////////////////////////////////
View RenderTarget::computeView() const
{
    return View::fromScreenSize(getSize().toVec2f());
}


////////////////////////////////////////////////////////////
void RenderTarget::resetGLStates()
{
    flush();
    resetGLStatesImpl();
}


////////////////////////////////////////////////////////////
void RenderTarget::resetGLStatesImpl()
{
// Workaround for states not being properly reset on
// macOS unless a context switch really takes place
#ifdef ZA_SYSTEM_MACOS
    if (!setActive(false))
        priv::errMsg("Failed to set render target inactive");
#endif

    if (!setActive(true))
        return;

#ifdef ZA_DEBUG
    // Make sure that the user didn't leave an unchecked OpenGL error
    if (const GLenum error = glGetError(); error != GL_NO_ERROR)
        priv::errMsg("OpenGL error ({}) detected in user code, you should check for errors with glGetError()", error);
#endif

    // Make sure that the texture unit which is active is the number 0
    glCheck(glActiveTexture(GL_TEXTURE0));

    // Define the default OpenGL states
    glCheck(glDisable(GL_CULL_FACE));
    glCheck(glDisable(GL_STENCIL_TEST));
    glCheck(glDisable(GL_DEPTH_TEST));
    glCheck(glDisable(GL_SCISSOR_TEST));
    glCheck(glEnable(GL_BLEND));
    glCheck(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

    m_impl->cache.scissorEnabled             = false;
    m_impl->cache.stencilEnabled             = false;
    m_impl->cache.lastVaoGroup               = 0u;
    m_impl->cache.lastVaoGroupAttribStateGen = 0u;

    m_impl->cache.glStatesSet = true;

    // Apply the default Zancle states
    applyBlendMode(BlendAlpha);
    applyStencilMode(StencilMode{});
    unapplyTexture();

    {
        Shader::unbind();
        m_impl->cache.lastProgramId = 0u;
    }

    VertexBuffer::unbind();

    // Reapply the current view to re-sync `glViewport` / `glScissor` with
    // `cache.lastView`. Otherwise, raw GL (or third-party code such as the
    // ImGui backend) that mutated the viewport between draws can leave the
    // pipeline pointing at the wrong rectangle: the next `applyDrawCacheStates`
    // would compare `usedView == cache.lastView`, find them equal, and skip
    // the `applyView` call that would otherwise correct it.
    applyView(m_impl->cache.lastView);

    m_impl->cache.enable = true;
}


////////////////////////////////////////////////////////////
bool RenderTarget::isScissorEnabledCached() const
{
    return m_impl->cache.scissorEnabled;
}


////////////////////////////////////////////////////////////
RenderTarget::DrawStatistics RenderTarget::flush()
{
    if (m_numAutoBatchVertices > 0u)
    {
        if (m_lastRenderStates.shader != nullptr &&
            m_lastRenderStates.shader->m_uniformGeneration != m_lastShaderGeneration) [[unlikely]]
        {
            priv::errMsg(
                "Shader uniform mutation detected while autobatch was in flight -- call `flush()` before mutating "
                "uniforms on a pending-draw shader");
        }

        if (m_lastRenderStates.texture != nullptr &&
            m_lastRenderStates.texture->m_destructiveGeneration != m_lastTextureGeneration) [[unlikely]]
        {
            priv::errMsg(
                "Destructive texture mutation detected while autobatch was in flight -- call `flush()` before "
                "destructively modifying a pending-draw texture");
        }
    }

    ZA_SCOPE_GUARD({
        // Only refresh the cached generations if a batch was actually
        // flushed. With an empty batch, `m_lastRenderStates` may still
        // reference a shader or texture the user has since destroyed.
        if (m_numAutoBatchVertices > 0u)
            updateCachedGenerations(m_lastRenderStates);

        m_numAutoBatchVertices = 0u;
    });

    if (m_autoBatchMode == AutoBatchMode::Disabled)
        return m_currentDrawStats;

    withCurrentAutobatch([&]<typename Batch>(Batch& b)
    {
        if constexpr (ZA_IS_SAME(Batch, CPUDrawableBatch))
        {
            immediateDrawDrawableBatch(b, m_lastRenderStates);
            b.clear();
        }
        else
        {
#ifndef ZA_OPENGL_ES
            auto& batch        = m_impl->gpuAutoBatch;
            auto& indexOffset  = m_impl->gpuAutoBatchIndexOffset;
            auto& vertexOffset = m_impl->gpuAutoBatchVertexOffset;

            ZA_ASSERT(&batch == &b);

            const auto vertexCount = batch.getNumVertices() - vertexOffset;
            const auto indexCount  = batch.getNumIndices() - indexOffset;

            if (vertexCount == 0u || indexCount == 0u)
                return;

            batch.flushVertexWritesToGPU(vertexCount, vertexOffset);
            batch.flushIndexWritesToGPU(indexCount, indexOffset);

            immediateDrawPersistentMappedIndexedVertices(
                {
                    .gpuDrawableBatch = batch,
                    .indexCount       = indexCount,
                    .indexOffset      = indexOffset,
                    .vertexOffset     = 0u, // Vertex offset is always `0` for GPU autobatching
                    .primitiveType    = PrimitiveType::Triangles,
                },
                m_lastRenderStates);

            indexOffset  = batch.getNumIndices();
            vertexOffset = batch.getNumVertices();
#endif
        }
    });

    return m_currentDrawStats;
}


////////////////////////////////////////////////////////////
void RenderTarget::invokeGlFlush()
{
    glCheck(glFlush());
}


////////////////////////////////////////////////////////////
void RenderTarget::invokeGlFinish()
{
    glCheck(glFinish());
}


////////////////////////////////////////////////////////////
RenderTarget::WithRenderStatesContext::WithRenderStatesContext(RenderTarget& rt, const RenderStates& states, const bool locked) :
    m_rt{&rt},
    m_states{states},
    m_locked{locked}
{
    // Sync `m_lastRenderStates` (and flush if needed) regardless of the
    // auto-batch mode: with `AutoBatchMode::Disabled`, skipping this left
    // `m_lastRenderStates` stale, so the locked-state assertion in
    // `flushIfNeeded` could fire spuriously on the first `draw` through the
    // context. `flush()` is a no-op when auto-batching is disabled.
    m_rt->flushIfNeeded(m_states);

    if (m_locked)
    {
        ZA_ASSERT(!m_rt->m_isStateLocked && "Cannot create a context while another is active");
        m_rt->m_isStateLocked = true;
    }
}


////////////////////////////////////////////////////////////
RenderTarget::WithRenderStatesContext::WithRenderStatesContext::~WithRenderStatesContext()
{
    if (m_locked)
    {
        ZA_ASSERT(m_rt->m_isStateLocked && "Cannot destroy a context while no context is active");
        m_rt->m_isStateLocked = false;
    }
}


////////////////////////////////////////////////////////////
void RenderTarget::syncGPUEndFrame()
{
    // Statistics are per-frame, and `display` is the frame boundary: both
    // `display` implementations capture `flush()`'s return value before
    // calling this, so resetting here starts the next frame's counters.
    // (Resetting in `prepare` instead would tie the counters to `clear`,
    // dropping work flushed by `clear` itself and breaking no-`clear` loops.)
    m_currentDrawStats = {};

#ifndef ZA_OPENGL_ES
    // Commit the frame's ring buffer writes: the fence inserted by commit()
    // covers all GPU work up to this point, including this frame's draws
    // (both `display` implementations `flush()` before calling this, so no
    // unflushed vertices can be pending).
    m_impl->gpuAutoBatch.m_storage.commitPendingDrawSubmission();

    // Rotate the GPU auto-batch to the next frame state and drain it. The
    // drained state was last used 2 frames ago, so its fence is almost
    // always signaled and drain returns instantly. Rotating eagerly at the
    // frame boundary (rather than lazily at the next append) is safe:
    // cursors are only ever reset by this explicit `drain`, never by
    // `reclaim`/overflow handling, so draws issued after `display` without
    // an intervening `clear` simply land in the fresh frame state.
    m_impl->gpuAutoBatch.clear();

    m_impl->gpuAutoBatchIndexOffset  = 0u;
    m_impl->gpuAutoBatchVertexOffset = 0u;
#endif
}


////////////////////////////////////////////////////////////
void RenderTarget::applyView(const View& view)
{
    const Vec2f targetSize = getSize().toVec2f();

    // Set the viewport
    const Rect2i viewport    = view.computePixelViewport(targetSize);
    const int    viewportTop = static_cast<int>(targetSize.y) - (viewport.position.y + viewport.size.y);

    glCheck(glViewport(viewport.position.x, viewportTop, viewport.size.x, viewport.size.y));

    // Set the scissor rectangle and enable/disable scissor testing
    if (view.scissor == Rect2f{{0.f, 0.f}, {1.f, 1.f}})
    {
        if (!m_impl->cache.enable || m_impl->cache.scissorEnabled)
        {
            glCheck(glDisable(GL_SCISSOR_TEST));
            m_impl->cache.scissorEnabled = false;
        }
    }
    else
    {
        const Rect2i pixelScissor = view.computePixelScissor(targetSize);
        const int    scissorTop   = static_cast<int>(targetSize.y) - (pixelScissor.position.y + pixelScissor.size.y);

        glCheck(glScissor(pixelScissor.position.x, scissorTop, pixelScissor.size.x, pixelScissor.size.y));

        if (!m_impl->cache.enable || !m_impl->cache.scissorEnabled)
        {
            glCheck(glEnable(GL_SCISSOR_TEST));
            m_impl->cache.scissorEnabled = true;
        }
    }

    m_impl->cache.lastView = view;
}


////////////////////////////////////////////////////////////
void RenderTarget::applyBlendMode(const BlendMode& mode)
{
    glCheck(glBlendFuncSeparate(priv::factorToGlConstant(mode.colorSrcFactor),
                                priv::factorToGlConstant(mode.colorDstFactor),
                                priv::factorToGlConstant(mode.alphaSrcFactor),
                                priv::factorToGlConstant(mode.alphaDstFactor)));

    glCheck(glBlendEquationSeparate(priv::equationToGlConstant(mode.colorEquation),
                                    priv::equationToGlConstant(mode.alphaEquation)));

    m_impl->cache.lastBlendMode = mode;
}


////////////////////////////////////////////////////////////
void RenderTarget::applyStencilMode(const StencilMode& mode)
{
    m_impl->cache.lastStencilMode = mode;

    // Fast path if we have a default (disabled) stencil mode
    if (mode == StencilMode{})
    {
        if (!m_impl->cache.enable || m_impl->cache.stencilEnabled)
        {
            glCheck(glDisable(GL_STENCIL_TEST));
            glCheck(glStencilMask(~0u)); // Restore the full write mask so stencil clears affect every bit

            m_impl->cache.stencilEnabled = false;
        }

        return;
    }

    // Apply the stencil mode
    if (!m_impl->cache.enable || !m_impl->cache.stencilEnabled)
        glCheck(glEnable(GL_STENCIL_TEST));

    glCheck(glStencilOp(GL_KEEP,
                        priv::stencilOperationToGlConstant(mode.stencilUpdateOperation),
                        priv::stencilOperationToGlConstant(mode.stencilUpdateOperation)));

    glCheck(glStencilFunc(priv::stencilFunctionToGlConstant(mode.stencilComparison),
                          static_cast<int>(mode.stencilReference.value),
                          mode.stencilMask.value));

    // Mask which stencil bits the update operation may write
    glCheck(glStencilMask(mode.stencilMask.value));

    m_impl->cache.stencilEnabled = true;
}


////////////////////////////////////////////////////////////
void RenderTarget::unapplyTexture()
{
    Texture::unbind();
    m_impl->cache.lastTextureId = 0ul;
}


////////////////////////////////////////////////////////////
void RenderTarget::setupDraw(const GLVAOGroup& vaoGroup, const RenderStates& states)
{
    // GL_FRAMEBUFFER_SRGB is not available on OpenGL ES
    // If a framebuffer supports sRGB, it will always be enabled on OpenGL ES
#ifndef ZA_OPENGL_ES
    // Enable or disable sRGB encoding
    // This is needed for drivers that do not check the format of the surface drawn to before applying sRGB conversion
    if (!m_impl->cache.enable)
    {
        if (isSrgb())
            glCheck(glEnable(GL_FRAMEBUFFER_SRGB));
        else
            glCheck(glDisable(GL_FRAMEBUFFER_SRGB));
    }
#endif

    // First set the persistent OpenGL states if it's the very first call
    if (!m_impl->cache.glStatesSet)
        resetGLStatesImpl();

    // Bind GL objects
    {
        const bool mustRebindVAO = m_impl->cache.lastVaoGroup == 0u || m_impl->cache.lastVaoGroup != vaoGroup.getId() ||
                                   m_impl->cache.lastVaoGroupContextId == 0u ||
                                   m_impl->cache.lastVaoGroupContextId !=
                                       GraphicsContext::getActiveThreadLocalGlContextId() ||
                                   m_impl->cache.lastVaoGroupAttribStateGen != vaoGroup.attribStateGen;

        if (!m_impl->cache.enable || mustRebindVAO)
        {
            // Full rebind path. Also runs after a persistent ring-buffer
            // growth, which move-assigns a fresh `GLBufferObject` into the
            // VAO group: the grow path bumps `attribStateGen`, the
            // mismatch above forces this branch, and
            // `setupVertexAttribPointers` re-captures the live VBO handle
            // into the VAO's attribute state.
            m_impl->bindGLObjects(vaoGroup);
        }
        else
        {
            // Same VAO and its attribute state is still valid -- only the
            // `GL_ARRAY_BUFFER` target binding may have drifted (instanced
            // draw callbacks bind per-instance VBOs, `VertexBuffer::create`
            // / `update` end with `glBindBuffer(GL_ARRAY_BUFFER, 0)`, and
            // raw user GL between draws is unobservable). Rebind the VAO's
            // shared VBO so the upcoming `streamVerticesToGPU` writes to
            // the right buffer. No `setupVertexAttribPointers` -- the VAO
            // already holds the correct attribute-to-buffer association.
            vaoGroup.vbo.bind();
        }
    }

    // Select shader to be used
    const Shader& usedShader = states.shader != nullptr ? *states.shader : GraphicsContext::getInstalledBuiltInShader();

    // Select view to be used
    const View usedView = states.view == View{} ? computeView() : states.view;

    // Update shader
    const auto usedNativeHandle = usedShader.getNativeHandle();
    const bool shaderChanged    = m_impl->cache.lastProgramId != usedNativeHandle;

    if (!m_impl->cache.enable || shaderChanged)
    {
        usedShader.bind();
        m_impl->cache.lastProgramId = usedNativeHandle;
    }
    else if (usedShader.m_textureBindingsGeneration != m_impl->cache.lastTextureBindingsGeneration)
    {
        // Same program, but a texture uniform was (re)assigned since the last
        // draw: `setUniform(loc, texture)` / `setUniform(loc, CurrentTexture)`
        // only record CPU-side state -- the GL work happens at bind time, and
        // the full `bind()` above was skipped. Re-run the texture bindings.
        usedShader.bindTextures();
    }

    m_impl->cache.lastTextureBindingsGeneration = usedShader.m_textureBindingsGeneration;

    // Apply the view
    const bool viewChanged = m_impl->cache.lastView != usedView;
    if (!m_impl->cache.enable || viewChanged)
        applyView(usedView);

    // Set the model-view-projection matrix. Always evaluated: uniform values
    // live in the shared program object, so a per-render-target "unchanged"
    // shortcut goes stale whenever another render target draws with the same
    // program (e.g. two windows using the built-in shader). `setupDrawMVP`
    // dedups the actual uploads against a per-`Shader` shadow.
    setupDrawMVP(states.transform, m_impl->cache.lastView.getTransform(), usedShader);

    // Apply the blend mode
    if (!m_impl->cache.enable || (states.blendMode != m_impl->cache.lastBlendMode))
        applyBlendMode(states.blendMode);

    // Apply the stencil mode
    if (!m_impl->cache.enable || (states.stencilMode != m_impl->cache.lastStencilMode))
        applyStencilMode(states.stencilMode);

    // Mask the color buffer off if necessary
    if (states.stencilMode.stencilOnly)
        glCheck(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));

    // Deal with texture
    setupDrawTexture(states, usedShader);

    // Update last used render states
    m_lastRenderStates = states;
}


////////////////////////////////////////////////////////////
void RenderTarget::setupDrawMVP(const Transform& renderStatesTransform, const Transform& viewTransform, const Shader& usedShader)
{
    // Compute the final draw transform
    const Transform trsfm = viewTransform * /* model-view matrix */ renderStatesTransform;

    // Upload the 2D affine transform as two vec3 rows:
    //   row0 = (a00, a01, a02)  ->  gl_Position.x = dot(row0, vec3(pos, 1))
    //   row1 = (a10, a11, a12)  ->  gl_Position.y = dot(row1, vec3(pos, 1))
    //
    // Uniform values live in the (possibly shared) program object, not in any
    // GL context or render target, so redundant uploads are avoided by
    // comparing against the per-`Shader` shadow of the last uploaded value.

    if (usedShader.m_hasBuiltInUniformMVPRow0)
    {
        if (const za::Array<float, 3> mvpRow0{trsfm.a00, trsfm.a01, trsfm.a02}; usedShader.m_lastUploadedMVPRow0 != mvpRow0)
        {
            glCheck(glUniform3fv(/* location */ 0u, /* count */ 1, mvpRow0.data())); // `za_u_mvpRow0`
            usedShader.m_lastUploadedMVPRow0 = mvpRow0;
        }
    }

    if (usedShader.m_hasBuiltInUniformMVPRow1)
    {
        if (const za::Array<float, 3> mvpRow1{trsfm.a10, trsfm.a11, trsfm.a12}; usedShader.m_lastUploadedMVPRow1 != mvpRow1)
        {
            glCheck(glUniform3fv(/* location */ 1u, /* count */ 1, mvpRow1.data())); // `za_u_mvpRow1`
            usedShader.m_lastUploadedMVPRow1 = mvpRow1;
        }
    }
}


////////////////////////////////////////////////////////////
void RenderTarget::setupDrawTexture(const RenderStates& states, const Shader& usedShader)
{
    // Select texture to be used
    const Texture& usedTexture = states.texture != nullptr ? *states.texture
                                                           : GraphicsContext::getInstalledBuiltInWhiteDotTexture();

    // If the texture is an FBO attachment, always rebind it in order to inform the OpenGL driver that we
    // want changes made to it in other contexts to be visible here as well. This saves us from having to
    // call `glFlush()` in `RenderTextureImplFBO` which can be quite costly
    //
    // See: https://www.khronos.org/opengl/wiki/Memory_Model

    // Should the texture be bound?
    const bool mustApplyTexture = !m_impl->cache.enable || usedTexture.m_fboAttachment ||
                                  usedTexture.m_cacheId != m_impl->cache.lastTextureId;

    // Bind the texture if needed
    if (mustApplyTexture)
    {
        usedTexture.bind();
        m_impl->cache.lastTextureId = usedTexture.m_cacheId;
    }

    // Upload inverse texture size if needed (hardcoded layout location `3u` for `za_u_invTextureSize`).
    // Always evaluated: the uniform value lives in the (possibly shared) program
    // object, so redundant uploads are avoided by comparing against the
    // per-`Shader` shadow of the last uploaded value (see `setupDrawMVP`).
    if (usedShader.m_hasBuiltInUniformInvTextureSize)
    {
        const auto v = 1.f / usedTexture.getSize().toVec2f();

        if (const za::Array<float, 2> invTexSize{v.x, v.y}; usedShader.m_lastUploadedInvTextureSize != invTexSize)
        {
            glCheck(glUniform2fv(/* location */ 3u, /* count */ 1, invTexSize.data()));
            usedShader.m_lastUploadedInvTextureSize = invTexSize;
        }
    }
}


////////////////////////////////////////////////////////////
void RenderTarget::cleanupDraw(const RenderStates& states)
{
    // Do not unbind the shader here, as it could be reused for the next draw call.

    // If the texture we used to draw belonged to a RenderTexture, then forcibly unbind that texture.
    // This prevents a bug where some drivers do not clear RenderTextures properly.
    if (states.texture != nullptr && states.texture->m_fboAttachment)
        unapplyTexture();

    // Mask the color buffer back on if necessary
    if (states.stencilMode.stencilOnly)
        glCheck(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

    // Re-enable the cache at the end of the draw if it was disabled
    m_impl->cache.enable = true;
}


////////////////////////////////////////////////////////////
void RenderTarget::invokePrimitiveDrawCall(const PrimitiveType type, const za::SizeT firstVertex, const za::SizeT vertexCount)
{
    m_currentDrawStats.drawCalls += 1u;
    m_currentDrawStats.drawnVertices += vertexCount;

    glCheck(glDrawArrays(/*     primitive type */ priv::primitiveTypeToOpenGLMode(type),
                         /* first vertex index */ static_cast<GLint>(firstVertex),
                         /*       vertex count */ static_cast<GLsizei>(vertexCount)));
}


////////////////////////////////////////////////////////////
void RenderTarget::invokePrimitiveDrawCallIndexed(const PrimitiveType type, const za::SizeT indexCount, const za::SizeT indexOffset)
{
    m_currentDrawStats.drawCalls += 1u;
    m_currentDrawStats.drawnVertices += indexCount;

    glCheck(glDrawElements(/* primitive type */ priv::primitiveTypeToOpenGLMode(type),
                           /*    index count */ static_cast<GLsizei>(indexCount),
                           /*     index type */ GL_UNSIGNED_INT,
                           /*   index offset */ reinterpret_cast<void*>(indexOffset * sizeof(IndexType))));
}


////////////////////////////////////////////////////////////
void RenderTarget::invokePrimitiveDrawCallIndexedBaseVertex(
    [[maybe_unused]] const PrimitiveType type,
    [[maybe_unused]] const za::SizeT     indexCount,
    [[maybe_unused]] const za::SizeT     indexOffset,
    [[maybe_unused]] const za::SizeT     vertexOffset)
{
#ifdef ZA_OPENGL_ES
    priv::errMsg("FATAL ERROR: `glDrawElementsBaseVertex` only available in OpenGL ES 3.2+ (unsupported)");
    za::abort();
#else
    m_currentDrawStats.drawCalls += 1u;
    m_currentDrawStats.drawnVertices += indexCount;

    glCheck(glDrawElementsBaseVertex(/* primitive type */ priv::primitiveTypeToOpenGLMode(type),
                                     /*    index count */ static_cast<GLsizei>(indexCount),
                                     /*     index type */ GL_UNSIGNED_INT,
                                     /*   index offset */ reinterpret_cast<void*>(indexOffset * sizeof(IndexType)),
                                     /*    base vertex */ static_cast<GLint>(vertexOffset)));
#endif
}


////////////////////////////////////////////////////////////
void RenderTarget::invokeInstancedPrimitiveDrawCall(const PrimitiveType type,
                                                    const za::SizeT     vertexOffset,
                                                    const za::SizeT     vertexCount,
                                                    const za::SizeT     instanceCount)
{
    m_currentDrawStats.drawCalls += 1u;
    m_currentDrawStats.drawnVertices += vertexCount * instanceCount;

    glCheck(glDrawArraysInstanced(/*      primitive type */ priv::primitiveTypeToOpenGLMode(type),
                                  /*       vertex offset */ static_cast<GLint>(vertexOffset),
                                  /*        vertex count */ static_cast<GLsizei>(vertexCount),
                                  /* number of instances */ static_cast<GLsizei>(instanceCount)));
}


////////////////////////////////////////////////////////////
void RenderTarget::invokeInstancedPrimitiveDrawCallIndexed(
    const PrimitiveType type,
    const za::SizeT     indexOffset,
    const za::SizeT     indexCount,
    const za::SizeT     instanceCount)
{
    m_currentDrawStats.drawCalls += 1u;
    m_currentDrawStats.drawnVertices += indexCount * instanceCount;

    glCheck(glDrawElementsInstanced(/*      primitive type */ priv::primitiveTypeToOpenGLMode(type),
                                    /*         index count */ static_cast<GLsizei>(indexCount),
                                    /*          index type */ GL_UNSIGNED_INT,
                                    /*        index offset */ reinterpret_cast<void*>(indexOffset * sizeof(IndexType)),
                                    /* number of instances */ static_cast<GLsizei>(instanceCount)));
}

} // namespace za


////////////////////////////////////////////////////////////
// Render states caching strategies
//
// * Blending mode
//   Since it overloads the == operator, we can easily check
//   whether any of the 6 blending components changed and,
//   thus, whether we need to update the blend mode.
//
// * Texture
//   Storing the pointer or OpenGL ID of the last used texture
//   is not enough; if the za::Texture instance is destroyed,
//   both the pointer and the OpenGL ID might be recycled in
//   a new texture instance. We need to use our own unique
//   identifier system to ensure consistent caching.
//
// * Shader
//   Shaders are very hard to optimize, because they have
//   parameters that can be hard (if not impossible) to track,
//   like matrices or textures. The only optimization that we
//   do is that we avoid setting a null shader if there was
//   already none for the previous draw.
//
////////////////////////////////////////////////////////////

// TODO P0: document autobatching limitations
// - shader uniform updates tracked, but require manual flush
// - texture updates not tracked in same batch (would break additive font atlases)
