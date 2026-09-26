#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
#ifndef ZA_OPENGL_ES
    #error "EGLContext included but ES disabled"
#endif


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Config.hpp"

#include "Zancle/GLUtils/GlContext.hpp"

#include "Zancle/Window/ContextSettings.hpp"

#include "Zancle/Vocabulary/InPlacePImpl.hpp"


namespace za::priv
{
class SDLWindowImpl;

////////////////////////////////////////////////////////////
/// \brief EGL-backed implementation of `GlContext`
///
/// Used on platforms that expose OpenGL ES through EGL -- most notably
/// Emscripten and Android. Wraps an `EGLDisplay`, `EGLContext`,
/// `EGLSurface` and `EGLConfig` quartet, picking the best matching
/// config based on the requested `ContextSettings`.
///
////////////////////////////////////////////////////////////
class EglContext : public GlContext
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Create a new offscreen EGL context, not associated with a window
    ///
    /// Allocates an `EGL_PBUFFER` surface of size 1x1 (or a dummy
    /// surface on Emscripten) so the context can be made current
    /// without needing a real window.
    ///
    /// \param id              Unique numeric ID assigned by `WindowContext`
    /// \param shared          Sibling context to share resources with
    ///                        (may be `nullptr` for an unshared context)
    /// \param contextSettings Requested context settings
    ///
    ////////////////////////////////////////////////////////////
    explicit EglContext(unsigned int id, EglContext* shared, const ContextSettings& contextSettings);

    ////////////////////////////////////////////////////////////
    /// \brief Create a new EGL context attached to a window
    ///
    /// \param id              Unique numeric ID assigned by `WindowContext`
    /// \param shared          Sibling context to share resources with
    ///                        (may be `nullptr` for an unshared context)
    /// \param contextSettings Requested context settings
    /// \param owner           Window that owns the context (used to
    ///                        obtain the native window handle)
    /// \param bitsPerPixel    Pixel depth, in bits per pixel
    ///
    ////////////////////////////////////////////////////////////
    explicit EglContext(unsigned int           id,
                        EglContext*            shared,
                        const ContextSettings& contextSettings,
                        const SDLWindowImpl&   owner,
                        unsigned int           bitsPerPixel);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Deactivates the context if it is current on the calling
    /// thread, then destroys the EGL context and surface.
    ///
    ////////////////////////////////////////////////////////////
    ~EglContext() override;

    ////////////////////////////////////////////////////////////
    /// \brief Get the address of an OpenGL function
    ///
    /// \param name Name of the function to get the address of
    ///
    /// \return Address of the OpenGL function, or `nullptr` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] GlFunctionPointer getFunction(const char* name) const override;

    ////////////////////////////////////////////////////////////
    /// \brief Activate or deactivate the context as the current rendering target
    ///
    /// \param activate `true` to make the context current on the
    ///                 calling thread, `false` to deactivate it
    ///
    /// \return `true` on success, `false` if the EGL call failed or
    ///         no surface is associated with the context
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool makeCurrent(bool activate) override;

    ////////////////////////////////////////////////////////////
    /// \brief Present the rendered contents of the context (`eglSwapBuffers`)
    ///
    ////////////////////////////////////////////////////////////
    void display() override;

    ////////////////////////////////////////////////////////////
    /// \brief Enable or disable vertical synchronization
    ///
    /// Forwards to `eglSwapInterval`. No-op on Emscripten, since
    /// browsers manage vsync themselves through `requestAnimationFrame`.
    ///
    /// \param enabled `true` to enable v-sync, `false` to disable it
    ///
    ////////////////////////////////////////////////////////////
    void setVerticalSyncEnabled(bool enabled) override;

    ////////////////////////////////////////////////////////////
    /// \brief Check whether vertical synchronization is enabled
    ///
    /// \return `true` if vertical synchronization is enabled,
    ///         `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isVerticalSyncEnabled() const override;

    ////////////////////////////////////////////////////////////
    /// \brief Create the underlying EGL context object
    ///
    /// Called from the constructors after the display and config have
    /// been picked. The new context is created with OpenGL ES 3.1 on
    /// native platforms and OpenGL ES 2 on Emscripten.
    ///
    /// \param shared Sibling context to share resources with, or
    ///               `nullptr` for an unshared context
    ///
    ////////////////////////////////////////////////////////////
    void createContext(EglContext* shared);

    ////////////////////////////////////////////////////////////
    /// \brief Create the EGL window surface for this context
    ///
    /// Must be called when the underlying native window first becomes
    /// available -- and again on Android whenever the activity is
    /// (re)started or the device orientation changes.
    ///
    /// \param windowPtr Type-erased pointer to a platform-specific
    ///                  native window handle (`EGLNativeWindowType*`)
    ///
    ////////////////////////////////////////////////////////////
    void createSurface(void* windowPtr);

    ////////////////////////////////////////////////////////////
    /// \brief Destroy the EGL window surface
    ///
    /// Must be called on Android whenever the activity is stopped or
    /// the device orientation changes, so that the surface can be
    /// recreated against the new native window.
    ///
    ////////////////////////////////////////////////////////////
    void destroySurface();

private:
    ////////////////////////////////////////////////////////////
    /// \brief Refresh `m_settings` from the chosen EGL config
    ///
    /// Reads the depth/stencil sizes (and resets the version/profile
    /// flags) from the currently selected EGL config so that
    /// `getSettings()` reflects what was actually allocated.
    ///
    ////////////////////////////////////////////////////////////
    void updateSettings();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 64> m_impl; //!< Implementation details (display, context, surface, config)
};

} // namespace za::priv
