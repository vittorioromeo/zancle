// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/GLUtils/GLPersistentBuffer.hpp"

#include "Zancle/GLUtils/GLBufferObject.hpp"
#include "Zancle/GLUtils/GLUniqueResource.hpp"

#ifdef ZA_OPENGL_ES
    #include "Zancle/Err/Err.hpp"

    #include "Zancle/Base/Abort.hpp"
#else
    #include "Zancle/GLUtils/GLCheck.hpp"
    #include "Zancle/GLUtils/Glad.hpp"

    #include "Zancle/Math/MinMaxMacros.hpp"

    #include "Zancle/Base/Assert.hpp"
    #include "Zancle/Base/Macros.hpp"
#endif


namespace za
{
////////////////////////////////////////////////////////////
template <typename TBufferObject>
GLPersistentBuffer<TBufferObject>::GLPersistentBuffer(GLPersistentBuffer&& rhs) noexcept :
    m_mappedPtr{rhs.m_mappedPtr},
    m_capacity{rhs.m_capacity}
{
    rhs.m_mappedPtr = nullptr;
    rhs.m_capacity  = 0u;
}


////////////////////////////////////////////////////////////
template <typename TBufferObject>
GLPersistentBuffer<TBufferObject>& GLPersistentBuffer<TBufferObject>::operator=(GLPersistentBuffer&& rhs) noexcept
{
    if (this == &rhs)
        return *this;

    m_mappedPtr = rhs.m_mappedPtr;
    m_capacity  = rhs.m_capacity;

    rhs.m_mappedPtr = nullptr;
    rhs.m_capacity  = 0u;

    return *this;
}


////////////////////////////////////////////////////////////
template <typename TBufferObject>
void GLPersistentBuffer<TBufferObject>::unmapIfNeeded([[maybe_unused /* not available in EGL */]] TBufferObject& obj)
{
#ifdef ZA_OPENGL_ES
    priv::errMsg("FATAL ERROR: Persistent OpenGL buffers are not available in OpenGL ES");
    za::abort();
#else
    if (m_mappedPtr == nullptr)
        return;

    m_mappedPtr = nullptr;

    obj.bind();

    [[maybe_unused]] const bool rc = glCheck(glUnmapNamedBuffer(obj.getId()));
    ZA_ASSERT(rc);
#endif
}


////////////////////////////////////////////////////////////
template <typename TBufferObject>
void GLPersistentBuffer<TBufferObject>::reserveImpl([[maybe_unused]] TBufferObject&  obj,
                                                    [[maybe_unused]] const za::SizeT byteCount,
                                                    [[maybe_unused]] const za::SizeT preserveByteCount)
{
#ifdef ZA_OPENGL_ES
    priv::errMsg("FATAL ERROR: Persistent OpenGL buffers are not available in OpenGL ES");
    za::abort();
#else
    ZA_ASSERT(m_capacity < byteCount);

    const auto geometricGrowthTarget = m_capacity + (m_capacity / 2u); // Equivalent to `capacity * 1.5`
    const auto newCapacity           = ZA_MAX(byteCount, geometricGrowthTarget);

    auto newObj = tryCreateGLUniqueResource<TBufferObject>().value();
    newObj.bind();

    glCheck(glNamedBufferStorage(newObj.getId(),
                                 static_cast<GLsizeiptr>(newCapacity),
                                 /* data */ nullptr,
                                 GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT));

    // Per ARB_buffer_storage (issue 6): when `GL_MAP_PERSISTENT_BIT` is set,
    // the `GL_MAP_INVALIDATE_*` bits are ignored. The buffer was just created
    // (no prior contents to discard), so there is nothing useful to invalidate
    // anyway -- dropped to keep the flag set minimal.
    void* const newMappedPtr = glCheck(
        glMapNamedBufferRange(newObj.getId(),
                              /* offset */ 0u,
                              /* length */ static_cast<GLsizeiptr>(newCapacity),
                              GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_UNSYNCHRONIZED_BIT |
                                  GL_MAP_FLUSH_EXPLICIT_BIT));

    ZA_ASSERT(newMappedPtr != nullptr);

    if (m_mappedPtr != nullptr)
    {
        if (preserveByteCount > 0u)
        {
            ZA_ASSERT(preserveByteCount <= m_capacity);

            // The old mapping has no `GL_MAP_READ_BIT`: reading it from the
            // CPU (memcpy) is undefined per spec and hits uncached
            // write-combined memory in practice. Copy server-side instead,
            // and only the live bytes rather than the full old capacity.
            //
            // The mapping is `GL_MAP_FLUSH_EXPLICIT_BIT`, so CPU writes are
            // only guaranteed visible to GL commands after an explicit
            // flush -- and growth can happen mid-write-cycle, before the
            // caller's own flush. Flush the live range first. Copying while
            // still mapped is legal because the mapping is persistent
            // (`GL_MAP_PERSISTENT_BIT`); `obj` stays alive until the
            // move-assignment below, so the copy source is valid.
            flushBytesToGPU(obj, /* byteOffset */ 0u, preserveByteCount);

            glCheck(glCopyNamedBufferSubData(obj.getId(),
                                             newObj.getId(),
                                             /* readOffset */ 0,
                                             /* writeOffset */ 0,
                                             static_cast<GLsizeiptr>(preserveByteCount)));
        }

        unmapIfNeeded(obj);
    }

    obj = ZA_MOVE(newObj);
    obj.bind();

    m_mappedPtr = newMappedPtr;
    m_capacity  = newCapacity;
#endif
}


////////////////////////////////////////////////////////////
// Explicit instantiation definitions
////////////////////////////////////////////////////////////
template class GLPersistentBuffer<GLVertexBufferObject>;
template class GLPersistentBuffer<GLElementBufferObject>;

} // namespace za
