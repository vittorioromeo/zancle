#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/GLUtils/Export.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Blit a rectangular region between framebuffers with optional Y-axis inversion
///
/// Copies the color buffer from the framebuffer currently bound to
/// `GL_READ_FRAMEBUFFER` into the framebuffer currently bound to
/// `GL_DRAW_FRAMEBUFFER`. Setting `invertYAxis` to `true` swaps the
/// source's vertical bounds, producing a vertically flipped result --
/// useful when copying between framebuffers that disagree on Y axis
/// orientation (e.g. flipping for screen capture).
///
/// \param invertYAxis Whether to flip the source region vertically during copy
/// \param src         Source region coordinates and dimensions
/// \param dst         Destination region coordinates and dimensions
///
/// \note Uses `GL_NEAREST` filtering -- appropriate for same-size copies.
///       The caller is responsible for binding the source and
///       destination framebuffers before calling.
///
////////////////////////////////////////////////////////////
ZA_GLUTILS_API void blitFramebuffer(bool invertYAxis, Rect2u src, Rect2u dst);

////////////////////////////////////////////////////////////
/// \brief Blit equal-sized regions between framebuffers with optional Y-axis inversion
///
/// Convenience overload that copies a region of identical size from
/// `srcPos` in the read framebuffer to `dstPos` in the draw framebuffer.
/// See the `Rect2u` overload for the underlying behavior.
///
/// \param invertYAxis Whether to flip the source region vertically during copy
/// \param size        Dimensions of the region to copy
/// \param srcPos      Source region starting position
/// \param dstPos      Destination region starting position
///
/// \note Uses `GL_NEAREST` filtering -- appropriate for same-size copies.
///       The caller is responsible for binding the source and
///       destination framebuffers before calling.
///
////////////////////////////////////////////////////////////
ZA_GLUTILS_API void blitFramebuffer(bool invertYAxis, Vec2u size, Vec2u srcPos, Vec2u dstPos);

} // namespace za::priv
