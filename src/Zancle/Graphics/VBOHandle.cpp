// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/VBOHandle.hpp"

#include "Zancle/GLUtils/GLBufferObject.hpp"
#include "Zancle/GLUtils/GLPersistentRingBuffer.hpp"
#include "Zancle/GLUtils/GLUniqueResource.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace za
{
////////////////////////////////////////////////////////////
struct VBOHandle::Impl
{
    GLVertexBufferObject vbo;

#ifndef ZA_OPENGL_ES
    GLPersistentRingBuffer<GLVertexBufferObject> persistentRingBuffer;
#endif

    explicit Impl(GLVertexBufferObject&& theVbo) : vbo(ZA_MOVE(theVbo))
    {
    }

    ~Impl()
    {
#ifndef ZA_OPENGL_ES
        persistentRingBuffer.destroy(vbo);
#endif
    }

    Impl(const Impl&)            = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) noexcept        = default;

    Impl& operator=(Impl&& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

#ifndef ZA_OPENGL_ES
        persistentRingBuffer.destroy(vbo);
#endif

        vbo = ZA_MOVE(rhs.vbo);

#ifndef ZA_OPENGL_ES
        persistentRingBuffer = ZA_MOVE(rhs.persistentRingBuffer);
#endif

        return *this;
    }
};


////////////////////////////////////////////////////////////
VBOHandle::VBOHandle() : m_impl{tryCreateGLUniqueResource<GLVertexBufferObject>().value()}
{
}


////////////////////////////////////////////////////////////
VBOHandle::~VBOHandle() = default;


////////////////////////////////////////////////////////////
void VBOHandle::bind()
{
    m_impl->vbo.bind();
}


////////////////////////////////////////////////////////////
za::SizeT VBOHandle::uploadStreamingData(const void* const data, const za::SizeT byteCount)
{
    ZA_ASSERT(data != nullptr);
    ZA_ASSERT(byteCount > 0u);

#ifdef ZA_OPENGL_ES
    bind();
    glCheck(glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(byteCount), data, GL_STREAM_DRAW));
    return 0u;
#else
    // Streaming reuse: if the pending upload would not fit and no uploads
    // are staged for the current draw, block until in-flight cycles complete
    // and restart from the front of the buffer instead of growing it
    // (`reclaim()`/overflow handling no longer reset the ring's cursors).
    m_impl->persistentRingBuffer.drainIfWouldOverflow(byteCount);

    const auto byteOffset = m_impl->persistentRingBuffer.beginWrite(m_impl->vbo, byteCount);

    ZA_MEMCPY(static_cast<char*>(m_impl->persistentRingBuffer.data()) + byteOffset, data, byteCount);
    m_impl->persistentRingBuffer.flushBytesToGPU(m_impl->vbo, byteOffset, byteCount);

    return byteOffset;
#endif
}


////////////////////////////////////////////////////////////
void VBOHandle::commitPendingUploads()
{
#ifndef ZA_OPENGL_ES
    m_impl->persistentRingBuffer.commit();
#endif
}


////////////////////////////////////////////////////////////
void VBOHandle::rollbackPendingUploads() noexcept
{
#ifndef ZA_OPENGL_ES
    m_impl->persistentRingBuffer.rollback();
#endif
}

} // namespace za
