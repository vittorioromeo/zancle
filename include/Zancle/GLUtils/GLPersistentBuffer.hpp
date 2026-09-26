#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Config.hpp" // IWYU pragma: keep

#include "Zancle/GLUtils/Export.hpp"

#include "Zancle/GLUtils/GLBufferObject.hpp"
#include "Zancle/GLUtils/GLUniqueResource.hpp"

#include "Zancle/Base/SizeT.hpp"

#ifdef ZA_OPENGL_ES
    #include "Zancle/Err/Err.hpp"

    #include "Zancle/Base/Abort.hpp"
#else
    #include "Zancle/GLUtils/GLCheck.hpp"
    #include "Zancle/GLUtils/Glad.hpp"

    #include "Zancle/Base/Assert.hpp"
#endif


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Manages a persistently mapped OpenGL buffer
/// \ingroup glutils
///
/// Wraps an OpenGL buffer object that is allocated with immutable
/// storage (`glNamedBufferStorage`) and persistently mapped for writing
/// from the CPU side. By keeping the mapping alive across frames the
/// CPU can update buffer contents through a plain pointer, avoiding the
/// overhead of explicit `glBufferSubData` calls and the implicit
/// synchronization they involve.
///
/// The class is responsible for:
/// - Lazily allocating and mapping the underlying storage on first use
/// - Growing the buffer geometrically (capacity * 1.5) on reservation
///   requests that exceed the current capacity, server-side-copying the
///   requested number of live bytes into the freshly allocated storage
/// - Unmapping the buffer on demand
/// - Flushing explicit ranges of writes to the GPU
///
/// The buffer is mapped with the following flags:
/// `GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_UNSYNCHRONIZED_BIT |
/// GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT |
/// GL_MAP_FLUSH_EXPLICIT_BIT`. The caller is responsible for any
/// CPU/GPU synchronization (typically via fences or memory barriers)
/// before the GPU reads from regions that the CPU has just written.
///
/// Persistent mapping requires desktop OpenGL 4.4+ functionality and is
/// **not** available in OpenGL ES; calling `unmapIfNeeded`,
/// `flushBytesToGPU`, or causing a reservation under
/// `ZA_OPENGL_ES` will abort the program with a fatal error.
///
/// \tparam TBufferObject The type of the underlying buffer object
///                       (e.g. `GLVertexBufferObject`,
///                       `GLElementBufferObject`)
///
////////////////////////////////////////////////////////////
template <typename TBufferObject>
class [[nodiscard]] ZA_GLUTILS_API GLPersistentBuffer
{
public:
    ////////////////////////////////////////////////////////////
    GLPersistentBuffer() = default;


    ////////////////////////////////////////////////////////////
    GLPersistentBuffer(const GLPersistentBuffer&)            = delete;
    GLPersistentBuffer& operator=(const GLPersistentBuffer&) = delete;


    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    /// Transfers the cached mapped pointer and capacity metadata to the
    /// new wrapper and leaves `rhs` empty.
    ///
    /// \warning `GLPersistentBuffer` does not own the associated
    /// `TBufferObject`. The caller must move that buffer object in
    /// lockstep with this wrapper, otherwise `m_mappedPtr` would refer to
    /// the wrong GL object.
    ///
    /// \warning The moved-from wrapper does not unmap anything during the
    /// transfer. Lifetime management of the underlying GL mapping remains
    /// the responsibility of the owning type.
    ///
    ////////////////////////////////////////////////////////////
    GLPersistentBuffer(GLPersistentBuffer&& rhs) noexcept;


    ////////////////////////////////////////////////////////////
    /// \brief Move-assign the cached mapped pointer and capacity metadata
    ///
    /// Leaves `rhs` empty after the transfer.
    ///
    /// \warning This wrapper cannot safely clean up the destination's
    /// previous mapping on its own because it does not own the associated
    /// `TBufferObject`. The destination must already have been unmapped or
    /// otherwise invalidated by the owning type before this assignment.
    ///
    /// \warning As with the move constructor, the caller must move the
    /// matching `TBufferObject` in lockstep so the transferred mapping
    /// metadata continues to describe the correct GL buffer object.
    ///
    ////////////////////////////////////////////////////////////
    GLPersistentBuffer& operator=(GLPersistentBuffer&& rhs) noexcept;


