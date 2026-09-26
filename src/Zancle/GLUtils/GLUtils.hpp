#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/GLUtils/Export.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Query an OpenGL integer state via `glGetIntegerv`
///
/// Convenience wrapper around `glGetIntegerv` that returns the queried
/// value directly. The call goes through `glCheck`, so any error is
/// reported in debug builds.
///
/// \param parameterName The GL state enum to query (e.g.
///                      `GL_MAX_TEXTURE_SIZE`)
///
/// \return The integer value reported by OpenGL
///
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_GLUTILS_API int getGLInteger(unsigned int parameterName);

////////////////////////////////////////////////////////////
/// \brief Generate and bind a new framebuffer object
///
/// Creates a new OpenGL framebuffer object via `glGenFramebuffers` and,
/// if creation succeeds, immediately binds it to the `GL_FRAMEBUFFER`
/// target on the current context.
///
/// \return The new framebuffer ID, or `0` if framebuffer creation failed
///
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_GLUTILS_API unsigned int generateAndBindFramebuffer();

////////////////////////////////////////////////////////////
/// \brief RAII guard that temporarily disables `GL_SCISSOR_TEST`
///
/// On construction, snapshots the current `GL_SCISSOR_TEST` enable
/// state and disables it (no-op if already disabled). On destruction,
/// re-enables the scissor test if it was originally enabled.
///
/// Useful around operations such as full-target clears or blits where
/// an existing scissor rectangle would otherwise constrain the result.
///
////////////////////////////////////////////////////////////
class ZA_GLUTILS_API ScissorDisableGuard
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor: snapshot and disable the scissor test
    ///
    ////////////////////////////////////////////////////////////
    explicit ScissorDisableGuard();

    ////////////////////////////////////////////////////////////
    /// \brief Destructor: re-enable the scissor test if it was originally on
    ///
    ////////////////////////////////////////////////////////////
    ~ScissorDisableGuard();

    ScissorDisableGuard(const ScissorDisableGuard&) = delete;
    ScissorDisableGuard(ScissorDisableGuard&&)      = delete;

    ScissorDisableGuard& operator=(const ScissorDisableGuard&) = delete;
    ScissorDisableGuard& operator=(ScissorDisableGuard&&)      = delete;

private:
    bool m_savedState; //!< `true` if scissor test was enabled at construction time
};

////////////////////////////////////////////////////////////
/// \brief Bind a 2D texture and initialize its storage with default sampling state
///
/// Binds the given texture to `GL_TEXTURE_2D`, allocates RGBA storage
/// of the requested size (in either linear `GL_RGBA` or `GL_SRGB8_ALPHA8`
/// format) via `glTexImage2D`, and configures `GL_NEAREST` filtering
/// for both magnification and minification along with the requested
/// wrap mode for both axes. The pixel storage is left uninitialized
/// (a `nullptr` data pointer is passed).
///
/// \param textureId        ID of the texture to initialize
/// \param sRgb             If `true`, allocate the texture in sRGB color space
/// \param size             Width and height of the texture, in texels
/// \param textureWrapParam GL wrap mode for both `GL_TEXTURE_WRAP_S` and
///                         `GL_TEXTURE_WRAP_T` (e.g. `GL_CLAMP_TO_EDGE`)
///
////////////////////////////////////////////////////////////
ZA_GLUTILS_API void bindAndInitializeTexture(unsigned int textureId, bool sRgb, Vec2u size, unsigned int textureWrapParam);

} // namespace za::priv
