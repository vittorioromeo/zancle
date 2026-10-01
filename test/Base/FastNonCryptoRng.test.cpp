#include "Tst/Tst.hpp"

#include "Zancle/Random/FastNonCryptoRng.hpp"

#include "Zancle/Random/Xoroshiro128PlusPlusBitGenerator.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Base/IntTypes.hpp"

#include <limits>
#include <random>

#include <cmath>


namespace
{
namespace FastNonCryptoRngTest // for unity builds
{
////////////////////////////////////////////////////////////
// Reference values from the authors' implementation (https://prng.di.unimi.it/xoroshiro128plusplus.c),
// seeded through their SplitMix64 (https://prng.di.unimi.it/splitmix64.c)
constexpr za::U64 seed42Outputs[] = {16'756'476'715'040'848'931ull,
                                     6'098'722'386'207'918'385ull,
                                     17'541'662'578'032'534'341ull,
                                     3'771'828'211'556'203'317ull};


////////////////////////////////////////////////////////////
[[nodiscard]] constexpr bool matchesSeed42Reference()
{
    za::Xoroshiro128PlusPlusBitGenerator generator{42u};

    for (const za::U64 expected : seed42Outputs)
        if (generator.next() != expected)
            return false;

    return true;
}


////////////////////////////////////////////////////////////
[[nodiscard]] constexpr bool usableInConstantExpressions()
{
    za::FastNonCryptoRng rng{7u};

    const int   i = rng.getI(-5, 5);
    const float f = rng.getF(-1.f, 1.f);
    rng.jump();

    return i >= -5 && i <= 5 && f >= -1.f && f <= 1.f && rng.getI(3u, 3u) == 3u;
}

} // namespace FastNonCryptoRngTest
} // namespace


