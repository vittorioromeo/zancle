#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Chrono/StdChrono.hpp"
#include "Zancle/Chrono/TimeChronoUtil.hpp"

#include "Zancle/Base/IntTypes.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsStandardLayout.hpp"
#include "Zancle/Trait/IsTrivial.hpp"
#include "Zancle/Trait/IsTriviallyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyConstructible.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyMoveAssignable.hpp"
#include "Zancle/Trait/IsTriviallyMoveConstructible.hpp"

using namespace std::chrono_literals;

TEST_CASE("[System] za::Time")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Time));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Time));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Time));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Time));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Time));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Time));
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(za::Time));
        STATIC_CHECK(!ZA_IS_AGGREGATE(za::Time));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Time));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Time));
        STATIC_CHECK(ZA_IS_TRIVIALLY_ASSIGNABLE(za::Time, za::Time));
    }

    SECTION("Construction")
    {
        SECTION("Default constructor")
        {
            constexpr za::Time time;
            STATIC_CHECK(time.asSeconds() == 0.f);
            STATIC_CHECK(time.asMilliseconds() == 0);
            STATIC_CHECK(time.asMicroseconds() == 0);
        }

        SECTION("Construct from seconds")
        {
            constexpr za::Time time = za::seconds(123);
            STATIC_CHECK(time.asSeconds() == 123.f);
            STATIC_CHECK(time.asMilliseconds() == 123'000);
            STATIC_CHECK(time.asMicroseconds() == 123'000'000);

            STATIC_CHECK(za::seconds(1000.f).asMicroseconds() == 1'000'000'000);
            STATIC_CHECK(za::seconds(0.0000009f).asMicroseconds() == 0);
            STATIC_CHECK(za::seconds(0.0000001f).asMicroseconds() == 0);
            STATIC_CHECK(za::seconds(0.00000001f).asMicroseconds() == 0);
            STATIC_CHECK(za::seconds(0.000000001f).asMicroseconds() == 0);
            STATIC_CHECK(za::seconds(-0.000000001f).asMicroseconds() == 0);
            STATIC_CHECK(za::seconds(-0.00000001f).asMicroseconds() == 0);
            STATIC_CHECK(za::seconds(-0.0000001f).asMicroseconds() == 0);
            STATIC_CHECK(za::seconds(-0.0000009f).asMicroseconds() == 0);
            STATIC_CHECK(za::seconds(-1000.f).asMicroseconds() == -1'000'000'000);
        }

        SECTION("Construct from milliseconds")
        {
            constexpr za::Time time = za::milliseconds(42);
            CHECK(time.asSeconds() == Approx(0.042f));
            STATIC_CHECK(time.asMilliseconds() == 42);
            STATIC_CHECK(time.asMicroseconds() == 42'000);
        }

        SECTION("Construct from microseconds")
        {
            constexpr za::Time time = za::microseconds(987'654);
            CHECK(time.asSeconds() == Approx(0.987654f));
            STATIC_CHECK(time.asMilliseconds() == 987);
            STATIC_CHECK(time.asMicroseconds() == 987'654);
        }

        SECTION("Convert from chrono duration")
        {
            {
                constexpr za::Time time = za::TimeChronoUtil::fromDuration(3min);
                STATIC_CHECK(time.asSeconds() == 180.f);
                STATIC_CHECK(time.asMilliseconds() == 180'000);
                STATIC_CHECK(time.asMicroseconds() == 180'000'000);
            }
            {
                constexpr za::Time time = za::TimeChronoUtil::fromDuration(1s);
                STATIC_CHECK(time.asSeconds() == 1.f);
                STATIC_CHECK(time.asMilliseconds() == 1000);
                STATIC_CHECK(time.asMicroseconds() == 1'000'000);
            }
            {
                constexpr za::Time time = za::TimeChronoUtil::fromDuration(10ms);
                CHECK(time.asSeconds() == Approx(0.01f));
                STATIC_CHECK(time.asMilliseconds() == 10);
                STATIC_CHECK(time.asMicroseconds() == 10'000);
            }
            {
                constexpr za::Time time = za::TimeChronoUtil::fromDuration(2048us);
                CHECK(time.asSeconds() == Approx(0.002048f));
                STATIC_CHECK(time.asMilliseconds() == 2);
                STATIC_CHECK(time.asMicroseconds() == 2048);
            }
        }
    }

    SECTION("toDuration()")
    {
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::seconds(0)) == 0s);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::milliseconds(0)) == 0ms);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::microseconds(0)) == 0us);

        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::seconds(-1)) == -1s);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::milliseconds(-1)) == -1ms);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::microseconds(-1)) == -1us);

        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::seconds(1)) == 1s);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::milliseconds(1)) == 1ms);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::microseconds(1)) == 1us);

        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::seconds(-10)) == -10s);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::milliseconds(-10)) == -10ms);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::microseconds(-10)) == -10us);

        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::seconds(10)) == 10s);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::milliseconds(10)) == 10ms);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::microseconds(10)) == 10us);

        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::TimeChronoUtil::fromDuration(1s)) == 1s);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::TimeChronoUtil::fromDuration(1ms)) == 1ms);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::TimeChronoUtil::fromDuration(1us)) == 1us);
    }

    SECTION("Conversion to duration")
    {
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::seconds(0)) == 0s);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::milliseconds(0)) == 0ms);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::microseconds(0)) == 0us);

        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::seconds(-1)) == -1s);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::milliseconds(-1)) == -1ms);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::microseconds(-1)) == -1us);

        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::seconds(1)) == 1s);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::milliseconds(1)) == 1ms);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::microseconds(1)) == 1us);

        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::seconds(-10)) == -10s);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::milliseconds(-10)) == -10ms);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::microseconds(-10)) == -10us);

        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::seconds(10)) == 10s);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::milliseconds(10)) == 10ms);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::microseconds(10)) == 10us);

        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::TimeChronoUtil::fromDuration(1s)) == 1s);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::TimeChronoUtil::fromDuration(1ms)) == 1ms);
        STATIC_CHECK(za::TimeChronoUtil::toDuration(za::TimeChronoUtil::fromDuration(1us)) == 1us);
    }

    SECTION("Zero time")
    {
        STATIC_CHECK(za::Time{}.asSeconds() == 0.f);
        STATIC_CHECK(za::Time{}.asMilliseconds() == 0);
        STATIC_CHECK(za::Time{}.asMicroseconds() == 0);
    }

    SECTION("Operators")
    {
        SECTION("operator==")
        {
            STATIC_CHECK(za::Time() == za::Time());
            STATIC_CHECK(za::seconds(1) == za::seconds(1));
            STATIC_CHECK(za::seconds(10) == za::milliseconds(10'000));
            STATIC_CHECK(za::milliseconds(450'450) == za::microseconds(450'450'000));
            STATIC_CHECK(za::seconds(0.5f) == za::microseconds(500'000));
        }

        SECTION("operator!=")
        {
            STATIC_CHECK(za::seconds(10.12f) != za::milliseconds(10'121));
            STATIC_CHECK(za::microseconds(123'456) != za::milliseconds(123));
        }

        SECTION("operator<")
        {
            STATIC_CHECK(za::seconds(54.999f) < za::seconds(55));
            STATIC_CHECK(za::microseconds(10) < za::milliseconds(10));
            STATIC_CHECK(za::milliseconds(1000) < za::microseconds(1'000'001));
        }

        SECTION("operator>")
        {
            STATIC_CHECK(za::seconds(55.001f) > za::seconds(55));
            STATIC_CHECK(za::microseconds(1) > za::seconds(0.0000001f));
            STATIC_CHECK(za::microseconds(1'000'001) > za::milliseconds(1000));
        }

        SECTION("operator<=")
        {
            STATIC_CHECK(za::milliseconds(100) <= za::milliseconds(100));
            STATIC_CHECK(za::seconds(0.0012f) <= za::microseconds(1201));
        }

        SECTION("operator>=")
        {
            STATIC_CHECK(za::milliseconds(100) >= za::milliseconds(-100));
            STATIC_CHECK(za::microseconds(1201) >= za::seconds(0.0012f));
        }

        SECTION("operator-")
        {
            STATIC_CHECK(za::seconds(-1) == -za::seconds(1));
            STATIC_CHECK(za::microseconds(1234) == -za::microseconds(-1234));
        }

        SECTION("operator+")
        {
            STATIC_CHECK(za::seconds(1) + za::seconds(1) == za::seconds(2));
            STATIC_CHECK(za::milliseconds(400) + za::microseconds(400) == za::microseconds(400'400));
        }

        SECTION("operator+=")
        {
            za::Time time = za::seconds(1.5f);
            time += za::seconds(1);
            CHECK(time == za::seconds(2.5f));
        }

        SECTION("operator-")
        {
            STATIC_CHECK(za::seconds(1) - za::seconds(1) == za::seconds(0));
            STATIC_CHECK(za::milliseconds(400) - za::microseconds(400) == za::microseconds(399'600));
        }

        SECTION("operator-=")
        {
            za::Time time = za::seconds(1.5f);
            time -= za::seconds(10);
            CHECK(time == za::seconds(-8.5f));
        }

        SECTION("operator*")
        {
            STATIC_CHECK(za::seconds(1) * 2.f == za::seconds(2));
            STATIC_CHECK(za::seconds(12) * 0.5f == za::seconds(6));
            STATIC_CHECK(za::seconds(1) * za::I64{2} == za::seconds(2));
            STATIC_CHECK(za::seconds(42) * za::I64{2} == za::seconds(84));
            STATIC_CHECK(2.f * za::seconds(1) == za::seconds(2));
            STATIC_CHECK(0.5f * za::seconds(12) == za::seconds(6));
            STATIC_CHECK(za::I64{2} * za::seconds(1) == za::seconds(2));
            STATIC_CHECK(za::I64{2} * za::seconds(42) == za::seconds(84));
        }

        SECTION("operator*=")
        {
            za::Time time = za::milliseconds(1000);
            time *= za::I64{10};
            CHECK(time == za::milliseconds(10'000));
            time *= 0.1f;
            CHECK(time.asMilliseconds() == 1000);
        }

        SECTION("operator/")
        {
            STATIC_CHECK(za::seconds(1) / 2.f == za::seconds(0.5f));
            STATIC_CHECK(za::seconds(12) / 0.5f == za::seconds(24));
            STATIC_CHECK(za::seconds(1) / za::I64{2} == za::seconds(0.5f));
            STATIC_CHECK(za::seconds(42) / za::I64{2} == za::seconds(21));
            STATIC_CHECK(za::seconds(1) / za::seconds(1) == 1.f);
            CHECK(za::milliseconds(10) / za::microseconds(1) == Approx(10'000.f));
        }

        SECTION("operator/=")
        {
            za::Time time = za::milliseconds(1000);
            time /= za::I64{2};
            CHECK(time == za::milliseconds(500));
            time /= 0.5f;
            CHECK(time.asMilliseconds() == 1000);
        }

        SECTION("operator%")
        {
            STATIC_CHECK(za::seconds(10) % za::seconds(3) == za::seconds(1));
            STATIC_CHECK(za::milliseconds(100) % za::microseconds(10) == za::seconds(0));
        }

        SECTION("operator%=")
        {
            za::Time time = za::milliseconds(100);
            time %= za::milliseconds(99);
            CHECK(time == za::milliseconds(1));
        }
    }

    SECTION("Constexpr support")
    {
        constexpr auto result = []
        {
            za::Time time = za::milliseconds(100);
            time %= za::milliseconds(99);
            return time;
        }();

        STATIC_CHECK(result == za::milliseconds(1));
    }
}
