#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/GLUtils/GlContext.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
struct SDL_Window;
struct SDL_GLContextState;

namespace za::priv
{
class SDLLayer;
class SDLWindowImpl;
} // namespace za::priv

namespace za
{
struct ContextSettings;
}


namespace za::priv
{
////////////////////////////////////////////////////////////
class SDLGlContext : public GlContext
{
public:
    ////////////////////////////////////////////////////////////
    /// rief Create a context with its own hidden window
    ///
    /// `sdlLayer` is passed explicitly (rather than obtained from the
    /// installed `WindowContext`) because the shared context is created
    /// while the `WindowContext` itself is still being constructed.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit SDLGlContext(SDLLayer& sdlLayer, unsigned int id, SDLGlContext* shared, const ContextSettings& settings);

    ////////////////////////////////////////////////////////////
    /// rief Create a context for an existing window
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit SDLGlContext(
        SDLLayer&              sdlLayer,
        unsigned int           id,
        SDLGlContext*          shared,
        const ContextSettings& settings,
        const SDLWindowImpl&   owner,
        unsigned int           bitsPerPixel);

    ////////////////////////////////////////////////////////////
    ~SDLGlContext() override;

    ////////////////////////////////////////////////////////////
    SDLGlContext(const SDLGlContext&)            = delete;
    SDLGlContext& operator=(const SDLGlContext&) = delete;

    ////////////////////////////////////////////////////////////
    SDLGlContext(SDLGlContext&&)            = delete;
    SDLGlContext& operator=(SDLGlContext&&) = delete;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] GlFunctionPointer getFunction(const char* name) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SDL_Window* getSDLWindow() const noexcept;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool makeCurrent(bool activate) override;

    ////////////////////////////////////////////////////////////
    void display() override;

    ////////////////////////////////////////////////////////////
    void setVerticalSyncEnabled(bool enabled) override;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isVerticalSyncEnabled() const override;

private:
    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void initContext(SDLLayer& sdlLayer, SDLGlContext* shared);

    ////////////////////////////////////////////////////////////
    /// \brief TODO P1: docs
    ///
    ////////////////////////////////////////////////////////////
    void destroyWindowIfNeeded();

    ////////////////////////////////////////////////////////////
    SDL_Window*         m_window;     // SDL window associated with the context
    SDL_GLContextState* m_context;    // SDL OpenGL context handle
    bool                m_ownsWindow; // Whether the context owns the window (for offscreen contexts)
#ifdef ZA_SYSTEM_EMSCRIPTEN
    bool m_vsyncRequested{true}; // Emscripten VSync support needs manual handling
#endif
};
} // namespace za::priv
