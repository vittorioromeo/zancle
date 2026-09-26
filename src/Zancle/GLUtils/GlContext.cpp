// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/GLUtils/GlContext.hpp"

#include "Zancle/Config.hpp" // IWYU pragma: keep

#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/GlFuncTypesImpl.hpp"
#include "Zancle/GLUtils/Glad.hpp"

#include "Zancle/Window/ContextSettings.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Strstr.hpp"


namespace
{
////////////////////////////////////////////////////////////
thread_local constinit struct
{
    unsigned int         id{0u};
    za::priv::GlContext* ptr{nullptr};
} activeGlContext;


////////////////////////////////////////////////////////////
constinit za::priv::GlContext* sharedGlContextPtr{nullptr}; //!< Owned by `WindowContext`

} // namespace


namespace za::priv
{
////////////////////////////////////////////////////////////
GlContext::~GlContext()
{
    // If this context is not the active one on this thread, don't do anything
    if (m_id != activeGlContext.id)
        return;

    if (!setActiveThreadLocalGlContextToSharedContext())
    {
        errMsg("Failed to enable shared GL context in `GlContext::~GlContext`");
        ZA_ASSERT(false);
    }
}


////////////////////////////////////////////////////////////
void GlContext::setSharedGlContext(GlContext* const sharedGlContext) noexcept
{
    if (sharedGlContext == nullptr && activeGlContext.ptr == sharedGlContextPtr)
        activeGlContext = {};

    sharedGlContextPtr = sharedGlContext;
}


////////////////////////////////////////////////////////////
GlContext* GlContext::getActiveThreadLocalGlContextPtr() noexcept
{
    return activeGlContext.ptr;
}


////////////////////////////////////////////////////////////
unsigned int GlContext::getActiveThreadLocalGlContextId() noexcept
{
    return activeGlContext.id;
}


////////////////////////////////////////////////////////////
bool GlContext::hasActiveThreadLocalGlContext() noexcept
{
    return activeGlContext.id != 0u && activeGlContext.ptr != nullptr;
}


////////////////////////////////////////////////////////////
bool GlContext::isActiveGlContextSharedContext() noexcept
{
    return activeGlContext.ptr != nullptr && activeGlContext.ptr == sharedGlContextPtr;
}


////////////////////////////////////////////////////////////
bool GlContext::setActiveThreadLocalGlContext(GlContext& glContext, const bool active)
{
    // If `glContext` is already the active one on this thread, don't do anything
    if (active && glContext.m_id == activeGlContext.id)
    {
        ZA_ASSERT(activeGlContext.ptr == &glContext);
        return true;
    }

    // If `glContext` is not the active one on this thread, don't do anything
    if (!active && glContext.m_id != activeGlContext.id)
    {
        ZA_ASSERT(activeGlContext.ptr != &glContext);
        return true;
    }

    // Activate/deactivate the context
    if (!glContext.makeCurrent(active))
    {
        errMsg("`glContext.makeCurrent` failure in `GlContext::setActiveThreadLocalGlContext`");
        return false;
    }

    if (&glContext == sharedGlContextPtr)
    {
        ZA_ASSERT(active);

        activeGlContext.id  = glContext.m_id;
        activeGlContext.ptr = &glContext;
    }
    else
    {
        // Revert to shared context if `glContext` is disabled
        ZA_ASSERT(active || sharedGlContextPtr != nullptr);

        activeGlContext.id  = active ? glContext.m_id : sharedGlContextPtr->m_id;
        activeGlContext.ptr = active ? &glContext : sharedGlContextPtr;
    }

    return true;
}


////////////////////////////////////////////////////////////
bool GlContext::setActiveThreadLocalGlContextToSharedContext()
{
    if (sharedGlContextPtr == nullptr) [[unlikely]]
    {
        errMsg("No shared GL context -- is a `za::WindowContext` installed?");
        return false;
    }

    return setActiveThreadLocalGlContext(*sharedGlContextPtr, true);
}


////////////////////////////////////////////////////////////
bool GlContext::disableSharedGlContext()
{
    ZA_ASSERT(hasActiveThreadLocalGlContext());
    ZA_ASSERT(isActiveGlContextSharedContext());

    if (!sharedGlContextPtr->makeCurrent(false))
    {
        errMsg("Could not disable shared GL context in `GlContext::disableSharedGlContext()`");
        return false;
    }

    activeGlContext = {};
    return true;
}


////////////////////////////////////////////////////////////
const ContextSettings& GlContext::getSettings() const
{
    return m_settings;
}


////////////////////////////////////////////////////////////
unsigned int GlContext::getId() const
{
    return m_id;
}


////////////////////////////////////////////////////////////
GlContext::GlContext(unsigned int id, const ContextSettings& contextSettings) : m_settings(contextSettings), m_id{id}
{
}


////////////////////////////////////////////////////////////
bool GlContext::initialize(const GlContext& sharedGlContext, const ContextSettings& requestedSettings)
{
    ZA_ASSERT(getActiveThreadLocalGlContextPtr() == this);

    // Try the new way first
    auto glGetIntegervFunc = reinterpret_cast<glGetIntegervFuncType>(sharedGlContext.getFunction("glGetIntegerv"));

    auto glGetErrorFunc = reinterpret_cast<glGetErrorFuncType>(sharedGlContext.getFunction("glGetError"));

    if (!glGetIntegervFunc || !glGetErrorFunc)
    {
        errMsg("Could not load necessary function to initialize OpenGL context");
        return false;
    }

#if defined(ZA_SYSTEM_EMSCRIPTEN)

    // Hardcoded for WebGL 2.0
    m_settings.majorVersion   = 3;
    m_settings.minorVersion   = 0;
    m_settings.attributeFlags = requestedSettings.attributeFlags;

#else

    // Retrieve the context version number
    int majorVersion = 0;
    glCheckIgnoreWithFunc(glGetErrorFunc, glGetIntegervFunc(GL_MAJOR_VERSION, &majorVersion));
    m_settings.majorVersion = static_cast<unsigned int>(majorVersion);

    if (m_settings.majorVersion < 3)
    {
        errMsg("Context major version below 3.x is not supported");
        return false;
    }

    int minorVersion = 0;
    glCheckIgnoreWithFunc(glGetErrorFunc, glGetIntegervFunc(GL_MINOR_VERSION, &minorVersion));
    m_settings.minorVersion = static_cast<unsigned int>(minorVersion);

    m_settings.attributeFlags = requestedSettings.attributeFlags;

    // Retrieve the context flags
    int flags = 0;
    glCheckIgnoreWithFunc(glGetErrorFunc, glGetIntegervFunc(GL_CONTEXT_FLAGS, &flags));

    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        m_settings.attributeFlags |= ContextSettings::Attribute::Debug;

    if ((m_settings.majorVersion == 3) && (m_settings.minorVersion == 1))
    {
        // OpenGL ES likely hits this path

        m_settings.attributeFlags |= ContextSettings::Attribute::Core;

        if (auto glGetStringiFunc = reinterpret_cast<glGetStringiFuncType>(sharedGlContext.getFunction("glGetStr"
                                                                                                       "ingi")))
        {
            int numExtensions = 0;
            glCheckIgnoreWithFunc(glGetErrorFunc, glGetIntegervFunc(GL_NUM_EXTENSIONS, &numExtensions));

            for (unsigned int i = 0; i < static_cast<unsigned int>(numExtensions); ++i)
            {
                const auto* extensionString = reinterpret_cast<const char*>(glGetStringiFunc(GL_EXTENSIONS, i));

                if (ZA_STRSTR(extensionString, "GL_ARB_compatibility"))
                {
                    m_settings.attributeFlags &= ~ContextSettings::Attribute::Core;
                    break;
                }
            }
        }
    }
    else if ((m_settings.majorVersion > 3) || (m_settings.minorVersion >= 2))
    {
        // Retrieve the context profile
        int profile = 0;
        glCheckIgnoreWithFunc(glGetErrorFunc, glGetIntegervFunc(GL_CONTEXT_PROFILE_MASK, &profile));

        if (profile & GL_CONTEXT_CORE_PROFILE_BIT)
            m_settings.attributeFlags |= ContextSettings::Attribute::Core;
    }

#endif

    return true;
}


////////////////////////////////////////////////////////////
void GlContext::checkSettings(const ContextSettings& requestedSettings) const
{
    const auto boolToString = [](bool b) { return b ? "true" : "false"; };

    // Perform checks to inform the user if they are getting a context they might not have expected
    const int version = static_cast<int>(m_settings.majorVersion * 10u + m_settings.minorVersion);
    const int requestedVersion = static_cast<int>(requestedSettings.majorVersion * 10u + requestedSettings.minorVersion);

    if ((m_settings.attributeFlags != requestedSettings.attributeFlags) || (version < requestedVersion) ||
        (m_settings.stencilBits < requestedSettings.stencilBits) || (m_settings.depthBits < requestedSettings.depthBits))
    {
        errMsg(
            "Warning: The created OpenGL context does not fully meet the settings that were requested{}Requested: "
            "version = {}.{} ; depth bits = {} ; stencil bits = {} ; core = {} ; debug = {}{}Created: version = {}.{} "
            "; depth bits = {} ; stencil bits = {} ; core = {} ; debug = {}",
            '\n',
            requestedSettings.majorVersion,
            requestedSettings.minorVersion,
            requestedSettings.depthBits,
            requestedSettings.stencilBits,
            boolToString(requestedSettings.isCore()),
            boolToString(requestedSettings.isDebug()),
            '\n',
            m_settings.majorVersion,
            m_settings.minorVersion,
            m_settings.depthBits,
            m_settings.stencilBits,
            boolToString(m_settings.isCore()),
            boolToString(m_settings.isDebug()));
    }
}

} // namespace za::priv
