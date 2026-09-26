#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/IntTypes.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Depth/stencil attachment format for a `za::RenderTexture`
///
////////////////////////////////////////////////////////////
enum class [[nodiscard]] DepthStencilFormat : za::U8
{
    None,            //!< No depth/stencil attachment
    Depth16,         //!< 16-bit depth attachment, no stencil (`GL_DEPTH_COMPONENT16`)
    Depth24,         //!< 24-bit depth attachment, no stencil (`GL_DEPTH_COMPONENT24`)
    Stencil8,        //!< 8-bit stencil attachment, no depth (`GL_STENCIL_INDEX8`)
    Depth24Stencil8, //!< Packed 24-bit depth + 8-bit stencil attachment (`GL_DEPTH24_STENCIL8`)
};

} // namespace za


////////////////////////////////////////////////////////////
/// \enum za::DepthStencilFormat
/// \ingroup graphics
///
/// `za::DepthStencilFormat` selects the depth/stencil attachment
/// backing a `za::RenderTexture`, modeling the renderbuffer formats
/// guaranteed to be framebuffer-attachable by OpenGL ES 3.0.
///
/// Depth and stencil are combined in a single enumeration because,
/// when both are needed, GL requires a single packed depth/stencil
/// image on the framebuffer -- separate depth and stencil attachments
/// are not portably supported.
///
/// \li `None` -- no attachment; depth testing and stencil operations
///     are unavailable on the render texture.
/// \li `Depth16` / `Depth24` -- depth-only attachment, e.g. for 3D
///     content rendered into the texture.
/// \li `Stencil8` -- stencil-only attachment, e.g. for masked 2D
///     rendering via `za::StencilMode`.
/// \li `Depth24Stencil8` -- packed attachment providing both.
///
/// The format is chosen at creation time via
/// `za::RenderTextureCreateSettings::depthStencilFormat` and cannot
/// be changed afterwards.
///
/// \see `za::RenderTexture`, `za::RenderTextureCreateSettings`, `za::StencilMode`
///
////////////////////////////////////////////////////////////
