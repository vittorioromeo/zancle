// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/VertexBuffer.hpp"

#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/Glad.hpp"

#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/Vertex.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Swap.hpp"


namespace
{
// A nested named namespace is used here to allow unity builds of Zancle.
namespace VertexBufferImpl
{
GLenum usageToGlEnum(za::VertexBuffer::Usage usage)
{
    switch (usage)
    {
        case za::VertexBuffer::Usage::Static:
            return GL_STATIC_DRAW;
        case za::VertexBuffer::Usage::Dynamic:
            return GL_DYNAMIC_DRAW;
        default:
            return GL_STREAM_DRAW;
    }
}
} // namespace VertexBufferImpl
} // namespace


namespace za
{
////////////////////////////////////////////////////////////
VertexBuffer::VertexBuffer() = default;


////////////////////////////////////////////////////////////
VertexBuffer::VertexBuffer(PrimitiveType type) : m_primitiveType(type)
{
}


////////////////////////////////////////////////////////////
VertexBuffer::VertexBuffer(Usage usage) : m_usage(usage)
{
}


////////////////////////////////////////////////////////////
VertexBuffer::VertexBuffer(PrimitiveType type, Usage usage) : m_primitiveType(type), m_usage(usage)
{
}


////////////////////////////////////////////////////////////
VertexBuffer::VertexBuffer(const VertexBuffer& rhs) : m_primitiveType(rhs.m_primitiveType), m_usage(rhs.m_usage)
{
    if (rhs.m_buffer && rhs.m_size)
    {
        if (!create(rhs.m_size))
        {
            priv::errMsg("Could not create vertex buffer for copying");
            return;
        }

        if (!update(rhs))
            priv::errMsg("Could not copy vertex buffer");
    }
}


////////////////////////////////////////////////////////////
VertexBuffer::~VertexBuffer()
{
    if (m_buffer)
    {
        ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
        ZA_ASSERT(GraphicsContext::isInstalled());

        glCheck(glDeleteBuffers(1, &m_buffer));
    }
}


////////////////////////////////////////////////////////////
bool VertexBuffer::create(za::SizeT vertexCount)
{
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
    ZA_ASSERT(GraphicsContext::isInstalled());

    if (!m_buffer)
        glCheck(glGenBuffers(1, &m_buffer));

    if (!m_buffer)
    {
        priv::errMsg("Could not create vertex buffer, generation failed");
        return false;
    }

    glCheck(glBindBuffer(GL_ARRAY_BUFFER, m_buffer));
    glCheck(glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptrARB>(sizeof(Vertex) * vertexCount),
                         nullptr,
                         VertexBufferImpl::usageToGlEnum(m_usage)));
    glCheck(glBindBuffer(GL_ARRAY_BUFFER, 0));

    m_size = vertexCount;

    return true;
}


////////////////////////////////////////////////////////////
za::SizeT VertexBuffer::getVertexCount() const
{
    return m_size;
}


////////////////////////////////////////////////////////////
bool VertexBuffer::update(const Vertex* vertices)
{
    return update(vertices, m_size, 0);
}


////////////////////////////////////////////////////////////
bool VertexBuffer::update(const Vertex* vertices, za::SizeT vertexCount, unsigned int offset)
{
    // Sanity checks
    if (!m_buffer)
        return false;

    if (!vertices)
        return false;

    if (offset && (offset + vertexCount > m_size))
        return false;

    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
    ZA_ASSERT(GraphicsContext::isInstalled());

    glCheck(glBindBuffer(GL_ARRAY_BUFFER, m_buffer));

    // Check if we need to resize or orphan the buffer
    if (vertexCount >= m_size)
    {
        glCheck(glBufferData(GL_ARRAY_BUFFER,
                             static_cast<GLsizeiptrARB>(sizeof(Vertex) * vertexCount),
                             nullptr,
                             VertexBufferImpl::usageToGlEnum(m_usage)));

        m_size = vertexCount;
    }

    glCheck(glBufferSubData(GL_ARRAY_BUFFER,
                            static_cast<GLintptrARB>(sizeof(Vertex) * offset),
                            static_cast<GLsizeiptrARB>(sizeof(Vertex) * vertexCount),
                            vertices));

    glCheck(glBindBuffer(GL_ARRAY_BUFFER, 0));

    return true;
}


////////////////////////////////////////////////////////////
bool VertexBuffer::update(const VertexBuffer& vertexBuffer)
{
    if (!m_buffer || !vertexBuffer.m_buffer)
        return false;

    if (vertexBuffer.m_size > m_size)
    {
        priv::errMsg("Could not copy vertex buffer, destination buffer is too small");
        return false;
    }

    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
    ZA_ASSERT(GraphicsContext::isInstalled());

    glCheck(glBindBuffer(GL_COPY_READ_BUFFER, vertexBuffer.m_buffer));
    glCheck(glBindBuffer(GL_COPY_WRITE_BUFFER, m_buffer));

    glCheck(glCopyBufferSubData(GL_COPY_READ_BUFFER,
                                GL_COPY_WRITE_BUFFER,
                                0,
                                0,
                                static_cast<GLsizeiptr>(sizeof(Vertex) * vertexBuffer.m_size)));

    glCheck(glBindBuffer(GL_COPY_WRITE_BUFFER, 0));
    glCheck(glBindBuffer(GL_COPY_READ_BUFFER, 0));

    return true;
}


////////////////////////////////////////////////////////////
VertexBuffer& VertexBuffer::operator=(const VertexBuffer& rhs)
{
    VertexBuffer temp(rhs);

    swap(temp);

    return *this;
}


////////////////////////////////////////////////////////////
void VertexBuffer::swap(VertexBuffer& rhs) noexcept
{
    za::genericSwap(m_size, rhs.m_size);
    za::genericSwap(m_buffer, rhs.m_buffer);
    za::genericSwap(m_primitiveType, rhs.m_primitiveType);
    za::genericSwap(m_usage, rhs.m_usage);
}


////////////////////////////////////////////////////////////
unsigned int VertexBuffer::getNativeHandle() const
{
    return m_buffer;
}


////////////////////////////////////////////////////////////
void VertexBuffer::bind() const
{
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
    ZA_ASSERT(GraphicsContext::isInstalled());
    glCheck(glBindBuffer(GL_ARRAY_BUFFER, m_buffer));
}


////////////////////////////////////////////////////////////
void VertexBuffer::unbind()
{
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
    ZA_ASSERT(GraphicsContext::isInstalled());
    glCheck(glBindBuffer(GL_ARRAY_BUFFER, 0u));
}


////////////////////////////////////////////////////////////
void VertexBuffer::setPrimitiveType(PrimitiveType type)
{
    m_primitiveType = type;
}


////////////////////////////////////////////////////////////
PrimitiveType VertexBuffer::getPrimitiveType() const
{
    return m_primitiveType;
}


////////////////////////////////////////////////////////////
void VertexBuffer::setUsage(Usage usage)
{
    m_usage = usage;
}


////////////////////////////////////////////////////////////
VertexBuffer::Usage VertexBuffer::getUsage() const
{
    return m_usage;
}


////////////////////////////////////////////////////////////
void VertexBuffer::draw(RenderTarget& target, RenderStates states) const
{
    if (m_buffer && m_size)
        target.draw(*this, 0, m_size, states);
}


////////////////////////////////////////////////////////////
void swap(VertexBuffer& lhs, VertexBuffer& rhs) noexcept
{
    lhs.swap(rhs);
}

} // namespace za
