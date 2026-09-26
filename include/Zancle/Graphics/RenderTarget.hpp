#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/BatchedGeometry.hpp"
#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DrawTextureSettings.hpp"
#include "Zancle/Graphics/DrawVerticesSettings.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/Priv/ShaderBase.hpp"
#include "Zancle/Graphics/Priv/ShapeDataConcept.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/Texture.hpp" // needed for `hasGenerationMismatch` to access `Texture::m_destructiveGeneration`

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/FunctionRef.hpp"
#include "Zancle/Vocabulary/InPlacePImpl.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class CPUDrawableBatch;
class Font;
class FontFace;
class GlyphMappedText;
class PersistentGPUDrawableBatch;
class Shader;
class Shape;
class Text;
class VAOHandle;
class VertexBuffer;

struct BlendMode;
struct DrawIndexedVerticesSettings;
struct DrawInstancedIndexedVerticesSettings;
struct DrawInstancedVerticesSettings;
struct DrawPersistentMappedIndexedVerticesSettings;
struct DrawQuadsSettings;
struct GLElementBufferObject;
struct GLVAOGroup;
struct GLVertexBufferObject;
struct GlyphMappedTextData;
struct GlyphMapping;
struct InstanceAttributeBinder;
struct Sprite;
struct StencilMode;
struct StencilValue;
struct TextData;
struct Transform;
struct Vertex;
struct View;

} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Base class for all render targets (window, texture, etc...)
///
////////////////////////////////////////////////////////////
class [[nodiscard]] ZA_GRAPHICS_API RenderTarget
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Struct to hold the statistics of a render target
    ///
    ////////////////////////////////////////////////////////////
    struct DrawStatistics
    {
        unsigned int drawCalls{0u};     //!< Number of draw calls
        za::SizeT    drawnVertices{0u}; //!< Number of vertices drawn
    };

    ////////////////////////////////////////////////////////////
    enum : za::SizeT
    {
        drawQuadsMaxQuadsPerCall = 65'536u, //!< Max quad count for `drawQuads` (precomputed-index-buffer capacity).
        drawQuadsMaxVerticesPerCall = drawQuadsMaxQuadsPerCall * 4u, //!< Max vertex count for `drawQuads` (multiple of 4).
    };

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    virtual ~RenderTarget();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    RenderTarget(const RenderTarget&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    RenderTarget& operator=(const RenderTarget&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    RenderTarget(RenderTarget&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    RenderTarget& operator=(RenderTarget&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Clear the entire target with a single color
    ///
    /// This function is usually called once every frame,
    /// to clear the previous contents of the target.
    ///
    /// \param color Fill color to use to clear the render target
    ///
    ////////////////////////////////////////////////////////////
    void clear(Color color = Color::Black);

    ////////////////////////////////////////////////////////////
    /// \brief Clear the stencil buffer to a specific value
    ///
    /// The specified value is truncated to the bit width of
    /// the current stencil buffer.
    ///
    /// \param stencilValue Stencil value to clear to
    ///
    ////////////////////////////////////////////////////////////
    void clearStencil(StencilValue stencilValue);

    ////////////////////////////////////////////////////////////
    /// \brief Clear the entire target with a single color and stencil value
    ///
    /// The specified stencil value is truncated to the bit
    /// width of the current stencil buffer.
    ///
    /// \param color        Fill color to use to clear the render target
    /// \param stencilValue Stencil value to clear to
    ///
    ////////////////////////////////////////////////////////////
    void clear(Color color, StencilValue stencilValue);

    ////////////////////////////////////////////////////////////
    /// \brief Auto-batching strategy used by the render target
    ///
    /// Auto-batching is a performance optimization that
    /// transparently coalesces consecutive compatible draw calls
    /// into a single GPU submission, drastically reducing CPU
    /// overhead for scenes with many small drawables (sprites,
    /// shapes, text characters, ...).
    ///
    /// The mode controls *where* the intermediate vertex buffer
    /// lives:
    /// \li `Disabled`   -- no batching; each draw call goes
    ///                     directly to the GPU.
    /// \li `CPUStorage` -- batching enabled, vertices accumulate
    ///                     in a CPU-side buffer and are uploaded
    ///                     on flush. Best general default.
    /// \li `GPUStorage` -- batching enabled, vertices stream
    ///                     directly into a persistent-mapped GPU
    ///                     buffer. Falls back to `CPUStorage`
    ///                     when persistent mapping is unavailable
    ///                     (e.g. on platforms without
    ///                     `GL_ARB_buffer_storage`).
    ///
    ////////////////////////////////////////////////////////////
    enum class [[nodiscard]] AutoBatchMode : unsigned char
    {
        Disabled,   //!< No auto-batching; each draw call is submitted directly
        CPUStorage, //!< Auto-batching enabled, intermediate vertices stored in CPU memory
        GPUStorage, //!< Auto-batching enabled, intermediate vertices stored in a persistent-mapped GPU buffer (CPU fallback)
    };

    ////////////////////////////////////////////////////////////
    /// \brief Set the current auto-batching mode
    ///
    /// Auto-batching is a performance optimization that groups
    /// draw calls together to reduce the number of state changes
    /// and improve rendering performance. When enabled, the
    /// render target will automatically batch draw calls
    /// together when possible, reducing the overhead of
    /// individual draw calls.
    ///
    /// \param mode The auto-batching mode to set
    ///
    /// \see `getAutoBatchMode`
    ///
    ////////////////////////////////////////////////////////////
    void setAutoBatchMode(AutoBatchMode mode);

    ////////////////////////////////////////////////////////////
    /// \brief Get the current auto-batching mode
    ///
    /// \return The current auto-batching mode
    ///
    /// \see `setAutoBatchMode`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] AutoBatchMode getAutoBatchMode() const;

    ////////////////////////////////////////////////////////////
    /// \brief Set the vertex threshold for auto-batching
    ///
    /// When the current number of vertices in the batch exceeds
    /// this threshold, the batch will be flushed and a new one
    /// will be started. This can help to control the size of
    /// the batches and optimize performance.
    ///
    /// \param threshold The vertex threshold for auto-batching
    ///
    /// \see `getAutoBatchVertexThreshold`
    ///
    ////////////////////////////////////////////////////////////
    void setAutoBatchVertexThreshold(za::SizeT threshold);

    ////////////////////////////////////////////////////////////
    /// \brief Get the vertex threshold for auto-batching
    ///
    /// \return The vertex threshold for auto-batching
    ///
    /// \see `setAutoBatchVertexThreshold`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::SizeT getAutoBatchVertexThreshold() const;

    ////////////////////////////////////////////////////////////
    /// \brief Reserve space in the current auto-batch for triangles
    ///
    /// Ensures that the currently active auto-batch has enough
    /// storage for `triangleCount` triangles.
    ///
    /// \param triangleCount Number of triangles to reserve for
    ///
    ////////////////////////////////////////////////////////////
    void reserveAutoBatchTriangles(za::SizeT triangleCount);

    ////////////////////////////////////////////////////////////
    /// \brief Reserve space in the current auto-batch for quads
    ///
    /// Ensures that the currently active auto-batch has enough
    /// storage for `quadCount` quads.
    ///
    /// \param quadCount Number of quads to reserve for
    ///
    ////////////////////////////////////////////////////////////
    void reserveAutoBatchQuads(za::SizeT quadCount);

    ////////////////////////////////////////////////////////////
    /// \brief Draw a texture to the render target with default parameters
    ///
    /// The full texture is drawn at position `{0.f, 0.f}` with the
    /// default origin, rotation, scale, and color (opaque white).
    /// This is the simplest way to put a texture on screen, but
    /// also the least flexible -- use the `DrawTextureSettings`
    /// overload (or `za::Sprite`) for any further control.
    ///
    /// \param texture Texture to draw
    /// \param states  Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    void draw(const Texture& texture, RenderStates states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw a texture to the render target with custom parameters
    ///
    /// Uses `params` to control the position, rotation, scale,
    /// origin, sub-rectangle, and tint of the rendered texture.
    /// This is the recommended one-shot path; for repeated drawing
    /// of the same configuration, use `za::Sprite`.
    ///
    /// \param texture Texture to draw
    /// \param params  Position, rotation, scale, sub-rect, and tint
    /// \param states  Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    void draw(const Texture& texture, const DrawTextureSettings& params, RenderStates states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw a sprite to the render target
    ///
    /// Zancle sprites do not own their texture: the texture must
    /// be supplied at draw time, either inline (preferred for
    /// one-offs) or through `RenderStates::texture`.
    ///
    /// \param sprite Sprite to draw
    /// \param states Render states to use for drawing (must reference the sprite's texture)
    ///
    ////////////////////////////////////////////////////////////
    void draw(const Sprite& sprite, const RenderStates& states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw a shape to the render target
    ///
    /// As with `za::Sprite`, the texture (if any) is passed at
    /// draw time via `RenderStates::texture`.
    ///
    /// \param shape  Shape to draw
    /// \param states Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    void draw(const Shape& shape, RenderStates states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw a text object to the render target
    ///
    /// `text` carries its own font reference, so no extra
    /// resource needs to be passed in `states`.
    ///
    /// \param text   Text to draw
    /// \param states Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    void draw(const Text& text, RenderStates states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw a `za::GlyphMappedText` to the render target
    ///
    /// Lower-level text drawing path: glyphs are precomputed in
    /// a `za::GlyphMapping` and reused across draws, which is
    /// significantly faster than `za::Text` for static or
    /// frequently-redrawn strings.
    ///
    /// \param text   Pre-mapped glyph text to draw
    /// \param states Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    void draw(const GlyphMappedText& text, RenderStates states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw the contents of a CPU-side drawable batch
    ///
    /// The vertices accumulated in `drawableBatch` are uploaded
    /// (if needed) and drawn in one go. This is the high-level
    /// path for manual batching of many small drawables.
    ///
    /// \param drawableBatch Batch whose contents should be drawn
    /// \param states        Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    void draw(const CPUDrawableBatch& drawableBatch, const RenderStates& states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw the contents of a persistent GPU drawable batch
    ///
    /// The same as the `CPUDrawableBatch` overload, but the
    /// vertex data already lives in a persistent-mapped GPU
    /// buffer owned by `drawableBatch`. No CPU-to-GPU upload
    /// happens at draw time.
    ///
    /// \param drawableBatch Batch whose contents should be drawn
    /// \param states        Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    void draw(const PersistentGPUDrawableBatch& drawableBatch, RenderStates states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw primitives defined by a vertex buffer
    ///
    /// \param vertexBuffer Vertex buffer
    /// \param states       Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    void draw(const VertexBuffer& vertexBuffer, const RenderStates& states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw primitives defined by a vertex buffer
    ///
    /// \param vertexBuffer Vertex buffer
    /// \param firstVertex  Index of the first vertex to render
    /// \param vertexCount  Number of vertices to render
    /// \param states       Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    void draw(const VertexBuffer& vertexBuffer, za::SizeT firstVertex, za::SizeT vertexCount, const RenderStates& states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw a shape from its plain `*ShapeData` description
    ///
    /// This is the "stateless" shape drawing path: instead of
    /// constructing a full `za::Shape`, you pass any of the
    /// `za::CircleShapeData`, `za::RectangleShapeData`, etc.
    /// structs and the renderer tessellates and draws it in one
    /// call.
    ///
    /// \param shapeData Plain shape description
    /// \param states    Render states to use for drawing
    ///
    /// \return Span pointing to the generated vertices inside the
    ///         current batch. The span is invalidated by the next
    ///         draw call -- copy it out if you need to keep it.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry draw(const priv::ShapeDataConcept auto& shapeData, const RenderStates& states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw text from a font and a plain `TextData` description
    ///
    /// Stateless counterpart to `draw(const Text&, ...)`. Useful
    /// when the text properties change every frame and the
    /// overhead of (re)building a `za::Text` would be wasted.
    ///
    /// \param font     Font used to rasterize glyphs
    /// \param textData Text description (string, character size, style, ...)
    /// \param states   Render states to use for drawing
    ///
    /// \return Span pointing to the generated vertices inside the
    ///         current batch. The span is invalidated by the next
    ///         draw call -- copy it out if you need to keep it.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry draw(const Font& font, const TextData& textData, RenderStates states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw text using a precomputed glyph mapping
    ///
    /// Lowest-level text drawing path: the glyph layout has
    /// already been computed and cached in `glyphMapping`, so
    /// each draw is essentially a vertex generation pass. This
    /// is the fastest path for static or frequently-redrawn text.
    ///
    /// \param fontFace     Font face providing glyph metrics
    /// \param glyphMapping Precomputed glyph layout
    /// \param textData     Text description without glyph-derived fields
    /// \param states       Render states to use for drawing
    ///
    /// \return Span pointing to the generated vertices inside the
    ///         current batch. The span is invalidated by the next
    ///         draw call -- copy it out if you need to keep it.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry draw(const FontFace&            fontFace,
                         const GlyphMapping&        glyphMapping,
                         const GlyphMappedTextData& textData,
                         const RenderStates&        states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw primitives defined by an array of vertices
    ///
    /// \param settings Draw settings
    ///
    ////////////////////////////////////////////////////////////
    void drawVertices(const DrawVerticesSettings& settings, const RenderStates& states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw primitives defined by an array of indices and vertices
    ///
    /// \param settings Draw settings
    ///
    ////////////////////////////////////////////////////////////
    void drawIndexedVertices(const DrawIndexedVerticesSettings& settings, const RenderStates& states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw quads defined by an array of vertices and precomputed quad indices
    ///
    /// Each consecutive group of 4 vertices in `settings.vertexSpan` is
    /// interpreted as one quad and expanded into two triangles by a
    /// precomputed internal index buffer (pattern `0,1,2,1,2,3` per quad).
    /// Vertex layout per quad must therefore place the shared diagonal
    /// between the 2nd and 3rd vertex.
    ///
    /// \param settings Draw settings. `settings.vertexSpan.size()` must
    ///                 be a multiple of 4. Draws larger than
    ///                 `drawQuadsMaxVerticesPerCall` (i.e. more than
    ///                 `drawQuadsMaxQuadsPerCall` quads) are internally
    ///                 split into multiple draw calls.
    ///
    /// \see `drawQuadsMaxQuadsPerCall`, `drawQuadsMaxVerticesPerCall`,
    ///      `drawIndexedVertices`
    ///
    ////////////////////////////////////////////////////////////
    void drawQuads(const DrawQuadsSettings& settings, const RenderStates& states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw primitives defined by a persistent mapped buffer and indices
    ///
    /// \param settings Draw settings
    ///
    ////////////////////////////////////////////////////////////
    void drawPersistentMappedIndexedVertices(const DrawPersistentMappedIndexedVerticesSettings& settings,
                                             const RenderStates&                                states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw an instanced primitive (same vertices replayed N times)
    ///
    /// Issues a single instanced draw call. `setupFn` is invoked
    /// once before the call to bind the per-instance attribute
    /// streams onto the VAO carried by `settings`. See
    /// `za::InstanceAttributeBinder` for the available attribute
    /// types.
    ///
    /// \param settings Instanced draw parameters (vertices, VAO, instance count, primitive type)
    /// \param setupFn  Callback that binds per-instance attribute streams
    /// \param states   Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    void drawInstancedVertices(const DrawInstancedVerticesSettings&            settings,
                               za::FunctionRef<void(InstanceAttributeBinder&)> setupFn,
                               const RenderStates&                             states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Draw an instanced and indexed primitive
    ///
    /// Combines indexing (vertex reuse) with instancing
    /// (geometry reuse). `setupFn` is invoked once before the
    /// call to bind the per-instance attribute streams onto the
    /// VAO carried by `settings`.
    ///
    /// \param settings Instanced indexed draw parameters
    /// \param setupFn  Callback that binds per-instance attribute streams
    /// \param states   Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    void drawInstancedIndexedVertices(const DrawInstancedIndexedVerticesSettings&     settings,
                                      za::FunctionRef<void(InstanceAttributeBinder&)> setupFn,
                                      const RenderStates&                             states = {});

    ////////////////////////////////////////////////////////////
    /// \brief Return the size of the rendering region of the target
    ///
    /// \return Size in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] virtual Vec2u getSize() const = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Tell if the render target will use sRGB encoding when drawing on it
    ///
    /// The sRGB state is a fact of the underlying target type
    /// (always `false` for `RenderWindow`; set at creation time for
    /// `RenderTexture`) and is captured once at construction.
    ///
    /// \return `true` if the render target use sRGB encoding, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] bool isSrgb() const
    {
        return m_isSrgb;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Activate or deactivate the render target for rendering
    ///
    /// This function makes the render target's context current for
    /// future OpenGL rendering operations (so you shouldn't care
    /// about it if you're not doing direct OpenGL stuff).
    /// A render target's context is active only on the current thread,
    /// if you want to make it active on another thread you have
    /// to deactivate it on the previous thread first if it was active.
    /// Only one context can be current in a thread, so if you
    /// want to draw OpenGL geometry to another render target
    /// don't forget to activate it again. Activating a render
    /// target will automatically deactivate the previously active
    /// context (if any).
    ///
    /// \param active `true` to activate, `false` to deactivate
    ///
    /// \return `true` if operation was successful, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] virtual bool setActive(bool active = true);

    ////////////////////////////////////////////////////////////
    /// \brief Compute a default view that exactly matches the target's current size
    ///
    /// Equivalent to `za::View::fromScreenSize(getSize())`. Useful
    /// after a resize event, when you want a fresh view that maps
    /// world units to pixels 1:1.
    ///
    /// \return View covering the entire render target
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] View computeView() const;

    ////////////////////////////////////////////////////////////
    /// \brief Reset the internal OpenGL states so that the target is ready for drawing
    ///
    /// This function can be used when you mix Zancle drawing
    /// and direct OpenGL rendering. It makes sure that all OpenGL
    /// states needed by Zancle are set, so that subsequent `draw()`
    /// calls will work as expected.
    ///
    /// Example:
    /// \code
    /// // OpenGL code here...
    /// glPushAttrib(...);
    /// window.resetGLStates();
    /// window.draw(...);
    /// window.draw(...);
    /// glPopAttrib(...);
    /// // OpenGL code here...
    /// \endcode
    ///
    ////////////////////////////////////////////////////////////
    void resetGLStates();

    ////////////////////////////////////////////////////////////
    /// \brief Draws the currently autobatched drawables (if any)
    ///
    /// \return Statistics about the draw calls made since the last `display()` (the current frame so far)
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::cold]] RenderTarget::DrawStatistics flush();

    ////////////////////////////////////////////////////////////
    /// \brief Flush queued GPU commands to the driver (`glFlush`)
    ///
    /// Hints the driver to start processing previously submitted
    /// commands as soon as possible, but does **not** wait for
    /// them to complete. Use sparingly: most code does not need
    /// this, since the renderer flushes implicitly at frame
    /// boundaries.
    ///
    ////////////////////////////////////////////////////////////
    void invokeGlFlush();

    ////////////////////////////////////////////////////////////
    /// \brief Block until all queued GPU commands have completed (`glFinish`)
    ///
    /// Strictly stronger than `invokeGlFlush`: this returns
    /// only after the GPU has actually finished executing every
    /// previously submitted command. Useful for benchmarking and
    /// for synchronizing CPU work that depends on the result of
    /// previous draw calls. Avoid in normal rendering paths --
    /// it serializes CPU and GPU and tanks frame pacing.
    ///
    ////////////////////////////////////////////////////////////
    void invokeGlFinish();

    ////////////////////////////////////////////////////////////
    /// \brief RAII helper bound to a `RenderStates` snapshot for chained drawing
    ///
    /// Returned by `withRenderStates` and `withLockedRenderStates`.
    /// Calling `draw`/`drawAll` on this context forwards the
    /// drawables to the parent `RenderTarget` while implicitly
    /// reusing the captured `RenderStates`. The "locked" variant
    /// additionally asserts (in debug builds) that no nested call
    /// changes those render states behind your back.
    ///
    ////////////////////////////////////////////////////////////
    class [[nodiscard]] WithRenderStatesContext
    {
    private:
        ////////////////////////////////////////////////////////////
        RenderTarget* const m_rt;
        const RenderStates  m_states;
        const bool          m_locked;

    public:
        ////////////////////////////////////////////////////////////
        WithRenderStatesContext(RenderTarget& rt, const RenderStates& states, bool locked);
        ~WithRenderStatesContext();

        ////////////////////////////////////////////////////////////
        WithRenderStatesContext(const WithRenderStatesContext&)            = delete;
        WithRenderStatesContext& operator=(const WithRenderStatesContext&) = delete;

        ////////////////////////////////////////////////////////////
        WithRenderStatesContext(WithRenderStatesContext&&)            = delete;
        WithRenderStatesContext& operator=(WithRenderStatesContext&&) = delete;

        ////////////////////////////////////////////////////////////
        [[gnu::always_inline]] const WithRenderStatesContext& draw(const auto& drawable) const
        {
            m_rt->draw(drawable, m_states);
            return *this;
        }

        ////////////////////////////////////////////////////////////
        [[gnu::always_inline]] const WithRenderStatesContext& drawAll(const auto&... drawables) const
        {
            (..., draw(drawables));
            return *this;
        }

        ////////////////////////////////////////////////////////////
        // NOLINTNEXTLINE(modernize-use-nodiscard)
        [[gnu::always_inline]] const WithRenderStatesContext& draw(const Texture& texture, const DrawTextureSettings& params) const
        {
            m_rt->draw(texture, params, m_states);
            return *this;
        }

        ////////////////////////////////////////////////////////////
        // NOLINTNEXTLINE(modernize-use-nodiscard)
        [[gnu::always_inline]] const WithRenderStatesContext& draw(const Font& font, const TextData& textData) const
        {
            m_rt->draw(font, textData, m_states);
            return *this;
        }

        ////////////////////////////////////////////////////////////
        // NOLINTNEXTLINE(modernize-use-nodiscard)
        [[gnu::always_inline]] const WithRenderStatesContext& draw(const FontFace&            fontFace,
                                                                   const GlyphMapping&        glyphMapping,
                                                                   const GlyphMappedTextData& textData) const
        {
            m_rt->draw(fontFace, glyphMapping, textData, m_states);
            return *this;
        }
    };

    ////////////////////////////////////////////////////////////
    friend class WithRenderStatesContext;

    ////////////////////////////////////////////////////////////
    /// \brief Begin a chained-drawing context with a fixed `RenderStates` snapshot
    ///
    /// Returns a `WithRenderStatesContext` whose `draw` calls
    /// reuse `states`. Useful for drawing many objects with
    /// identical render states without repeating them at every
    /// call site.
    ///
    /// \param states Render states to capture
    ///
    /// \return Chained drawing context (do not store across frames)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] WithRenderStatesContext withRenderStates(const RenderStates& states)
    {
        return {*this, states, /* locked */ false};
    }

    ////////////////////////////////////////////////////////////
    /// \brief Like `withRenderStates`, but asserts the snapshot stays unchanged
    ///
    /// In debug builds, drawing through the returned context
    /// triggers an assertion if any nested code path changes the
    /// captured `RenderStates`. Use this to enforce that a hot
    /// rendering loop never accidentally breaks its batch.
    ///
    /// \param states Render states to capture and lock
    ///
    /// \return Chained drawing context with state-mutation guard
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] WithRenderStatesContext withLockedRenderStates(const RenderStates& states)
    {
        return {*this, states, /* locked */ true};
    }

protected:
    ////////////////////////////////////////////////////////////
    /// \brief Read the cached `GL_SCISSOR_TEST` state (no GL query)
    ///
    /// Returns the value tracked by the internal state cache,
    /// avoiding a synchronous `glGetBooleanv` round-trip. The
    /// cache is kept in sync by every code path inside
    /// `RenderTarget` that touches scissor state.
    ///
    /// \warning If the cache has not yet been initialized
    /// (`resetGLStates` has not run on this target on this
    /// context), this returns the default-constructed value
    /// (`false`) rather than the actual GL state. Callers in
    /// hot paths should ensure the target has been activated
    /// at least once before relying on this.
    ///
    /// \warning If the user manipulates `GL_SCISSOR_TEST`
    /// directly via raw OpenGL outside of `RenderTarget`
    /// without subsequently calling `resetGLStates`, this
    /// value will drift from the real GL state. Same caveat
    /// as for every other piece of cached state on
    /// `RenderTarget` (blend mode, stencil, etc.).
    ///
    /// \return Cached scissor-test enable state
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isScissorEnabledCached() const;

    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    /// Protected: instances of `RenderTarget` are always created
    /// through one of the concrete subclasses (`za::RenderWindow`,
    /// `za::RenderTexture`).
    ///
    /// \param isSrgb Whether the target uses sRGB encoding (captured once)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit RenderTarget(bool isSrgb = false);

    ////////////////////////////////////////////////////////////
    /// \brief Mark the end of a frame (called by `display`)
    ///
    /// Resets the per-frame draw statistics and rotates the
    /// persistent GPU auto-batch to its next frame state. Must be
    /// called with the target's GL context active, after flushing.
    ///
    ////////////////////////////////////////////////////////////
    void syncGPUEndFrame();

private:
    ////////////////////////////////////////////////////////////
    /// \brief Prepare the render target for drawing
    ///
    /// Activates the target's context and ensures the renderer's
    /// OpenGL state matches what the next draw call expects. Not
    /// needed if `clear` has already been called this frame, since
    /// `clear` performs the same setup.
    ///
    /// \return `true` on success, `false` if the target could not be activated
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool prepare();

    ////////////////////////////////////////////////////////////
    /// \brief Immediately draw primitives defined by an array of vertices
    ///
    /// Will result in an OpenGL draw call.
    /// Does not flush any batch in-flight.
    ///
    /// \param settings Draw settings
    ///
    ////////////////////////////////////////////////////////////
    void immediateDrawVertices(const DrawVerticesSettings& settings, const RenderStates& states);

    ////////////////////////////////////////////////////////////
    /// \brief Immediately draw primitives defined by an array of indices and vertices
    ///
    /// Will result in an OpenGL draw call.
    /// Does not flush any batch in-flight.
    ///
    /// \param settings Draw settings
    ///
    ////////////////////////////////////////////////////////////
    void immediateDrawIndexedVertices(const DrawIndexedVerticesSettings& settings, const RenderStates& states);

    ////////////////////////////////////////////////////////////
    /// \brief Immediately draw primitives defined by a persistent mapped buffer and indices
    ///
    /// Will result in an OpenGL draw call.
    /// Does not flush any batch in-flight.
    ///
    /// \param settings Draw settings
    ///
    ////////////////////////////////////////////////////////////
    void immediateDrawPersistentMappedIndexedVertices(const DrawPersistentMappedIndexedVerticesSettings& settings,
                                                      const RenderStates&                                states);

    ////////////////////////////////////////////////////////////
    /// \brief Immediately draw a CPU drawable batch
    ///
    /// Submits the batch directly without going through any
    /// auto-batching path. Internal helper used by the public
    /// `draw(const CPUDrawableBatch&, ...)` overload.
    ///
    ////////////////////////////////////////////////////////////
    void immediateDrawDrawableBatch(const CPUDrawableBatch& drawableBatch, RenderStates states);

    ////////////////////////////////////////////////////////////
    /// \brief Immediately draw an instanced primitive
    ///
    /// Will result in an OpenGL `glDrawArraysInstanced` call.
    /// Does not flush any batch in flight.
    ///
    ////////////////////////////////////////////////////////////
    void immediateDrawInstancedVertices(const DrawInstancedVerticesSettings&            settings,
                                        za::FunctionRef<void(InstanceAttributeBinder&)> setupFn,
                                        const RenderStates&                             states);

    ////////////////////////////////////////////////////////////
    /// \brief Immediately draw an instanced and indexed primitive
    ///
    /// Will result in an OpenGL `glDrawElementsInstanced` call.
    /// Does not flush any batch in flight.
    ///
    ////////////////////////////////////////////////////////////
    void immediateDrawInstancedIndexedVertices(const DrawInstancedIndexedVerticesSettings&     settings,
                                               za::FunctionRef<void(InstanceAttributeBinder&)> setupFn,
                                               const RenderStates&                             states);

    ////////////////////////////////////////////////////////////
    /// \brief Check whether shader/texture generation counters diverge from cached values
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] bool hasGenerationMismatch(const RenderStates& states) const
    {
        return (states.shader != nullptr && states.shader->m_uniformGeneration != m_lastShaderGeneration) ||
               (states.texture != nullptr && states.texture->m_destructiveGeneration != m_lastTextureGeneration);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Store the current shader/texture generation counters
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void updateCachedGenerations(const RenderStates& states)
    {
        m_lastShaderGeneration  = states.shader != nullptr ? states.shader->m_uniformGeneration : za::U32{0};
        m_lastTextureGeneration = states.texture != nullptr ? states.texture->m_destructiveGeneration : za::U32{0};
    }

    ////////////////////////////////////////////////////////////
    /// \brief Flush the auto-batch if `states` differ or it has grown past the threshold
    ///
    /// Internal helper invoked by every public `draw` overload to
    /// decide whether the in-flight batch must be sent to the GPU
    /// before the new draw can be appended to it.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void flushIfNeeded(const RenderStates& states)
    {
        if (m_isStateLocked)
        {
            ZA_ASSERT(m_lastRenderStates == states && !hasGenerationMismatch(states) &&
                      "State mutation detected while inside a 'withLockedRenderStates' context!\n If you are "
                      "drawing Text, Shapes, or Sprites, you must explicitly bind their Texture to the context "
                      "upfront.");

            if (m_numAutoBatchVertices >= m_autoBatchVertexThreshold)
                flush();
        }
        else
        {
            if (m_numAutoBatchVertices >= m_autoBatchVertexThreshold || m_lastRenderStates != states ||
                hasGenerationMismatch(states))
            {
                flush();

                m_lastRenderStates = states;
                updateCachedGenerations(states);
            }
        }
    }

    ////////////////////////////////////////////////////////////
    /// \brief Backend implementation of `resetGLStates`
    ///
    /// Performs the actual OpenGL state restoration; the public
    /// wrapper additionally takes care of flushing the auto-batch
    /// before invoking it.
    ///
    ////////////////////////////////////////////////////////////
    void resetGLStatesImpl();

    ////////////////////////////////////////////////////////////
    /// \brief Apply a view
    ///
    ////////////////////////////////////////////////////////////
    void applyView(const View& view);

    ////////////////////////////////////////////////////////////
    /// \brief Apply a new blending mode
    ///
    /// \param mode Blending mode to apply
    ///
    ////////////////////////////////////////////////////////////
    void applyBlendMode(const BlendMode& mode);

    ////////////////////////////////////////////////////////////
    /// \brief Apply a new stencil mode
    ///
    /// \param mode Stencil mode to apply
    ///
    ////////////////////////////////////////////////////////////
    void applyStencilMode(const StencilMode& mode);

    ////////////////////////////////////////////////////////////
    /// \brief Unbind any bound texture
    ///
    ////////////////////////////////////////////////////////////
    void unapplyTexture();

    ////////////////////////////////////////////////////////////
    /// \brief Internal RAII helper that scopes a draw call
    ///
    /// Owns the lifetime of the per-draw setup performed by
    /// `setupDraw` and the matching `cleanupDraw` (state save,
    /// VAO/program/uniform setup, ...). Implementation lives in
    /// the `.cpp` file.
    ///
    ////////////////////////////////////////////////////////////
    struct DrawGuard;
    friend DrawGuard;

    ////////////////////////////////////////////////////////////
    /// \brief Setup environment for drawing
    ///
    /// \param states Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    void setupDraw(const GLVAOGroup& vaoGroup, const RenderStates& states);

    ////////////////////////////////////////////////////////////
    /// \brief Setup environment for drawing: MVP matrix
    ///
    /// Uploads are deduplicated against `usedShader`'s shadow of the last
    /// uploaded values, since uniform values live in the (possibly shared)
    /// program object rather than in this render target.
    ///
    /// \param renderStatesTransform Transform to use for the MVP matrix
    /// \param viewTransform         View transform to use for the MVP matrix
    /// \param usedShader            Shader whose built-in MVP uniforms are updated
    ///
    ////////////////////////////////////////////////////////////
    void setupDrawMVP(const Transform& renderStatesTransform, const Transform& viewTransform, const Shader& usedShader);

    ////////////////////////////////////////////////////////////
    /// \brief Setup environment for drawing: texture
    ///
    /// \param states     Render states to use for drawing
    /// \param usedShader Shader whose built-in `za_u_invTextureSize` uniform is updated
    ///
    ////////////////////////////////////////////////////////////
    void setupDrawTexture(const RenderStates& states, const Shader& usedShader);

    ////////////////////////////////////////////////////////////
    /// \brief Clean up environment after drawing
    ///
    /// \param states Render states used for drawing
    ///
    ////////////////////////////////////////////////////////////
    void cleanupDraw(const RenderStates& states);

    ////////////////////////////////////////////////////////////
    /// \brief Invoke primitive draw call: non-indexed
    ///
    /// Immediately executes an OpenGL draw call.
    /// This function is not intended to be used directly.
    /// Does not flush any batch in-flight.
    ///
    /// \param type        Type of primitives to draw
    /// \param firstVertex Index of the first vertex to use when drawing
    /// \param vertexCount Number of vertices to use when drawing
    ///
    ////////////////////////////////////////////////////////////
    void invokePrimitiveDrawCall(PrimitiveType type, za::SizeT firstVertex, za::SizeT vertexCount);

    ////////////////////////////////////////////////////////////
    /// \brief Invoke primitive draw call: indexed
    ///
    /// Immediately executes an OpenGL draw call.
    /// This function is not intended to be used directly.
    /// Does not flush any batch in-flight.
    ///
    /// \param type        Type of primitives to draw
    /// \param indexCount  Number of indices to use when drawing
    /// \param indexOffset Offset of the first index to use when drawing
    ///
    ////////////////////////////////////////////////////////////
    void invokePrimitiveDrawCallIndexed(PrimitiveType type, za::SizeT indexCount, za::SizeT indexOffset);

    ////////////////////////////////////////////////////////////
    /// \brief Invoke primitive draw call: indexed with base vertex (not supported on OpenGL ES 3.1)
    ///
    /// Immediately executes an OpenGL draw call.
    /// This function is not intended to be used directly.
    /// Does not flush any batch in-flight.
    ///
    /// \param type         Type of primitives to draw
    /// \param indexCount   Number of indices to use when drawing
    /// \param indexOffset  Offset of the first index to use when drawing
    /// \param vertexOffset Offset of the first vertex to use when drawing
    ///
    ////////////////////////////////////////////////////////////
    void invokePrimitiveDrawCallIndexedBaseVertex(PrimitiveType type,
                                                  za::SizeT     indexCount,
                                                  za::SizeT     indexOffset,
                                                  za::SizeT     vertexOffset);

    ////////////////////////////////////////////////////////////
    /// \brief Invoke primitive draw call: instanced, non-indexed
    ///
    /// Immediately executes an OpenGL draw call.
    /// This function is not intended to be used directly.
    /// Does not flush any batch in-flight.
    ///
    /// \param type          Type of primitives to draw
    /// \param vertexOffset  Offset of the first vertex to use when drawing
    /// \param vertexCount   Number of vertices to use when drawing
    /// \param instanceCount Number of instances to draw
    ///
    ////////////////////////////////////////////////////////////
    void invokeInstancedPrimitiveDrawCall(PrimitiveType type, za::SizeT vertexOffset, za::SizeT vertexCount, za::SizeT instanceCount);

    ////////////////////////////////////////////////////////////
    /// \brief Invoke primitive draw call: instanced, indexed
    ///
    /// Immediately executes an OpenGL draw call.
    /// This function is not intended to be used directly.
    /// Does not flush any batch in-flight.
    ///
    /// \param type          Type of primitives to draw
    /// \param indexOffset   Offset of the first index to use when drawing
    /// \param indexCount    Number of indices to use when drawing
    /// \param instanceCount Number of instances to draw
    ///
    ////////////////////////////////////////////////////////////
    void invokeInstancedPrimitiveDrawCallIndexed(PrimitiveType type,
                                                 za::SizeT     indexOffset,
                                                 za::SizeT     indexCount,
                                                 za::SizeT     instanceCount);

    ////////////////////////////////////////////////////////////
    /// \brief Forward a draw description to the active auto-batch
    ///
    /// Internal entry point shared by every public `draw` overload
    /// that wants to participate in auto-batching. The variadic
    /// argument pack is forwarded to the matching `add(...)`
    /// overload of the underlying drawable batch.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline auto addToAutoBatch(auto&&... xs);

    ////////////////////////////////////////////////////////////
    /// \brief Execute a function with the current auto-batch as the active target
    ///
    /// If OpenGL ES is detected, the CPU drawable batch is used.
    /// Otherwise, the used batch depends on `m_autoBatchMode`.
    ///
    /// Asserts that autobatching is enabled.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline decltype(auto) withCurrentAutobatch(auto&& f);

public:
    ////////////////////////////////////////////////////////////
    /// \brief Draw primitives defined by a contiguous container of vertices
    ///
    /// \tparam ContiguousVertexRange Type of the contiguous container,
    ///         must support `.data()` and `.size()` operations.
    ///
    /// \param vertices Reference to the contiguous vertex container
    /// \param type     Type of primitives to draw
    /// \param states   Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    template <typename ContiguousVertexRange>
    [[gnu::always_inline]] void draw(const ContiguousVertexRange& vertices, PrimitiveType type, const RenderStates& states = {})
        requires(requires { drawVertices(DrawVerticesSettings{.vertexSpan = vertices, .primitiveType = type}, states); })
    {
        drawVertices(
            DrawVerticesSettings{
                .vertexSpan    = vertices,
                .primitiveType = type,
            },
            states);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Draw primitives defined by a C-style array of vertices
    ///
    /// \param vertices Reference to the C-style vertex array
    /// \param type     Type of primitives to draw
    /// \param states   Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    template <za::SizeT N>
    [[gnu::always_inline]] void draw(const Vertex (&vertices)[N], PrimitiveType type, const RenderStates& states = {})
    {
        drawVertices(
            DrawVerticesSettings{
                .vertexSpan    = vertices,
                .primitiveType = type,
            },
            states);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Draw a drawable object to the render target
    ///
    /// \tparam DrawableObject Type of the drawable object,
    ///         must support `draw(RenderTarget&, RenderStates&)` method.
    ///
    /// \param drawable Object to draw
    /// \param states   Render states to use for drawing
    ///
    ////////////////////////////////////////////////////////////
    template <typename DrawableObject>
    [[gnu::always_inline]] void draw(const DrawableObject& drawableObject, const RenderStates& states = {})
        requires(requires { drawableObject.draw(*this, states); })
    {
        flushIfNeeded(states);
        drawableObject.draw(*this, states);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Draw every element of a range with the same render states
    ///
    /// Convenience overload that iterates over `drawableObjectRange`
    /// and forwards each element to the appropriate single-element
    /// `draw` overload. Useful when you have a `std::vector` (or
    /// any other iterable) of drawables you want to render in a
    /// single call.
    ///
    /// \tparam DrawableObjectRange Any range whose iterator
    ///         dereferences to a drawable accepted by `draw`.
    ///
    /// \param drawableObjectRange Range of drawables to render
    /// \param states              Render states reused for every element
    ///
    ////////////////////////////////////////////////////////////
    template <typename DrawableObjectRange>
    void draw(const DrawableObjectRange& drawableObjectRange, const RenderStates& states = {})
        requires(requires { draw(*drawableObjectRange.begin(), states); })
    {
        for (const auto& drawable : drawableObjectRange)
            draw(drawable, states);
    }

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    DrawStatistics m_currentDrawStats{};                       //!< Statistics for current draw calls
    AutoBatchMode  m_autoBatchMode{AutoBatchMode::GPUStorage}; //!< Enable automatic batching of draw calls
    za::SizeT      m_numAutoBatchVertices{0u};                 //!< Number of vertices in the current autobatch
    za::SizeT      m_autoBatchVertexThreshold{32'768u};        //!< Threshold for batch vertex count
    RenderStates   m_lastRenderStates{};                       //!< Cached render states (autobatching)
    za::U32        m_lastShaderGeneration{0};  //!< Cached shader uniform generation (autobatch invalidation)
    za::U32        m_lastTextureGeneration{0}; //!< Cached texture destructive generation (autobatch invalidation)
    bool           m_isStateLocked{false}; //!< Whether render states are currently bound via `withLockedRenderStates`
    bool           m_isSrgb{false};        //!< Whether the target uses sRGB encoding (set at construction)

    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 2560> m_impl; //!< Implementation details
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::RenderTarget
/// \ingroup graphics
///
/// `za::RenderTarget` is the abstract base of every 2D render
/// target in Zancle. It is the surface onto which sprites,
/// shapes, text, and raw vertex data are drawn, hiding the
/// underlying OpenGL state from user code.
///
/// The two concrete subclasses are `za::RenderWindow` (a target
/// backed by an OS window) and `za::RenderTexture` (a target
/// backed by an off-screen framebuffer object).
///
/// Key features:
/// \li High-level draw overloads for every built-in drawable
///     (`za::Sprite`, `za::Shape`, `za::Text`, `za::VertexBuffer`,
///     `za::CPUDrawableBatch`, `za::PersistentGPUDrawableBatch`,
///     ...).
/// \li Stateless draw overloads taking the corresponding
///     `*ShapeData`, `TextData`, or `GlyphMappedTextData` plain
///     descriptions, for code paths that prefer not to allocate
///     full drawable objects.
/// \li Low-level vertex draw paths for every primitive flavor:
///     `drawVertices`, `drawIndexedVertices`, `drawQuads`,
///     `drawInstancedVertices`, `drawInstancedIndexedVertices`,
///     and `drawPersistentMappedIndexedVertices`.
/// \li Automatic per-frame batching of compatible draw calls,
///     configurable via `setAutoBatchMode` and
///     `setAutoBatchVertexThreshold`. The strategy can be
///     `Disabled`, `CPUStorage`, or `GPUStorage`.
/// \li Per-call `za::RenderStates` (transform, blend mode,
///     stencil mode, view, shader, texture). The
///     `withRenderStates` and `withLockedRenderStates` helpers
///     scope a snapshot to a chained drawing block.
/// \li Native interop with raw OpenGL via `setActive`, `bind`,
///     `unbind`, and `resetGLStates`. Calling
///     `resetGLStates` is required after issuing direct OpenGL
///     commands so that subsequent Zancle draws produce the
///     expected output.
///
/// `za::RenderTarget` is move-only and is not safe to access
/// from multiple threads simultaneously. The OpenGL context it
/// owns is bound to the thread that drives it; if you need
/// cross-thread rendering, deactivate the target on the source
/// thread (`setActive(false)`) before activating it on the
/// destination thread.
///
/// Usage example:
/// \code
/// auto window = za::RenderWindow::create(/* ... */).value();
///
/// while (true)
/// {
///     // Process events, update game state, ...
///
///     window.clear(za::Color::Black);
///     window.draw(sprite, {.texture = &texture});
///     window.draw(shape);
///     window.draw(text);
///     window.display();
/// }
/// \endcode
///
/// \see `za::RenderWindow`, `za::RenderTexture`, `za::View`,
///      `za::RenderStates`, `za::DrawableBatch`
///
////////////////////////////////////////////////////////////
