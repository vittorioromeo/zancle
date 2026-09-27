// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Chrono/Clock.hpp"

#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Base/IntTypes.hpp"

#if defined(ZA_SYSTEM_WINDOWS)
    #include "Zancle/Base/WindowsHeader.hpp"
#else
    #include <time.h>
#endif


namespace za::priv
{
namespace
{
////////////////////////////////////////////////////////////
/// \brief Read the OS monotonic clock, in nanoseconds since an unspecified epoch
///
/// The clocks are the ones `std::chrono::steady_clock` uses on each
/// platform, so behavior is unchanged from the previous `<chrono>`-based
/// implementation:
///
/// - Windows: `QueryPerformanceCounter` (as in MSVC's STL, and in MinGW's
///   `clock_gettime(CLOCK_MONOTONIC)`).
/// - macOS/iOS: `CLOCK_UPTIME_RAW` (as in libc++); does not advance while
///   the system is asleep.
/// - Linux, BSDs, Android, Emscripten: `clock_gettime(CLOCK_MONOTONIC)` (as
///   in libstdc++ and libc++); on Emscripten this is `performance.now()`,
///   whose resolution is coarsened by browsers (typically 5-100us).
///
/// Android builds can opt into `CLOCK_BOOTTIME` with
/// `ZA_ANDROID_USE_SUSPEND_AWARE_CLOCK`, which keeps advancing while the
/// device is suspended.
///
/// For more information on Linux clocks visit:
/// https://man7.org/linux/man-pages/man2/clock_gettime.2.html
///
////////////////////////////////////////////////////////////
[[nodiscard]] I64 monotonicNanoseconds() noexcept
{
#if defined(ZA_SYSTEM_WINDOWS)

    // Fixed at boot, and cannot fail on Windows XP or later
    static const I64 frequency = []
    {
        LARGE_INTEGER result;
        QueryPerformanceFrequency(&result);
        return static_cast<I64>(result.QuadPart);
    }();

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    const auto ticks = static_cast<I64>(counter.QuadPart);

    // Windows 10+ always reports 10 MHz: avoid the divisions
    if (frequency == 10'000'000) [[likely]]
        return ticks * 100;

    // Split to avoid overflowing `ticks * 1e9` (the remainder is below `frequency`)
    return (ticks / frequency) * 1'000'000'000 + (ticks % frequency) * 1'000'000'000 / frequency;

#elif defined(ZA_SYSTEM_MACOS) || defined(ZA_SYSTEM_IOS)

    return static_cast<I64>(clock_gettime_nsec_np(CLOCK_UPTIME_RAW));

#else

    #if defined(ZA_SYSTEM_ANDROID) && defined(ZA_ANDROID_USE_SUSPEND_AWARE_CLOCK)
    constexpr clockid_t clockId = CLOCK_BOOTTIME;
    #else
    constexpr clockid_t clockId = CLOCK_MONOTONIC;
    #endif

    timespec ts{};
    clock_gettime(clockId, &ts);

    return static_cast<I64>(ts.tv_sec) * 1'000'000'000 + static_cast<I64>(ts.tv_nsec);

#endif
}


////////////////////////////////////////////////////////////
/// \brief `stopPoint` value of a running clock (never returned by `monotonicNanoseconds`)
///
////////////////////////////////////////////////////////////
constexpr I64 clockRunning = -9'223'372'036'854'775'807 - 1;


////////////////////////////////////////////////////////////
[[nodiscard]] constexpr Time nanosecondsToTime(const I64 nanoseconds)
{
    return microseconds(nanoseconds / 1000); // truncates toward zero, like `std::chrono::duration_cast`
}

} // namespace
} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
struct Clock::Impl
{
    I64 refPoint{priv::monotonicNanoseconds()}; //!< Time of last reset, in nanoseconds
    I64 stopPoint{priv::clockRunning};          //!< Time of last stop, in nanoseconds
};


////////////////////////////////////////////////////////////
Clock::Clock()                            = default;
Clock::~Clock()                           = default;
Clock::Clock(const Clock&)                = default;
Clock& Clock::operator=(const Clock&)     = default;
Clock::Clock(Clock&&) noexcept            = default;
Clock& Clock::operator=(Clock&&) noexcept = default;


////////////////////////////////////////////////////////////
Time Clock::getElapsedTime() const
{
    const I64 endPoint = isRunning() ? priv::monotonicNanoseconds() : m_impl->stopPoint;
    return priv::nanosecondsToTime(endPoint - m_impl->refPoint);
}


////////////////////////////////////////////////////////////
bool Clock::isRunning() const
{
    return m_impl->stopPoint == priv::clockRunning;
}


////////////////////////////////////////////////////////////
void Clock::start()
{
    if (isRunning())
        return;

    m_impl->refPoint += priv::monotonicNanoseconds() - m_impl->stopPoint;
    m_impl->stopPoint = priv::clockRunning;
}


////////////////////////////////////////////////////////////
void Clock::stop()
{
    if (!isRunning())
        return;

    m_impl->stopPoint = priv::monotonicNanoseconds();
}


////////////////////////////////////////////////////////////
Time Clock::restart()
{
    const Time elapsed = getElapsedTime();
    m_impl->refPoint   = priv::monotonicNanoseconds();
    m_impl->stopPoint  = priv::clockRunning;
    return elapsed;
}


////////////////////////////////////////////////////////////
Time Clock::reset()
{
    const Time elapsed = getElapsedTime();
    m_impl->refPoint   = priv::monotonicNanoseconds();
    m_impl->stopPoint  = m_impl->refPoint;
    return elapsed;
}


////////////////////////////////////////////////////////////
Time Clock::now()
{
    return priv::nanosecondsToTime(priv::monotonicNanoseconds());
}

} // namespace za
