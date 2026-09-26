// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/GLUtils/GLSharedContextGuard.hpp"

#include "Zancle/GLUtils/GlContext.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Base/Assert.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
GLSharedContextGuard::GLSharedContextGuard() : m_glContext(GlContext::getActiveThreadLocalGlContextPtr())
{
    ZA_ASSERT(m_glContext != nullptr);

    if (!GlContext::setActiveThreadLocalGlContextToSharedContext())
        errMsg("Could not enable shared GL context in `GLSharedContextGuard::GLSharedContextGuard()`");
}


////////////////////////////////////////////////////////////
GLSharedContextGuard::~GLSharedContextGuard()
{
    ZA_ASSERT(m_glContext != nullptr);

    if (!GlContext::setActiveThreadLocalGlContext(*m_glContext, true))
        errMsg("Could not restore context in `GLSharedContextGuard::~GLSharedContextGuard()`");
}

} // namespace za::priv
