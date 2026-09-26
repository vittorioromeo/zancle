#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/BatchedGeometry.hpp"
#include "Zancle/Graphics/DrawIndexedVerticesSettings.hpp"
#include "Zancle/Graphics/DrawVerticesSettings.hpp"
#include "Zancle/Graphics/IndexType.hpp"
#include "Zancle/Graphics/Transformable.hpp"
#include "Zancle/Graphics/Vertex.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/InPlacePImpl.hpp"

#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class Font;
class FontFace;
class GlyphMappedText;
class RenderTarget;
class Shape;
class Text;

struct ArrowShapeData;
struct ChevronShapeData;
struct CircleShapeData;
struct CogShapeData;
struct CrossShapeData;
struct CurvedArrowShapeData;
struct EllipseShapeData;
struct GlyphMapping;
struct GlyphMappedTextData;
struct HeartShapeData;
struct PieSliceShapeData;
struct RectangleShapeData;
struct RingPieSliceShapeData;
struct RingShapeData;
struct RoundedRectangleShapeData;
struct Sprite;
struct StarShapeData;
struct TrapezoidShapeData;
struct TextData;
struct Transform;
} // namespace za


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Internal storage strategy for `DrawableBatchImpl` using CPU-side memory
///
/// `CPUStorage` manages vertex and index data in standard `za::Vector`
/// containers residing in CPU memory. This storage is suitable when
/// batch data is frequently updated or when GPU resources are constrained.
/// Data is typically uploaded to the GPU on each draw call.
///
/// This struct is used as a template parameter for `za::priv::DrawableBatchImpl`.
///
////////////////////////////////////////////////////////////
struct CPUStorage
{
    ////////////////////////////////////////////////////////////
    /// \brief Clears all vertex and index data from the storage
    ///
    /// Resets the internal vertex and index vectors to be empty.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void clear()
    {
        vertices.clear();
        indices.clear();
    }

