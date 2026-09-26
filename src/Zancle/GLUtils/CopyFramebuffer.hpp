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
/// \brief Copy framebuffer contents between specified FBOs
///
/// Binds `srcFBO` to `GL_READ_FRAMEBUFFER` and `dstFBO` to
/// `GL_DRAW_FRAMEBUFFER` (asserting that both are framebuffer-complete
/// in debug builds), then delegates to `blitFramebuffer` to copy a
/// region of the requested size between them with optional Y-axis
/// inversion.
///
/// \param invertYAxis Whether to flip the source vertically during copy
/// \param size        Dimensions of the region to copy
/// \param srcFBO      Source framebuffer ID (`0` for the default framebuffer)
/// \param dstFBO      Destination framebuffer ID (`0` for the default framebuffer)
/// \param srcPos      Source region starting position (default: `{0, 0}`)
/// \param dstPos      Destination region starting position (default: `{0, 0}`)
///
/// \note Modifies the read/draw framebuffer bindings as a side effect.
///       Wrap the call in a `FramebufferSaver` if you need to preserve
///       the surrounding state.
///
////////////////////////////////////////////////////////////
ZA_GLUTILS_API void copyFramebuffer(
    bool         invertYAxis,
    Vec2u        size,
    unsigned int srcFBO,
    unsigned int dstFBO,
    Vec2u        srcPos = {0u, 0u},
    Vec2u        dstPos = {0u, 0u});

} // namespace za::priv
