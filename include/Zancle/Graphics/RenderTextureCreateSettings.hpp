#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/DepthStencilFormat.hpp"
#include "Zancle/Graphics/TextureWrapMode.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Settings used to create the framebuffer object backing a `za::RenderTexture`
///
/// All members default to sensible values, so the struct can usually
/// be passed empty (e.g. via designated initializers) and only the
/// fields that need to differ from their defaults need to be set.
///
////////////////////////////////////////////////////////////
struct RenderTextureCreateSettings
{
    bool            sRgbCapable{false};               //!< Whether the texture should use sRGB encoding
    bool            smooth{false};                    //!< Whether linear filtering should be enabled on the texture
    TextureWrapMode wrapMode{TextureWrapMode::Clamp}; //!< Wrap mode used when sampling the attached texture

    unsigned int       sampleCount{0u}; //!< Multisampling (MSAA) sample count (0 or 1 = disabled)
    DepthStencilFormat depthStencilFormat{DepthStencilFormat::None}; //!< Depth/stencil attachment format
};

} // namespace za
