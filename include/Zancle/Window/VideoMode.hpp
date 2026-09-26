#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Window/Export.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Base/RequireDesignatedInitializers.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief VideoMode defines a video mode (size, bpp)
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_WINDOW_API VideoMode
{
    ZA_REQUIRE_DESIGNATED_INITIALIZERS;

    ////////////////////////////////////////////////////////////
    /// \brief Tell whether or not the video mode is valid
    ///
    /// The validity of video modes is only relevant when using
    /// fullscreen windows; otherwise any video mode can be used
    /// with no restriction.
    ///
    /// \return `true` if the video mode is valid for fullscreen mode
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isValid() const;

    ////////////////////////////////////////////////////////////
    /// \relates VideoMode
    /// \brief Overload of `operator==` to compare two video modes
    ///
    /// \param rhs Right operand
    ///
    /// \return `true` if modes are equal
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool operator==(const VideoMode& rhs) const = default;


    ////////////////////////////////////////////////////////////
    /// \relates VideoMode
    /// \brief Overload of `operator<` to compare video modes
    ///
    /// \param lhs  Left operand (a video mode)
    /// \param rhs Right operand (a video mode)
    ///
    /// \return `true` if `lhs` is lesser than `rhs`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] friend constexpr bool operator<(const VideoMode& lhs, const VideoMode& rhs)
    {
        const auto vecLessThan = [](const Vec2u& vecLhs, const Vec2u& vecRhs) -> bool
        {
            if (vecLhs.x == vecRhs.x)
                return vecLhs.y < vecRhs.y;

            return vecLhs.x < vecRhs.x;
        };

        if (lhs.bitsPerPixel < rhs.bitsPerPixel)
            return true;

        if (rhs.bitsPerPixel < lhs.bitsPerPixel)
            return false;

        if (vecLessThan(lhs.size, rhs.size))
            return true;

        if (vecLessThan(rhs.size, lhs.size))
            return false;

        if (lhs.pixelDensity < rhs.pixelDensity)
            return true;

        if (rhs.pixelDensity < lhs.pixelDensity)
            return false;

        if (lhs.refreshRate < rhs.refreshRate)
            return true;

        return false;
    }


    ////////////////////////////////////////////////////////////
    /// \relates VideoMode
    /// \brief Overload of `operator>` to compare video modes
    ///
    /// \param lhs  Left operand (a video mode)
    /// \param rhs Right operand (a video mode)
    ///
    /// \return `true` if `lhs` is greater than `rhs`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] friend constexpr bool operator>(const VideoMode& lhs, const VideoMode& rhs)
    {
        return rhs < lhs;
    }


    ////////////////////////////////////////////////////////////
    /// \relates VideoMode
    /// \brief Overload of `operator<=` to compare video modes
    ///
    /// \param lhs  Left operand (a video mode)
    /// \param rhs Right operand (a video mode)
    ///
    /// \return `true` if `lhs` is lesser or equal than `rhs`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] friend constexpr bool operator<=(const VideoMode& lhs, const VideoMode& rhs)
    {
        return !(rhs < lhs);
    }


    ////////////////////////////////////////////////////////////
    /// \relates VideoMode
    /// \brief Overload of `operator>=` to compare video modes
    ///
    /// \param lhs  Left operand (a video mode)
    /// \param rhs Right operand (a video mode)
    ///
    /// \return `true` if `lhs` is greater or equal than `rhs`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] friend constexpr bool operator>=(const VideoMode& lhs, const VideoMode& rhs)
    {
        return !(lhs < rhs);
    }


    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    Vec2u        size;              //!< Video mode width and height, in pixels
    unsigned int bitsPerPixel{32u}; //!< Video mode pixel depth, in bits per pixels
    float        pixelDensity{1.f}; //!< Video mode pixel density multiplier
    float        refreshRate{60.f}; //!< Video mode refresh rate, in Hz
};

} // namespace za


////////////////////////////////////////////////////////////
/// \struct za::VideoMode
/// \ingroup window
///
/// A video mode is the description of a display configuration:
/// resolution (in pixels), bit depth (in bits per pixel),
/// pixel density (HiDPI scale factor), and refresh rate.
///
/// The main use of video modes is fullscreen window creation:
/// when going fullscreen, the requested mode must be one of
/// the modes that the OS reports as valid, otherwise window
/// creation will fail. The list of valid modes is queried via
/// `za::VideoModeUtils::getFullscreenModes`, the current
/// desktop mode via `za::VideoModeUtils::getDesktopMode`, and
/// a custom mode can be tested with `isValid()`.
///
/// `VideoMode` itself is a plain aggregate: feel free to
/// construct one with brace-init.
///
/// Usage example:
/// \code
/// // List all the video modes available for fullscreen
/// for (const za::VideoMode& mode : za::VideoModeUtils::getFullscreenModes())
/// {
///     // mode.size.x, mode.size.y, mode.bitsPerPixel, mode.refreshRate, ...
/// }
///
/// // Create window settings using the desktop's pixel depth
/// const za::VideoMode desktop = za::VideoModeUtils::getDesktopMode();
/// za::WindowSettings settings{
///     .size         = {1024u, 768u},
///     .bitsPerPixel = desktop.bitsPerPixel,
///     .title        = "Zancle window",
/// };
/// \endcode
///
/// \see `za::VideoModeUtils`, `za::WindowSettings`
///
////////////////////////////////////////////////////////////
