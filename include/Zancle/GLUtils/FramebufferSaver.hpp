#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/GLUtils/Export.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief RAII guard that saves and restores the current framebuffer binding
///
/// On construction, queries and stores the IDs currently bound to
/// `GL_DRAW_FRAMEBUFFER` and `GL_READ_FRAMEBUFFER`. On destruction, the
/// two binding points are restored to their original IDs. Useful when
/// a function needs to temporarily bind its own framebuffer without
/// disturbing the surrounding rendering state.
///
////////////////////////////////////////////////////////////
class ZA_GLUTILS_API FramebufferSaver
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor: snapshot the current framebuffer bindings
    ///
    ////////////////////////////////////////////////////////////
    FramebufferSaver();

    ////////////////////////////////////////////////////////////
    /// \brief Destructor: restore the previously saved framebuffer bindings
    ///
    ////////////////////////////////////////////////////////////
    ~FramebufferSaver();

    ////////////////////////////////////////////////////////////
    FramebufferSaver(const FramebufferSaver&)            = delete;
    FramebufferSaver& operator=(const FramebufferSaver&) = delete;

    ////////////////////////////////////////////////////////////
    FramebufferSaver(FramebufferSaver&&)            = delete;
    FramebufferSaver& operator=(FramebufferSaver&&) = delete;

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    int m_drawFramebufferBinding; //!< Saved `GL_DRAW_FRAMEBUFFER_BINDING` to restore
    int m_readFramebufferBinding; //!< Saved `GL_READ_FRAMEBUFFER_BINDING` to restore
};

} // namespace za::priv
