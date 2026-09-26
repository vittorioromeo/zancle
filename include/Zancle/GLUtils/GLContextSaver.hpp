#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/GLUtils/Export.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za::priv
{
class GlContext;
} // namespace za::priv


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief RAII guard that saves and restores the active thread-local GL context
///
/// On construction, captures a pointer to the GL context that is
/// currently active on the calling thread (asserts that one exists).
/// On destruction, re-activates that context, restoring whatever state
/// existed before code in between possibly switched to a different
/// context (for example via `GLSharedContextGuard`).
///
////////////////////////////////////////////////////////////
class ZA_GLUTILS_API GLContextSaver
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor: snapshot the active GL context pointer
    ///
    ////////////////////////////////////////////////////////////
    GLContextSaver();

    ////////////////////////////////////////////////////////////
    /// \brief Destructor: re-activate the previously saved GL context
    ///
    ////////////////////////////////////////////////////////////
    ~GLContextSaver();

    ////////////////////////////////////////////////////////////
    GLContextSaver(const GLContextSaver&)            = delete;
    GLContextSaver& operator=(const GLContextSaver&) = delete;

    ////////////////////////////////////////////////////////////
    GLContextSaver(GLContextSaver&&)            = delete;
    GLContextSaver& operator=(GLContextSaver&&) = delete;

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    priv::GlContext* m_glContext; //!< Saved active context, restored on destruction
};

} // namespace za::priv
