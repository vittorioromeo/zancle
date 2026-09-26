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
/// \brief Install the OpenGL debug-output callback
///
/// Enables `GL_DEBUG_OUTPUT` and `GL_DEBUG_OUTPUT_SYNCHRONOUS` on the
/// currently active context and registers an internal callback that
/// formats GL debug messages (source, type, severity, id, body) and
/// writes them to `priv::errMsg`. A small set of well-known noise
/// IDs (e.g. NVIDIA buffer hints) is filtered out.
///
/// Has no effect on Emscripten, where the GL debug-output extension is
/// unavailable.
///
////////////////////////////////////////////////////////////
ZA_GLUTILS_API void setupGLDebugCallback();

} // namespace za::priv
