#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Geometry/Rect2.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Structure describing a glyph
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_GRAPHICS_API Glyph
{
    float  advance{};   //!< Offset to move horizontally to the next character
    Rect2f bounds;      //!< Bounding rectangle of the glyph, in coordinates relative to the baseline
    Rect2f textureRect; //!< Texture coordinates of the glyph inside the font's texture
};

} // namespace za


////////////////////////////////////////////////////////////
/// \struct za::Glyph
/// \ingroup graphics
///
/// A glyph is the visual representation of a character.
///
/// The `za::Glyph` structure provides the information needed
/// to handle the glyph:
/// \li its coordinates in the font's texture
/// \li its bounding rectangle
/// \li the offset to apply to get the starting position of the next glyph
///
/// \see `za::Font`
///
////////////////////////////////////////////////////////////
