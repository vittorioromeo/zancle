// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Window/SDLGlContext.hpp"

#include "Zancle/Window/ContextSettings.hpp"
#include "Zancle/Window/SDLLayer.hpp"
#include "Zancle/Window/SDLWindowImpl.hpp"
#include "Zancle/Window/WindowContext.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Base/Assert.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>


namespace za::priv
{
////////////////////////////////////////////////////////////
void SDLGlContext::destroyWindowIfNeeded()
{
    if (!m_ownsWindow || m_window == nullptr)
        return;

    SDL_DestroyWindow(m_window);

    m_window     = nullptr;
    m_ownsWindow = false;
}


////////////////////////////////////////////////////////////
void SDLGlContext::initContext(SDLLayer& sdlLayer, SDLGlContext* const shared)
{
    // Set context sharing attributes if a shared context is provided
    if (shared != nullptr)
    {
        if (!shared->makeCurrent(true))
        {
            destroyWindowIfNeeded();
            return;
        }

        // The next created context will be shared with the current one
        if (!sdlLayer.setGLAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1))
            errMsg("Failed to set shared GL context attribute");
    }

    // Create the OpenGL context
    m_context = SDL_GL_CreateContext(m_window);

    if (!m_context)
    {
        errMsg("Failed to create SDL GL context: {}", SDL_GetError());
        destroyWindowIfNeeded();
        return;
    }

    // Reset sharing attribute to default
    if (!sdlLayer.setGLAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0))
        errMsg("Failed to reset shared GL context attribute");
}


////////////////////////////////////////////////////////////
SDLGlContext::SDLGlContext(SDLLayer& sdlLayer, const unsigned int id, SDLGlContext* const shared, const ContextSettings& settings) :
    GlContext(id, settings),
    m_window(nullptr),
    m_context(nullptr),
    m_ownsWindow(false)
{
    if (!sdlLayer.applyGLContextSettings(m_settings))
        errMsg("Failed to apply SDL GL context settings for shared GL context hidden window");

    // Create a hidden window for the context
    m_window = SDL_CreateWindow("", 1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (m_window == nullptr)
    {
        errMsg("Failed to create hidden window for SDLGlContext: {}", SDL_GetError());
        return;
    }

    m_ownsWindow = true;
    initContext(sdlLayer, shared);
}


////////////////////////////////////////////////////////////
SDLGlContext::SDLGlContext(SDLLayer&              sdlLayer,
                           const unsigned int     id,
                           SDLGlContext* const    shared,
                           const ContextSettings& settings,
                           const SDLWindowImpl&   owner,
                           const unsigned int /* bitsPerPixel */) :
    GlContext(id, settings),
    m_window(owner.getSDLHandle()),
    m_context(nullptr),
    m_ownsWindow(false)
{
    initContext(sdlLayer, shared);
}


////////////////////////////////////////////////////////////
SDLGlContext::~SDLGlContext()
{
    WindowContext::cleanupUnsharedFrameBuffers(*this);

    // Deactivate the context if it's current
    if (m_context && SDL_GL_GetCurrentContext() == m_context)
        (void)makeCurrent(false);

    // Delete the context
    if (m_context)
        SDL_GL_DestroyContext(m_context);

    // Destroy the window if owned
    destroyWindowIfNeeded();
}


////////////////////////////////////////////////////////////
GlFunctionPointer SDLGlContext::getFunction(const char* const name) const
{
    return SDL_GL_GetProcAddress(name);
}


////////////////////////////////////////////////////////////
SDL_Window* SDLGlContext::getSDLWindow() const noexcept
{
    return m_window;
}


////////////////////////////////////////////////////////////
bool SDLGlContext::makeCurrent(const bool activate)
{
    ZA_ASSERT((!activate || m_context != nullptr) &&
              "Cannot activate SDL GL context: context was not successfully created");

    auto*       targetWindow  = activate ? m_window : nullptr;
    auto*       targetContext = activate ? m_context : nullptr;
    const char* targetAction  = activate ? "activate" : "deactivate";

    if (!SDL_GL_MakeCurrent(targetWindow, targetContext))
    {
        errMsg("Failed to {} SDL GL context: {}", targetAction, SDL_GetError());
        return false;
    }

    return true;
}


////////////////////////////////////////////////////////////
void SDLGlContext::display()
{
    SDL_GL_SwapWindow(m_window);
}


////////////////////////////////////////////////////////////
void SDLGlContext::setVerticalSyncEnabled(const bool enabled)
{
#ifdef ZA_SYSTEM_EMSCRIPTEN
    // Emscripten path is handled by `Window::display()`
    m_vsyncRequested = enabled;
#else
    if (!SDL_GL_SetSwapInterval(enabled ? 1 : 0))
        errMsg("Failed to set vertical sync: {}", SDL_GetError());
#endif
}


////////////////////////////////////////////////////////////
bool SDLGlContext::isVerticalSyncEnabled() const
{
#ifdef ZA_SYSTEM_EMSCRIPTEN
    return m_vsyncRequested;
#else
    int interval{};

    if (!SDL_GL_GetSwapInterval(&interval))
    {
        errMsg("Failed to get vertical sync: {}", SDL_GetError());
        return false;
    }

    return interval != 0;
#endif
}


} // namespace za::priv