    ////////////////////////////////////////////////////////////
    /// \brief Ensure the buffer can hold at least `byteCount` bytes
    ///
    /// If the current capacity is already sufficient the call is a
    /// no-op (the fast path). Otherwise a new, larger buffer is
    /// allocated and mapped, the existing contents (if any) are copied
    /// into it, and `obj` is replaced with the new buffer object.
    ///
    /// \param obj       The buffer object to (re)allocate. Will be
    ///                  move-assigned a fresh instance on growth.
    /// \param byteCount Minimum number of bytes to make available
    /// \param preserveByteCount Number of bytes from the start of the old
    ///                          storage to copy into the new one on growth
    ///                          (server-side copy); `0` discards the old
    ///                          contents
    ///
    /// \return `true` if a reallocation occurred, `false` if the
    ///         existing storage was already large enough
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] bool reserve(TBufferObject& obj, const za::SizeT byteCount, const za::SizeT preserveByteCount)
    {
        if (m_capacity >= byteCount) [[likely]]
            return false;

        reserveImpl(obj, byteCount, preserveByteCount);
        return true;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Get a write pointer to the persistently mapped storage
    ///
    /// \return Pointer to the mapped buffer memory, or `nullptr` if no
    ///         storage has been reserved yet
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] void* data()
    {
        return m_mappedPtr;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Get a read pointer to the persistently mapped storage
    ///
    /// \return Pointer to the mapped buffer memory, or `nullptr` if no
    ///         storage has been reserved yet
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] const void* data() const
    {
        return m_mappedPtr;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Get the currently allocated capacity in bytes
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] za::SizeT capacity() const
    {
        return m_capacity;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Unmap the buffer if it is currently mapped
    ///
    /// Calls `glUnmapNamedBuffer` on the underlying buffer object and
    /// clears the cached mapped pointer. Safe to call when no mapping
    /// exists (no-op).
    ///
    /// \param obj The buffer object whose mapping should be released
    ///
    /// \warning Aborts the program when compiled for OpenGL ES.
    ///
    ////////////////////////////////////////////////////////////
    void unmapIfNeeded(TBufferObject& obj);


    ////////////////////////////////////////////////////////////
    /// \brief Flush a CPU-written byte range so that it becomes visible to the GPU
    ///
    /// Wraps `glFlushMappedNamedBufferRange`. Because the buffer is
    /// mapped with `GL_MAP_FLUSH_EXPLICIT_BIT`, the driver will not
    /// propagate CPU writes to GPU-visible memory until this function
    /// is called.
    ///
    /// This function does **not** issue a memory barrier; the caller
    /// must still ensure proper synchronization (e.g. `glMemoryBarrier`
    /// or fence sync) before the GPU consumes the data.
    ///
    /// \param obj        The buffer object containing the persistent mapping
    /// \param byteOffset Offset in bytes of the first byte to flush
    /// \param byteCount  Number of bytes in the range to flush
    ///
    /// \warning Aborts the program when compiled for OpenGL ES.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void flushBytesToGPU([[maybe_unused]] const TBufferObject& obj,
                                                [[maybe_unused]] const za::SizeT      byteOffset,
                                                [[maybe_unused]] const za::SizeT      byteCount) const
    {
#ifdef ZA_OPENGL_ES
        priv::errMsg("FATAL ERROR: Persistent OpenGL buffers are not available in OpenGL ES");
        za::abort();
#else
        const auto objId = obj.getId();

        ZA_ASSERT(objId != 0u);
        ZA_ASSERT(m_mappedPtr != nullptr);

        glCheck(glFlushMappedNamedBufferRange(objId, static_cast<GLintptr>(byteOffset), static_cast<GLsizeiptr>(byteCount)));
#endif
    }


private:
    ////////////////////////////////////////////////////////////
    [[gnu::cold, gnu::noinline]] void reserveImpl(TBufferObject& obj, za::SizeT byteCount, za::SizeT preserveByteCount);

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    void*     m_mappedPtr{nullptr}; //!< Write-only mapped pointer
    za::SizeT m_capacity{0u};       //!< Currently allocated capacity of the buffer
};


////////////////////////////////////////////////////////////
// Explicit instantiation declarations: the members that are not defined in the class are defined in
// `GLPersistentBuffer.cpp` (which also lets shared builds export them, as inline members are not)
////////////////////////////////////////////////////////////
extern template class GLPersistentBuffer<GLVertexBufferObject>;
extern template class GLPersistentBuffer<GLElementBufferObject>;

} // namespace za