TEST_CASE("[Base] Xoroshiro128PlusPlusBitGenerator")
{
    SECTION("Matches the reference implementation")
    {
        STATIC_CHECK(FastNonCryptoRngTest::matchesSeed42Reference());

        za::Xoroshiro128PlusPlusBitGenerator seed0{0u};
        CHECK(seed0.next() == 8'027'914'721'839'836'897ull);
        CHECK(seed0.next() == 13'805'533'416'164'201'645ull);
        CHECK(seed0.next() == 5'256'508'173'613'850'168ull);

        za::Xoroshiro128PlusPlusBitGenerator seedDeadBeef{0xDE'AD'BE'EFu};
        CHECK(seedDeadBeef.next() == 10'475'669'486'819'360'267ull);
        CHECK(seedDeadBeef.next() == 1'730'990'681'042'731'327ull);

        za::Xoroshiro128PlusPlusBitGenerator defaultSeeded;
        CHECK(defaultSeeded.next() == 17'081'294'481'478'417'899ull);
        CHECK(defaultSeeded.next() == 2'902'333'387'679'125'420ull);
    }

    SECTION("jump and longJump match the reference implementation")
    {
        za::Xoroshiro128PlusPlusBitGenerator jumped{42u};
        jumped.jump();
        CHECK(jumped.next() == 16'052'925'335'932'940'643ull);
        CHECK(jumped.next() == 13'241'858'892'588'731'496ull);

        za::Xoroshiro128PlusPlusBitGenerator longJumped{42u};
        longJumped.longJump();
        CHECK(longJumped.next() == 14'755'487'393'135'113'647ull);
        CHECK(longJumped.next() == 2'246'633'215'492'153'765ull);
    }

    SECTION("UniformRandomBitGenerator")
    {
        STATIC_CHECK(std::uniform_random_bit_generator<za::Xoroshiro128PlusPlusBitGenerator>);

        za::Xoroshiro128PlusPlusBitGenerator generator{1u};
        std::uniform_int_distribution<int>   distribution{1, 6};

        for (int i = 0; i < 1000; ++i)
        {
            const int roll = distribution(generator);
            CHECK(roll >= 1);
            CHECK(roll <= 6);
        }
    }
}


TEST_CASE("[Base] FastNonCryptoRng")
{
    STATIC_CHECK(FastNonCryptoRngTest::usableInConstantExpressions());

    SECTION("Deterministic, and `next` is the engine's output")
    {
        za::FastNonCryptoRng a{42u};
        za::FastNonCryptoRng b{42u};

        for (const za::U64 expected : FastNonCryptoRngTest::seed42Outputs)
        {
            CHECK(a.next() == expected);
            CHECK(b.next() == expected);
        }
    }

    SECTION("jump creates a non-overlapping stream")
    {
        za::FastNonCryptoRng original{3u};
        za::FastNonCryptoRng copy = original;
        copy.jump();

        int equal = 0;
        for (int i = 0; i < 10'000; ++i)
            equal += original.next() == copy.next() ? 1 : 0;

        CHECK(equal == 0);
    }

    SECTION("getI stays within bounds, for every integer type")
    {
        za::FastNonCryptoRng rng{1u};

        for (int i = 0; i < 10'000; ++i)
        {
            const int v = rng.getI(-7, 13);
            CHECK(v >= -7);
            CHECK(v <= 13);

            const za::U8 u8 = rng.getI(za::U8{10}, za::U8{20});
            CHECK(u8 >= 10);
            CHECK(u8 <= 20);

            const za::I64 i64 = rng.getI(za::I64{-1'000'000'000'000}, za::I64{-999'999'999'000});
            CHECK(i64 >= -1'000'000'000'000);
            CHECK(i64 <= -999'999'999'000);
        }

        CHECK(rng.getI(5, 5) == 5);
        CHECK(rng.getI(za::I8{-128}, za::I8{-128}) == -128);
    }

    SECTION("getI handles full ranges (no division by zero, no signed overflow)")
    {
        za::FastNonCryptoRng rng{2u};

        bool sawAll[256]{};
        for (int i = 0; i < 20'000; ++i)
            sawAll[rng.getI(za::U8{0}, za::U8{255})] = true;

        bool all = true;
        for (const bool saw : sawAll)
            all &= saw;
        CHECK(all);

        bool sawNegativeI8 = false, sawPositiveI8 = false;
        bool sawLowU64 = false, sawHighU64 = false;
        bool sawNegativeI64 = false, sawPositiveI64 = false;

        for (int i = 0; i < 1000; ++i)
        {
            const za::I8 i8 = rng.getI(za::I8{-128}, za::I8{127});
            sawNegativeI8 |= i8 < 0;
            sawPositiveI8 |= i8 > 0;

            const za::U64 u64 = rng.getI(za::U64{0}, std::numeric_limits<za::U64>::max());
            sawLowU64 |= u64 < (za::U64{1} << 62);
            sawHighU64 |= u64 > std::numeric_limits<za::U64>::max() - (za::U64{1} << 62);

            const za::I64 i64 = rng.getI(std::numeric_limits<za::I64>::min(), std::numeric_limits<za::I64>::max());
            sawNegativeI64 |= i64 < 0;
            sawPositiveI64 |= i64 > 0;

            // Span wider than `I64` max
            CHECK(rng.getI(za::I64{-10}, std::numeric_limits<za::I64>::max()) >= -10);
        }

        CHECK(sawNegativeI8);
        CHECK(sawPositiveI8);
        CHECK(sawLowU64);
        CHECK(sawHighU64);
        CHECK(sawNegativeI64);
        CHECK(sawPositiveI64);
    }

    SECTION("getI is unbiased")
    {
        za::FastNonCryptoRng rng{3u};

        // With `3 * 2^62` values, a modulo reduction would make the lowest quarter twice as likely (1/2, not 1/3)
        constexpr za::U64 rangeSize = za::U64{3} << 62;
        constexpr int     draws     = 100'000;

        int lowQuarter = 0;
        for (int i = 0; i < draws; ++i)
            lowQuarter += rng.getI(za::U64{0}, rangeSize - 1u) < (za::U64{1} << 62) ? 1 : 0;

        CHECK(std::abs(static_cast<double>(lowQuarter) / draws - 1.0 / 3.0) < 0.01);

        // Small range: every value equally likely (chi-square, 6 degrees of freedom; 22.5 is p ~ 0.001)
        int counts[7]{};
        for (int i = 0; i < 70'000; ++i)
            ++counts[rng.getI(0, 6)];

        double chiSquare = 0.0;
        for (const int count : counts)
            chiSquare += (count - 10'000.0) * (count - 10'000.0) / 10'000.0;

        CHECK(chiSquare < 22.5);
    }

    SECTION("getF stays within ordinary ranges, and is exact for degenerate ones")
    {
        za::FastNonCryptoRng rng{4u};

        double sum = 0.0;
        for (int i = 0; i < 100'000; ++i)
        {
            const float f = rng.getF(-1.f, 1.f);
            CHECK(f >= -1.f);
            CHECK(f <= 1.f);
            sum += static_cast<double>(f);

            const float g = rng.getF(0.f, 100.f);
            CHECK(g >= 0.f);
            CHECK(g <= 100.f);
        }

        CHECK(std::abs(sum / 100'000.0) < 0.01); // mean ~ midpoint

        for (const float x : {0.f, 1.f, 0.1f, -3.7f, 1e30f})
            CHECK(rng.getF(x, x) == x);

        const float lo = 0.1f;
        const float hi = std::nextafter(lo, 1.f);
        for (int i = 0; i < 1000; ++i)
        {
            const float f = rng.getF(lo, hi);
            CHECK((f == lo || f == hi));
        }
    }

    SECTION("getSignF")
    {
        za::FastNonCryptoRng rng{5u};

        int positives = 0;
        for (int i = 0; i < 1000; ++i)
        {
            const float sign = rng.getSignF();
            CHECK((sign == 1.f || sign == -1.f));
            positives += sign > 0.f ? 1 : 0;
        }

        CHECK(positives > 400);
        CHECK(positives < 600);
    }

    SECTION("2D vectors")
    {
        za::FastNonCryptoRng rng{6u};

        for (int i = 0; i < 1000; ++i)
        {
            const za::Vec2f v = rng.getVec2f({-1.f, 10.f}, {1.f, 20.f});
            CHECK(v.x >= -1.f);
            CHECK(v.x <= 1.f);
            CHECK(v.y >= 10.f);
            CHECK(v.y <= 20.f);

            const za::Vec2f w = rng.getVec2f({5.f, 6.f});
            CHECK(w.x >= 0.f);
            CHECK(w.x <= 5.f);
            CHECK(w.y >= 0.f);
            CHECK(w.y <= 6.f);

            const za::Vec2f p  = rng.getPointInCircle({100.f, 200.f}, 10.f);
            const float     dx = p.x - 100.f;
            const float     dy = p.y - 200.f;
            CHECK(dx * dx + dy * dy <= 100.f * (1.f + 1e-5f));

            const za::Vec2f d = rng.getDirVec2f();
            CHECK(std::abs(d.x * d.x + d.y * d.y - 1.f) < 1e-5f);
        }
    }
}
