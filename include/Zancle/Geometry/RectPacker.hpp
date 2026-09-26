#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/InPlacePImpl.hpp"
#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/Span.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Packs 2D rectangles into a larger fixed-size area (e.g. a texture atlas)
///
/// Non-copyable and non-movable: the internals require address stability.
///
////////////////////////////////////////////////////////////
class RectPacker
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Construct a packer over a bin of the given `(width, height)`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit RectPacker(Vec2u size);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~RectPacker();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    RectPacker(const RectPacker& rhs) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment operator
    ///
    ////////////////////////////////////////////////////////////
    RectPacker& operator=(const RectPacker& rhs) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted move constructor
    ///
    ////////////////////////////////////////////////////////////
    RectPacker(RectPacker&& rhs) noexcept = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted move assignment operator
    ///
    ////////////////////////////////////////////////////////////
    RectPacker& operator=(RectPacker&& rhs) noexcept = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Try to pack a rectangle of `rectSize` (both dimensions must be > 0)
    ///
    /// \return Top-left position of the packed rectangle, or `nullOpt` if it doesn't fit
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<Vec2u> pack(Vec2u rectSize);

    ////////////////////////////////////////////////////////////
    /// \brief Try to pack many rectangles at once; `outPositions` must have the same size as `rectSizes`
    ///
    /// On success, `outPositions` is filled with the top-left of each packed
    /// rectangle. Each entry in `rectSizes` must have both dimensions > 0.
    ///
    /// All-or-nothing: on failure, no space is consumed (the packer is left
    /// unchanged) and the contents of `outPositions` are unspecified.
    ///
    /// \return `true` if all rectangles fit, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool packMultiple(za::Span<Vec2u> outPositions, za::Span<const Vec2u> rectSizes);

    ////////////////////////////////////////////////////////////
    /// \brief Size of the bin
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vec2u getSize() const;

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 128> m_impl; //!< Implementation details
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::RectPacker
/// \ingroup system
///
/// `za::RectPacker` arranges smaller rectangles within a larger fixed-size
/// bin without overlap, e.g. for building texture atlases.
///
/// Construct with the bin dimensions, then call `pack()` for each rectangle
/// to receive its assigned top-left position (or `nullOpt` if it doesn't fit).
///
/// \see za::Rect2
///
////////////////////////////////////////////////////////////
