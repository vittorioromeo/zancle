#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Chrono/Clock.hpp"

#include "Zancle/Concurrency/Thread.hpp"

#include "Zancle/Chrono/StdChrono.hpp"
#include "Zancle/Chrono/Time.hpp"
#include "Zancle/Chrono/TimeChronoUtil.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"


TEST_CASE("[System] za::Clock")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Clock));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Clock));
        STATIC_CHECK(sizeof(za::Clock) == 16u);

        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::Clock));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::Clock));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::Clock));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::Clock));
    }

    SECTION("Construction")
    {
        const za::Clock clock;
        CHECK(clock.isRunning());
        CHECK(clock.getElapsedTime() >= za::microseconds(0));
    }

    SECTION("getElapsedTime()")
    {
        const za::Clock clock;
        CHECK(clock.getElapsedTime() >= za::microseconds(0));
        const auto elapsed = clock.getElapsedTime();
        za::ThisThread::sleepFor(za::milliseconds(1));
        CHECK(clock.getElapsedTime() > elapsed);
    }

    SECTION("start/stop")
    {
        za::Clock clock;
        clock.stop();
        CHECK(!clock.isRunning());
        const auto elapsed = clock.getElapsedTime();
        za::ThisThread::sleepFor(za::milliseconds(1));
        CHECK(elapsed == clock.getElapsedTime());

        clock.start();
        CHECK(clock.isRunning());
        CHECK(clock.getElapsedTime() >= elapsed);
    }

    SECTION("restart()")
    {
        za::Clock clock;
        CHECK(clock.restart() >= za::microseconds(0));
        CHECK(clock.isRunning());
        za::ThisThread::sleepFor(za::milliseconds(1));
        const auto elapsed = clock.restart();
        CHECK(clock.restart() < elapsed);
    }

    SECTION("reset()")
    {
        za::Clock clock;
        CHECK(clock.reset() >= za::microseconds(0));
        CHECK(!clock.isRunning());
    }

    SECTION("Consecutive restarts add up to the real elapsed time")
    {
        // Previously, each `restart()` dropped the sub-microsecond remainder and the time
        // between two clock readings: ~0.5us per call, i.e. ~5ms over 10'000 calls
        const za::Clock reference;
        za::Clock       clock;

        za::Time sum;
        for (int i = 0; i < 10'000; ++i)
            sum += clock.restart();

        const za::Time total = reference.getElapsedTime();

        CHECK(sum <= total);
        CHECK(total - sum < za::microseconds(500));
    }

    SECTION("restart() and reset() of a stopped clock")
    {
        za::Clock clock;
        clock.stop();
        const za::Time elapsed = clock.getElapsedTime();

        za::ThisThread::sleepFor(za::milliseconds(1));
        CHECK(clock.reset() == elapsed); // stopped: the sleep is not counted
        CHECK(!clock.isRunning());
        CHECK(clock.getElapsedTime() == za::Time{}); // only a sub-microsecond remainder is left

        CHECK(clock.restart() == za::Time{});
        CHECK(clock.isRunning());
    }

    SECTION("now() never goes backward")
    {
        za::Time previous      = za::Clock::now();
        bool     nonDecreasing = true;

        for (int i = 0; i < 100'000; ++i)
        {
            const za::Time current = za::Clock::now();
            nonDecreasing &= current >= previous;
            previous = current;
        }

        CHECK(nonDecreasing);
    }

    SECTION("Agrees with std::chrono::steady_clock")
    {
        const auto stdBegin = std::chrono::steady_clock::now();
        const auto zaBegin  = za::Clock::now();

        const za::Clock clock;
        za::ThisThread::sleepFor(za::milliseconds(50));

        const za::Time clockElapsed = clock.getElapsedTime();
        const za::Time nowElapsed   = za::Clock::now() - zaBegin;
        const za::Time stdElapsed   = za::TimeChronoUtil::fromDuration(std::chrono::steady_clock::now() - stdBegin);

        // Both measurements happen within the `std::chrono` interval
        const za::Time tolerance = za::milliseconds(2);

        CHECK(clockElapsed >= za::milliseconds(50));
        CHECK(clockElapsed <= stdElapsed + za::microseconds(1));
        CHECK(stdElapsed - clockElapsed < tolerance);

        CHECK(nowElapsed >= za::milliseconds(50));
        CHECK(nowElapsed <= stdElapsed + za::microseconds(1));
        CHECK(stdElapsed - nowElapsed < tolerance);
    }
}
