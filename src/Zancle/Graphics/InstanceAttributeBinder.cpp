// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/InstanceAttributeBinder.hpp"

#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/Glad.hpp"

#include "Zancle/Graphics/GlDataType.hpp"
#include "Zancle/Graphics/Priv/EnumToGlEnumConversions.hpp"
#include "Zancle/Graphics/VBOHandle.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Unreachable.hpp"

#ifdef ZA_OPENGL_ES
    #include "Zancle/Base/Abort.hpp"
#endif


namespace
{
////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] constexpr bool isIntegerAttributeType(const za::GlDataType type) noexcept
{
    switch (type)
    {
        case za::GlDataType::Byte:
        case za::GlDataType::UnsignedByte:
        case za::GlDataType::Short:
        case za::GlDataType::UnsignedShort:
        case za::GlDataType::Int:
        case za::GlDataType::UnsignedInt:
            return true;

        case za::GlDataType::HalfFloat:
        case za::GlDataType::Float:
        case za::GlDataType::Double:
            return false;
    }

    ZA_UNREACHABLE();
}


////////////////////////////////////////////////////////////
[[gnu::always_inline, gnu::flatten]] inline void setupVertexAttribPointer(
    const unsigned int   location,
    const unsigned int   size,
    const za::GlDataType type,
    const bool           normalized,
    const GLsizei        stride,
    void* const          offset)
{
    const GLenum glType = za::priv::dataTypeToOpenGLDataType(type);

    if (type == za::GlDataType::Double)
    {
#ifdef ZA_OPENGL_ES
        za::priv::errMsg("FATAL ERROR: per-instance `double` attributes are unsupported on OpenGL ES");
        za::abort();
#else
        glCheck(glVertexAttribLPointer(/*  index */ location,
                                       /*   size */ static_cast<GLint>(size),
                                       /*   type */ glType,
                                       /* stride */ stride,
                                       /* offset */ offset));
#endif

        return;
    }

    if (isIntegerAttributeType(type) && !normalized)
    {
        glCheck(glVertexAttribIPointer(/*  index */ location,
                                       /*   size */ static_cast<GLint>(size),
                                       /*   type */ glType,
                                       /* stride */ stride,
                                       /* offset */ offset));

        return;
    }

    glCheck(glVertexAttribPointer(/*      index */ location,
                                  /*       size */ static_cast<GLint>(size),
                                  /*       type */ glType,
                                  /* normalized */ normalized ? GL_TRUE : GL_FALSE,
                                  /*     stride */ stride,
                                  /*     offset */ offset));
}

} // namespace


namespace za
{
////////////////////////////////////////////////////////////
InstanceAttributeBinder::InstanceAttributeBinder(const za::SizeT instanceCount) : m_instanceCount(instanceCount)
{
    ZA_ASSERT(instanceCount > 0u);
}

////////////////////////////////////////////////////////////
InstanceAttributeBinder::~InstanceAttributeBinder()
{
    if (m_drawSubmitted)
    {
        for (VBOHandle* const vboHandle : m_touchedVBOHandles)
            vboHandle->commitPendingUploads();

        return;
    }

    for (VBOHandle* const vboHandle : m_touchedVBOHandles)
        vboHandle->rollbackPendingUploads();
}


////////////////////////////////////////////////////////////
void InstanceAttributeBinder::uploadData(VBOHandle& vboHandle, const void* const data, const za::SizeT stride)
{
    ZA_ASSERT(data != nullptr);
    ZA_ASSERT(stride > 0u);

    m_currentVBOHandle        = &vboHandle;
    m_currentUploadByteOffset = vboHandle.uploadStreamingData(data, stride * m_instanceCount);

    bool alreadyTouched = false;

    for (VBOHandle* const touchedVBOHandle : m_touchedVBOHandles)
    {
        if (touchedVBOHandle == &vboHandle)
        {
            alreadyTouched = true;
            break;
        }
    }

    if (!alreadyTouched)
    {
        if (m_touchedVBOHandles.size() == m_touchedVBOHandles.capacity()) [[unlikely]]
        {
            priv::errMsg("`InstanceAttributeBinder`: too many distinct VBOs for one draw, upload ignored");
            ZA_ASSERT(false);
            return;
        }

        m_touchedVBOHandles.emplaceBack(&vboHandle);
    }
}


////////////////////////////////////////////////////////////
void InstanceAttributeBinder::setup(
    const unsigned int location,
    const unsigned int size,
    const GlDataType   type,
    const bool         normalized,
    const za::SizeT    stride,
    const za::SizeT    fieldOffset)
{
    ZA_ASSERT(size >= 1u && size <= 4u);
    ZA_ASSERT(stride > 0u);
    ZA_ASSERT(fieldOffset < stride);
    ZA_ASSERT(m_currentVBOHandle != nullptr);

    if (m_deferredSetups.size() == m_deferredSetups.capacity()) [[unlikely]]
    {
        priv::errMsg("`InstanceAttributeBinder`: too many attribute setups for one draw, setup ignored");
        ZA_ASSERT(false);
        return;
    }

    m_deferredSetups.emplaceBack(DeferredSetup{
        .vboHandle  = m_currentVBOHandle,
        .location   = location,
        .size       = size,
        .type       = type,
        .normalized = normalized,
        .stride     = stride,
        .byteOffset = m_currentUploadByteOffset + fieldOffset,
    });
}

////////////////////////////////////////////////////////////
void InstanceAttributeBinder::applySetups() const
{
    VBOHandle* lastBoundVBOHandle = nullptr;

    for (const DeferredSetup& deferredSetup : m_deferredSetups)
    {
        if (deferredSetup.vboHandle != lastBoundVBOHandle)
        {
            deferredSetup.vboHandle->bind();
            lastBoundVBOHandle = deferredSetup.vboHandle;
        }

        glCheck(glEnableVertexAttribArray(deferredSetup.location));

        setupVertexAttribPointer(deferredSetup.location,
                                 deferredSetup.size,
                                 deferredSetup.type,
                                 deferredSetup.normalized,
                                 static_cast<GLsizei>(deferredSetup.stride),
                                 reinterpret_cast<void*>(deferredSetup.byteOffset));

        glCheck(glVertexAttribDivisor(deferredSetup.location, 1));
    }
}

////////////////////////////////////////////////////////////
void InstanceAttributeBinder::markDrawSubmitted() noexcept
{
    m_drawSubmitted = true;
}

} // namespace za
