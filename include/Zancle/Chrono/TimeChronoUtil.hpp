#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include "Zancle/Config.hpp"

#include "Zancle/Chrono/StdChrono.hpp"
#include "Zancle/Chrono/Time.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Utilities for converting between `za::Time` and `<chrono>` durations
///
////////////////////////////////////////////////////////////
class ZA_SYSTEM_API TimeChronoUtil
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Convert any `std::chrono::duration` to the equivalent `za::Time`
    ///
    ////////////////////////////////////////////////////////////
    template <typename Rep, typename Period>
    [[nodiscard]] static constexpr Time fromDuration(const std::chrono::duration<Rep, Period>& duration)
    {
        return microseconds(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
    }


    ////////////////////////////////////////////////////////////
    /// \brief Convert `time` to a `std::chrono::microseconds` duration
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static constexpr std::chrono::microseconds toDuration(const Time time)
    {
        return std::chrono::microseconds(time.asMicroseconds());
    }


    ////////////////////////////////////////////////////////////
    /// \brief Convert `time` to a custom `std::chrono::duration<Rep, Period>`
    ///
    /// Conversions to a coarser integer duration truncate toward zero,
    /// like `std::chrono::duration_cast`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename Rep, typename Period>
    [[nodiscard]] static constexpr std::chrono::duration<Rep, Period> toCustomDuration(const Time time)
    {
        return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(toDuration(time));
    }
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::TimeChronoUtil
/// \ingroup system
///
/// The `za::TimeChronoUtil` class provides static helper functions to simplify
/// the conversion between Zancle's time representation (`za::Time`) and the
/// standard C++ time library (`<chrono>`). This allows for easier integration
/// with other libraries or code that uses `std::chrono`.
///
/// \see za::Time, za::Clock
///
////////////////////////////////////////////////////////////