    ////////////////////////////////////////////////////////////
    /// \brief Reserves capacity for more vertices and returns a pointer to the new region
    ///
    /// Ensures that the internal vertex vector has enough capacity to
    /// store at least `count` additional vertices.
    ///
    /// \param count Number of additional vertices to reserve space for
    /// \return Pointer to the beginning of the reserved (but uncommitted) vertex region
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] Vertex* reserveMoreVertices(const za::SizeT count)
    {
        return vertices.reserveMore(count);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Ensure the vertex storage can accommodate `count` vertices without changing the committed size
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void reserveVertexCapacity(const za::SizeT count)
    {
        vertices.reserve(count);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Reserves capacity for more indices and returns a pointer to the new region
    ///
    /// Ensures that the internal index vector has enough capacity to
    /// store at least `count` additional indices.
    ///
    /// \param count Number of additional indices to reserve space for
    /// \return Pointer to the beginning of the reserved (but uncommitted) index region
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] IndexType* reserveMoreIndices(const za::SizeT count)
    {
        return indices.reserveMore(count);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Ensure the index storage can accommodate `count` indices without changing the committed size
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void reserveIndexCapacity(const za::SizeT count)
    {
        indices.reserve(count);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Commits a number of previously reserved vertices
    ///
    /// Updates the size of the vertex vector to include `count`
    /// more vertices, which are assumed to have been written into
    /// the memory obtained from a prior `reserveMoreVertices` call.
    /// This is an unsafe operation if `count` exceeds reserved capacity.
    ///
    /// \param count Number of vertices to commit
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void commitMoreVertices(const za::SizeT count) noexcept
    {
        vertices.unsafeSetSize(vertices.size() + count);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Commits a number of previously reserved indices
    ///
    /// Updates the size of the index vector to include `count`
    /// more indices, which are assumed to have been written into
    /// the memory obtained from a prior `reserveMoreIndices` call.
    /// This is an unsafe operation if `count` exceeds reserved capacity.
    ///
    /// \param count Number of indices to commit
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void commitMoreIndices(const za::SizeT count) noexcept
    {
        indices.unsafeSetSize(indices.size() + count);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Gets the current number of committed vertices
    ///
    /// \return The total number of vertices currently stored
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] IndexType getNumVertices() const noexcept
    {
        return static_cast<IndexType>(vertices.size());
    }

    ////////////////////////////////////////////////////////////
    /// \brief Gets the current number of committed indices
    ///
    /// \return The total number of indices currently stored
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] IndexType getNumIndices() const noexcept
    {
        return static_cast<IndexType>(indices.size());
    }

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    za::Vector<Vertex>    vertices; //!< CPU buffer for vertices
    za::Vector<IndexType> indices;  //!< CPU buffer for indices
};

////////////////////////////////////////////////////////////
/// \brief Internal storage strategy for `DrawableBatchImpl` using persistent GPU memory
///
/// `PersistentGPUStorage` manages vertex and index data in GPU buffers
/// that are mapped to CPU address space for direct writing. This strategy
/// is designed for performance when batch data changes, as it can avoid
/// per-draw GPU uploads by writing directly to mapped memory.
/// The `flushXXX` member functions are used to signal the GPU that writes are complete.
///
/// This struct is used as a template parameter for `za::priv::DrawableBatchImpl`.
/// It involves more complex GPU memory management, including Vertex Array Objects (VAOs).
///
////////////////////////////////////////////////////////////
struct PersistentGPUStorage
{
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor (3 frame states for CPU/GPU pipelining)
    ///
    /// Initializes the GPU storage with triple-buffered frame states
    /// so that the CPU can fill a batch while the GPU is still
    /// rendering from a previous one.
    ///
    ////////////////////////////////////////////////////////////
    explicit PersistentGPUStorage();

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Releases GPU resources managed by this storage object.
    ///
    ////////////////////////////////////////////////////////////
    ~PersistentGPUStorage();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    PersistentGPUStorage(const PersistentGPUStorage&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    PersistentGPUStorage& operator=(const PersistentGPUStorage&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    PersistentGPUStorage(PersistentGPUStorage&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    PersistentGPUStorage& operator=(PersistentGPUStorage&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Clears the number of active vertices and indices
    ///
    /// Resets the counts of vertices and indices to zero.
    /// Does not deallocate or overwrite GPU memory, but marks it as unused.
    ///
    ////////////////////////////////////////////////////////////
    void clear();

    ////////////////////////////////////////////////////////////
    /// \brief Reserves capacity for more vertices and returns a pointer to the mapped region
    ///
    /// Ensures that the GPU vertex buffer has enough capacity and returns
    /// a CPU-writable pointer to a region where `count` new vertices can be written.
    /// May involve buffer resizing or re-mapping.
    ///
    /// \param count Number of additional vertices to reserve space for
    /// \return Pointer to the beginning of the CPU-writable vertex region on the GPU buffer
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vertex* reserveMoreVertices(za::SizeT count);

    ////////////////////////////////////////////////////////////
    /// \brief Ensure the vertex buffer can accommodate `count` vertices in total without changing the active count
    ///
    ////////////////////////////////////////////////////////////
    void reserveVertexCapacity(za::SizeT count);

    ////////////////////////////////////////////////////////////
    /// \brief Reserves capacity for more indices and returns a pointer to the mapped region
    ///
    /// Ensures that the GPU index buffer has enough capacity and returns
    /// a CPU-writable pointer to a region where `count` new indices can be written.
    /// May involve buffer resizing or re-mapping.
    ///
    /// \param count Number of additional indices to reserve space for
    /// \return Pointer to the beginning of the CPU-writable index region on the GPU buffer
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] IndexType* reserveMoreIndices(za::SizeT count);

    ////////////////////////////////////////////////////////////
    /// \brief Ensure the index buffer can accommodate `count` indices in total without changing the active count
    ///
    ////////////////////////////////////////////////////////////
    void reserveIndexCapacity(za::SizeT count);

    ////////////////////////////////////////////////////////////
    /// \brief Commits a number of previously reserved vertices
    ///
    /// Updates the count of active vertices. The actual data is assumed
    /// to have been written directly to the GPU-mapped memory obtained
    /// from `reserveMoreVertices`.
    ///
    /// \param count Number of vertices to commit
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void commitMoreVertices(const za::SizeT count) noexcept
    {
        nVertices += static_cast<IndexType>(count);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Commits a number of previously reserved indices
    ///
    /// Updates the count of active indices. The actual data is assumed
    /// to have been written directly to the GPU-mapped memory obtained
    /// from `reserveMoreIndices`.
    ///
    /// \param count Number of indices to commit
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void commitMoreIndices(const za::SizeT count) noexcept
    {
        nIndices += static_cast<IndexType>(count);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Gets the current number of active vertices in the GPU buffer
    ///
    /// \return The total number of vertices currently marked as active
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] IndexType getNumVertices() const noexcept
    {
        return nVertices;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Gets the current number of active indices in the GPU buffer
    ///
    /// \return The total number of indices currently marked as active
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] IndexType getNumIndices() const noexcept
    {
        return nIndices;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Gets a pointer to the Vertex Array Object (VAO) group
    /// \warning Internal Zancle detail, subject to change.
    ///
    /// Returns a pointer representing the VAO configuration for this storage.
    /// This is used by the rendering system to set up vertex attribute pointers.
    ///
    /// \return Const void pointer to the VAO group information.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const void* getVAOGroup() const;

    ////////////////////////////////////////////////////////////
    /// \brief Flushes a range of vertex writes to the GPU
    /// \warning Advanced use: ensure correct synchronization and range.
    ///
    /// Signals to the GPU that a specific range of the vertex buffer,
    /// previously written to via a mapped pointer, has been modified
    /// and needs to be synchronized.
    ///
    /// \param count Number of vertices in the range to flush
    /// \param offset Offset (in number of vertices) from the beginning of the buffer
    ///
    ////////////////////////////////////////////////////////////
    void flushVertexWritesToGPU(za::SizeT count, za::SizeT offset) const;

    ////////////////////////////////////////////////////////////
    /// \brief Flushes a range of index writes to the GPU
    /// \warning Advanced use: ensure correct synchronization and range.
    ///
    /// Signals to the GPU that a specific range of the index buffer,
    /// previously written to via a mapped pointer, has been modified
    /// and needs to be synchronized.
    ///
    /// \param count Number of indices in the range to flush
    /// \param offset Offset (in number of indices) from the beginning of the buffer
    ///
    ////////////////////////////////////////////////////////////
    void flushIndexWritesToGPU(za::SizeT count, za::SizeT offset) const;

    ////////////////////////////////////////////////////////////
    /// \brief Commit all writes since the last submission, creating a GPU fence
    ///
    /// Used when a `PersistentGPUDrawableBatch` is submitted directly,
    /// outside of `RenderTarget`'s internal GPU autobatching path. Must
    /// be called after the draw command that consumes the batch has been
    /// issued, so the fence covers the draw.
    ///
    ////////////////////////////////////////////////////////////
    void commitPendingDrawSubmission() const;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 2048> impl; //!< Implementation details

    IndexType nVertices{}; //!< Number of "active" vertices in the buffer
    IndexType nIndices{};  //!< Number of "active" indices in the buffer
};

////////////////////////////////////////////////////////////
/// \brief Base class template for drawable batches
/// \ingroup graphics
///
/// `DrawableBatchImpl` is a templated base class that provides the core
/// functionality for batching multiple drawable objects into a single
/// draw call. It is designed to improve rendering performance by reducing
/// the number of state changes and draw commands sent to the GPU.
///
/// This class manages a collection of vertices and indices, using a
/// storage strategy defined by the `TStorage` template parameter
/// (e.g., `za::priv::CPUStorage` or `za::priv::PersistentGPUStorage`).
/// It inherits from `za::Transformable` to allow the entire batch
/// to be transformed (translated, rotated, scaled).
///
/// Users typically interact with derived classes like `za::CPUDrawableBatch`
/// or `za::PersistentGPUDrawableBatch` rather than this template directly.
///
/// \tparam TStorage The storage strategy for vertex and index data
///
/// \see za::CPUDrawableBatch, za::PersistentGPUDrawableBatch
///
////////////////////////////////////////////////////////////
template <typename TStorage>
class [[nodiscard]] ZA_GRAPHICS_API DrawableBatchImpl : public Transformable
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Constructor
    ///
    /// Initializes the drawable batch, forwarding any arguments
    /// to the constructor of the underlying storage object (`TStorage`).
    ///
    /// \param storageArgs Arguments to pass to the `TStorage` constructor
    ///
    ////////////////////////////////////////////////////////////
    template <typename... TStorageArgs>
    explicit DrawableBatchImpl(TStorageArgs&&... storageArgs) : m_storage(ZA_FORWARD(storageArgs)...)
    {
    }

    ////////////////////////////////////////////////////////////
    /// \brief Reserves space for a given number of triangles
    ///
    /// Pre-allocates memory in the internal storage for the specified
    /// number of triangles (3 vertices and 3 indices per triangle).
    /// This can help avoid reallocations when adding many triangles.
    ///
    /// \param triangleCount Number of triangles to reserve space for
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void reserveTriangles(const za::SizeT triangleCount)
    {
        m_storage.reserveIndexCapacity(3u * triangleCount);
        m_storage.reserveVertexCapacity(3u * triangleCount);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Reserves space for a given number of quads
    ///
    /// Pre-allocates memory in the internal storage for the specified
    /// number of quads (4 vertices and 6 indices per quad, typically
    /// rendered as two triangles). This can help avoid reallocations
    /// when adding many quads.
    ///
    /// \param quadCount Number of quads to reserve space for
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void reserveQuads(const za::SizeT quadCount)
    {
        m_storage.reserveIndexCapacity(6u * quadCount);
        m_storage.reserveVertexCapacity(4u * quadCount);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Adds raw vertex data to the batch
    ///
    /// Appends vertices to the batch. The `type` parameter currently
    /// only supports `za::PrimitiveType::Triangles`, `za::PrimitiveType::TriangleFan`,
    /// `za::PrimitiveType::TriangleFan` and `za::PrimitiveType::TriangleStrip`.
    /// Other types may lead to undefined behavior or be ignored.
    ///
    /// \param settings Struct containing vertex data, count, and primitive type
    ///
    /// \warning Only supports triangle, fan, or strip, primitives.
    ///
    ////////////////////////////////////////////////////////////
    void add(const DrawVerticesSettings& settings);

    ////////////////////////////////////////////////////////////
    /// \brief Adds raw indexed vertex data to the batch
    ///
    /// Appends indexed vertices to the batch. The `type` parameter currently
    /// only supports `za::PrimitiveType::Triangles`, `za::PrimitiveType::TriangleFan`,
    /// `za::PrimitiveType::TriangleFan` and `za::PrimitiveType::TriangleStrip`.
    /// Other types may lead to undefined behavior or be ignored.
    ///
    /// Indices are adjusted to be relative to the start of the newly added vertices.
    ///
    /// \param settings Struct containing vertex data, index data, counts, and primitive type
    ///
    /// \warning Only supports triangle, fan, or strip, primitives.
    ///
    ////////////////////////////////////////////////////////////
    void add(const DrawIndexedVerticesSettings& settings);

    ////////////////////////////////////////////////////////////
    /// \brief Adds an `za::Sprite` to the batch
    ///
    /// Extracts vertex data from the sprite and appends it to the batch.
    ///
    /// \param sprite The sprite to add
    ///
    ////////////////////////////////////////////////////////////
    void add(const Sprite& sprite);

    ////////////////////////////////////////////////////////////
    /// \brief Adds an `za::Shape` to the batch
    ///
    /// Extracts vertex data from the shape (both fill and outline)
    /// and appends it to the batch.
    ///
    /// \param shape The shape to add
    ///
    ////////////////////////////////////////////////////////////
    void add(const Shape& shape);

    ////////////////////////////////////////////////////////////
    /// \brief Adds an `za::Text` object to the batch
    ///
    /// Extracts vertex data from the text object (including fill and outline)
    /// and appends it to the batch.
    ///
    /// \param text The text object to add
    ///
    ////////////////////////////////////////////////////////////
    void add(const Text& text);

    ////////////////////////////////////////////////////////////
    /// \brief Adds an `za::GlyphMappedText` to the batch
    ///
    /// Extracts vertex data from the glyph-mapped text object (including fill and outline)
    /// and appends it to the batch.
    ///
    /// \param text The glyph-mapped text object to add
    ///
    ////////////////////////////////////////////////////////////
    void add(const GlyphMappedText& text);

    ////////////////////////////////////////////////////////////
    /// \brief Adds an arrow shape defined by `za::ArrowShapeData` to the batch
    ///
    /// Generates vertices for an arrow based on the provided data
    /// and appends them to the batch.
    ///
    /// \param sdArrow Data defining the arrow shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const ArrowShapeData& sdArrow);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a circle shape defined by `za::CircleShapeData` to the batch
    ///
    /// Generates vertices for a circle based on the provided data
    /// and appends them to the batch.
    ///
    /// \param sdCircle Data defining the circle shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const CircleShapeData& sdCircle);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a curved arrow shape defined by `za::CurvedArrowShapeData` to the batch
    ///
    /// Generates vertices for a curved arrow based on the provided data
    /// and appends them to the batch.
    ///
    /// \param sdCurvedArrow Data defining the curved arrow shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const CurvedArrowShapeData& sdCurvedArrow);

    ////////////////////////////////////////////////////////////
    /// \brief Adds an ellipse shape defined by `za::EllipseShapeData` to the batch
    ///
    /// Generates vertices for an ellipse based on the provided data
    /// and appends them to the batch.
    ///
    /// \param sdEllipse Data defining the ellipse shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const EllipseShapeData& sdEllipse);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a pie slice shape defined by `za::PieSliceShapeData` to the batch
    ///
    /// Generates vertices for a pie slice based on the provided data
    /// and appends them to the batch.
    ///
    /// \param sdPieSlice Data defining the pie slice shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const PieSliceShapeData& sdPieSlice);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a rectangle shape defined by `za::RectangleShapeData` to the batch
    ///
    /// Generates vertices for a rectangle based on the provided data
    /// and appends them to the batch.
    ///
    /// \param sdRectangle Data defining the rectangle shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const RectangleShapeData& sdRectangle);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a ring shape defined by `za::RingShapeData` to the batch
    ///
    /// Generates vertices for a ring based on the provided data
    /// and appends them to the batch.
    ///
    /// \param sdRing Data defining the ring shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const RingShapeData& sdRing);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a ring pie slice shape defined by `za::RingPieSliceShapeData` to the batch
    ///
    /// Generates vertices for a ring pie slice based on the provided data
    /// and appends them to the batch.
    ///
    /// \param sdRingPieSlice Data defining the ring pie slice shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const RingPieSliceShapeData& sdRingPieSlice);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a rounded rectangle shape defined by `za::RoundedRectangleShapeData` to the batch
    ///
    /// Generates vertices for a rounded rectangle based on the provided data
    /// and appends them to the batch.
    ///
    /// \param sdRoundedRectangle Data defining the rounded rectangle shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const RoundedRectangleShapeData& sdRoundedRectangle);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a star shape defined by `za::StarShapeData` to the batch
    ///
    /// Generates vertices for a star based on the provided data
    /// and appends them to the batch.
    ///
    /// \param sdStarShape Data defining the star shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const StarShapeData& sdStarShape);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a cross (plus) shape defined by `za::CrossShapeData` to the batch
    ///
    /// \param sdCross Data defining the cross shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const CrossShapeData& sdCross);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a trapezoid shape defined by `za::TrapezoidShapeData` to the batch
    ///
    /// \param sdTrapezoid Data defining the trapezoid shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const TrapezoidShapeData& sdTrapezoid);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a chevron shape defined by `za::ChevronShapeData` to the batch
    ///
    /// \param sdChevron Data defining the chevron shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const ChevronShapeData& sdChevron);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a heart shape defined by `za::HeartShapeData` to the batch
    ///
    /// \param sdHeart Data defining the heart shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const HeartShapeData& sdHeart);

    ////////////////////////////////////////////////////////////
    /// \brief Adds a cog (gear) shape defined by `za::CogShapeData` to the batch
    ///
    /// \param sdCog Data defining the cog shape
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const CogShapeData& sdCog);

    ////////////////////////////////////////////////////////////
    /// \brief Adds text geometry defined by `za::Font` and `za::TextData` to the batch
    ///
    /// Generates vertices for text based on the provided font and text data
    /// (which includes string, character size, style, fill/outline colors)
    /// and appends them to the batch.
    ///
    /// \param font The font to use for generating text geometry
    /// \param textData Data defining the text to render
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const Font& font, const TextData& textData);

    ////////////////////////////////////////////////////////////
    /// \brief Add text geometry using a glyph mapping (stateless)
    ///
    /// Generates vertices for text based on the provided glyph mapping and text data.
    ///
    /// \param glyphMapping The glyph mapping to use for generating text geometry
    /// \param textData Data defining the text to render
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry add(const FontFace& fontFace, const GlyphMapping& glyphMapping, const GlyphMappedTextData& textData);

    ////////////////////////////////////////////////////////////
    /// \brief Clears all geometry from the batch
    ///
    /// Removes all vertices and indices from the batch, making it empty.
    /// This calls the `clear` method of the underlying storage.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void clear()
    {
        m_storage.clear();
    }

    ////////////////////////////////////////////////////////////
    /// \brief Checks if the batch is empty
    ///
    /// \return `true` if the batch contains no vertices or indices, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] bool isEmpty() const noexcept
    {
        return m_storage.getNumVertices() == 0u && m_storage.getNumIndices() == 0u;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Gets the total number of vertices in the batch
    ///
    /// \return The current number of vertices stored in the batch
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] IndexType getNumVertices() const noexcept
    {
        return m_storage.getNumVertices();
    }

    ////////////////////////////////////////////////////////////
    /// \brief Gets the total number of indices in the batch
    ///
    /// \return The current number of indices stored in the batch
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] IndexType getNumIndices() const noexcept
    {
        return m_storage.getNumIndices();
    }

private:
    friend RenderTarget;

    ////////////////////////////////////////////////////////////
    /// \brief Generates and adds vertices for a shape resembling a triangle fan
    ///
    /// This utility function is used internally to create geometry for shapes
    /// that can be represented as a central point connected to a series of
    /// outer points (like circles, regular polygons, pie slices).
    ///
    /// \param nPoints Number of points on the shape's perimeter (excluding center)
    /// \param descriptor Shape-specific data (e.g., `CircleShapeData`)
    /// \param pointFn A function that takes an index and returns the coordinate of an outer point
    /// \param localApex Optional fan apex in LOCAL (untransformed) coordinates. When non-null, the
    ///                  apex is transformed alongside the perimeter points, so the caller does not
    ///                  need to pre-transform anything. When null, the world-space bounding-box
    ///                  center of the perimeter is used (suitable for convex/centrally-symmetric
    ///                  shapes).
    ///
    /// \return A `BatchedGeometry` with `fill` and `outline` spans referring to the added vertices.
    ///
    /// \warning The returned span is invalidated after the next call to `add` or batch flush.
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry drawTriangleFanShapeFromPoints(za::SizeT    nPoints,
                                                   const auto&  descriptor,
                                                   auto&&       pointFn,
                                                   const Vec2f* localApex = nullptr);

    ////////////////////////////////////////////////////////////
    /// \brief Adds vertices for a shape's fill to the batch
    ///
    /// Transforms and appends pre-calculated fill vertices.
    ///
    /// \param transform The transform to apply to the vertices
    /// \param data Pointer to the fill vertex data
    /// \param size Number of fill vertices
    ///
    ////////////////////////////////////////////////////////////
    void addShapeFill(const Transform& transform, const Vertex* data, za::SizeT size);

    ////////////////////////////////////////////////////////////
    /// \brief Adds vertices for a shape's outline to the batch
    ///
    /// Transforms and appends pre-calculated outline vertices.
    ///
    /// \param transform The transform to apply to the vertices
    /// \param data Pointer to the outline vertex data
    /// \param size Number of outline vertices
    ///
    ////////////////////////////////////////////////////////////
    void addShapeOutline(const Transform& transform, const Vertex* data, za::SizeT size);

    ////////////////////////////////////////////////////////////
    /// \brief Internal implementation that appends `text` (or `glyphMappedText`) vertices to the batch
    ///
    /// Generic over `za::Text` and `za::GlyphMappedText`.
    ///
    ////////////////////////////////////////////////////////////
    void addTextImpl(const auto& text);

    ////////////////////////////////////////////////////////////
    /// \brief Internal implementation that appends stateless text vertices to the batch
    ///
    /// Generic over the glyph source (`za::Font` or
    /// `za::GlyphMapping`) and the corresponding text data type
    /// (`za::TextData` or `za::GlyphMappedTextData`).
    ///
    ////////////////////////////////////////////////////////////
    BatchedGeometry addTextDataImpl(const auto&  glyphSource,
                                    const auto&  textData,
                                    bool         isBold,
                                    unsigned int characterSize,
                                    float        outlineThickness);

protected:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    TStorage m_storage;
};


////////////////////////////////////////////////////////////
// Explicit instantiation declarations
////////////////////////////////////////////////////////////
extern template class DrawableBatchImpl<CPUStorage>;
extern template class DrawableBatchImpl<PersistentGPUStorage>;

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief A drawable batch that stores vertex data in CPU memory
/// \ingroup graphics
///
/// `CPUDrawableBatch` is a specialization of `DrawableBatchImpl` that
/// uses `za::priv::CPUStorage`. This means all vertex and index data
/// is stored in system memory (RAM).
///
/// Example:
/// \code
/// za::CPUDrawableBatch batch;
///
/// batch.add(za::Sprite{/* ... */});
/// batch.add(font, za::TextData{/* ... */});
/// batch.add(za::CircleShapeData{/* ... */});
/// // ... add more drawables
///
/// window.draw(batch); // Vertex data uploaded to the GPU here
/// \endcode
///
/// \see `za::PersistentGPUDrawableBatch`, `za::priv::DrawableBatchImpl`, `za::priv::CPUStorage`
///
////////////////////////////////////////////////////////////
class CPUDrawableBatch : public priv::DrawableBatchImpl<priv::CPUStorage>
{
    using DrawableBatchImpl<priv::CPUStorage>::DrawableBatchImpl;
};

////////////////////////////////////////////////////////////
/// \brief A drawable batch that stores vertex data in persistent GPU memory
/// \ingroup graphics
///
/// `PersistentGPUDrawableBatch` is a specialization of `DrawableBatchImpl`
/// that uses `za::priv::PersistentGPUStorage`. This means vertex and index
/// data is stored in GPU memory that is persistently mapped to the
/// application's address space.
///
/// Example:
/// \code
/// za::PersistentGPUDrawableBatch batch;
///
/// batch.add(za::Sprite{/* ... */});
/// batch.add(font, za::TextData{/* ... */});
/// batch.add(za::CircleShapeData{/* ... */});
/// // ... add more drawables
///
/// window.draw(batch);
/// \endcode
///
/// \see `za::CPUDrawableBatch`, `za::priv::DrawableBatchImpl`, `za::priv::PersistentGPUStorage`
///
////////////////////////////////////////////////////////////
class PersistentGPUDrawableBatch : public priv::DrawableBatchImpl<priv::PersistentGPUStorage>
{
    using DrawableBatchImpl<priv::PersistentGPUStorage>::DrawableBatchImpl;

public:
    ////////////////////////////////////////////////////////////
    /// \brief Flushes a range of vertex writes to the GPU
    /// \warning Advanced use: ensure correct synchronization and range.
    ///
    /// Signals to the GPU that a specific range of the vertex buffer,
    /// previously written to via a mapped pointer (e.g., by modifying
    /// vertices returned in a `VertexSpan`), has been modified and its
    /// changes need to be made visible to the GPU.
    ///
    /// This method is specific to `PersistentGPUDrawableBatch` due to its
    /// direct GPU memory mapping strategy.
    ///
    /// \param count Number of vertices in the range to flush, starting from `offset`.
    /// \param offset Offset (in number of vertices) from the beginning of the batch's
    ///               vertex buffer where the modified region starts.
    ///
    /// \see za::priv::PersistentGPUStorage::flushVertexWritesToGPU
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void flushVertexWritesToGPU(const za::SizeT count, const za::SizeT offset) const
    {
        m_storage.flushVertexWritesToGPU(count, offset);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Flushes a range of index writes to the GPU
    /// \warning Advanced use: ensure correct synchronization and range.
    ///
    /// Signals to the GPU that a specific range of the index buffer,
    /// previously written to via a mapped pointer, has been modified
    /// and its changes need to be made visible to the GPU.
    ///
    /// This method is specific to `PersistentGPUDrawableBatch`.
    ///
    /// \param count Number of indices in the range to flush, starting from `offset`.
    /// \param offset Offset (in number of indices) from the beginning of the batch's
    ///               index buffer where the modified region starts.
    ///
    /// \see za::priv::PersistentGPUStorage::flushIndexWritesToGPU
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void flushIndexWritesToGPU(const za::SizeT count, const za::SizeT offset) const
    {
        m_storage.flushIndexWritesToGPU(count, offset);
    }
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::DrawableBatch
/// \ingroup graphics
///
/// `za::DrawableBatch` is the concept of batching drawables for
/// performance, realized through the concrete classes
/// `za::CPUDrawableBatch` and `za::PersistentGPUDrawableBatch`.
///
/// Batching is a fundamental optimization in graphics
/// programming. Instead of issuing one draw call per object
/// (sprite, shape, text glyph, ...), objects are collected into
/// a single batch and drawn with one (or very few) GPU
/// submissions. This eliminates the per-call CPU overhead and
/// usually translates to better GPU utilization.
///
/// Zancle provides two batch types:
///
/// \li `za::CPUDrawableBatch` -- vertex data lives in CPU memory.
///     Uploaded to the GPU at draw time. Easier to reason about,
///     no special platform requirements, recommended for highly
///     dynamic content. This is the only supported batch type on
///     OpenGL ES.
///
/// \li `za::PersistentGPUDrawableBatch` -- vertex data lives in a
///     GPU buffer that is persistently mapped to the application's
///     address space. Avoids per-frame uploads at the cost of
///     trickier synchronization. Available where
///     `GL_ARB_buffer_storage` is supported.
///
/// Both batch types inherit from `za::Transformable`, so the
/// **entire** group of batched drawables can be translated,
/// rotated, scaled, and re-anchored as a single unit.
///
/// Usage example with `za::CPUDrawableBatch`:
/// \code
/// za::CPUDrawableBatch batch;
///
/// // Add several drawables to the batch.
/// const za::CircleShape circle{{
///     .position  = {100.f, 100.f},
///     .fillColor = za::Color::Green,
///     .radius    = 50.f,
/// }};
/// batch.add(circle);
///
/// batch.add(za::Sprite{.textureRect = texture.getRect()});
/// batch.add(font, za::TextData{.string = "Hello", .characterSize = 32u});
///
/// // The batch can be transformed as a whole, since it inherits from za::Transformable.
/// batch.position = {5.f, 5.f};
/// batch.rotation = za::degrees(10.f);
///
/// // Draw the entire batch with one call.
/// window.draw(batch);
/// \endcode
///
/// Use the batch type that matches your update frequency and
/// platform constraints.
///
/// \note Outlines for the `*ShapeData` `add` overloads are
/// generated in the shape's **local** coordinate system, exactly
/// like retained `za::Shape` objects: `outlineThickness` scales
/// with the shape's transform, mirroring cannot flip the side the
/// outline grows on, and outline texture coordinates map the
/// outline texture rect over the outline's local bounds.
///
/// \see `za::CPUDrawableBatch`, `za::PersistentGPUDrawableBatch`,
///      `za::Transformable`
///
////////////////////////////////////////////////////////////
