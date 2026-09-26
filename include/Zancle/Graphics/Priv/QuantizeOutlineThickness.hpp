#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/IntTypes.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Quantize an outline thickness to 1/64th-pixel units
///
/// Shared by `za::Font` (glyph cache key) and `za::FontFace`
/// (`FT_Stroker` radius): both MUST quantize identically,
/// otherwise two thicknesses could share a cache key yet
/// rasterize differently (or vice versa).
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline za::I32 quantizeOutlineThickness(const float outlineThickness)
{
    const auto quantized = static_cast<za::I32>(outlineThickness * float{1 << 6});

    // Round away from zero: a non-zero thickness must never quantize to `0`,
    // because `0` is the fill-glyph cache key -- thicknesses in `(0, 1/64)`
    // would otherwise poison the fill cache slot with a stroked glyph
    if (quantized == 0 && outlineThickness != 0.f)
        return outlineThickness > 0.f ? 1 : -1;

    return quantized;
}

} // namespace za::priv
