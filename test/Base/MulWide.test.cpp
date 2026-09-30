#include "Tst/Tst.hpp"

#include "Zancle/Base/MulWide.hpp"

#include "Zancle/Base/IntTypes.hpp"


namespace
{
namespace MulWideTest // for unity builds
{
////////////////////////////////////////////////////////////
struct Product
{
    za::U64 high;
    za::U64 low;
};


////////////////////////////////////////////////////////////
[[nodiscard]] constexpr Product portable(const za::U64 a, const za::U64 b)
{
    za::U64       high = 0u;
    const za::U64 low  = za::priv::mulWidePortable(a, b, high);
    return {high, low};
}


////////////////////////////////////////////////////////////
[[nodiscard]] constexpr Product native(const za::U64 a, const za::U64 b)
{
    za::U64       high = 0u;
    const za::U64 low  = za::priv::mulWide(a, b, high);
    return {high, low};
}


////////////////////////////////////////////////////////////
[[nodiscard]] constexpr bool matches(const za::U64 a, const za::U64 b, const za::U64 high, const za::U64 low)
{
    const Product p = portable(a, b);
    const Product n = native(a, b);
    return p.high == high && p.low == low && n.high == high && n.low == low;
}

} // namespace MulWideTest
} // namespace


TEST_CASE("[Base] MulWide.hpp")
{
    using MulWideTest::matches;

    constexpr za::U64 max = ~za::U64{0};

    SECTION("Known products")
    {
        STATIC_CHECK(matches(0u, 0u, 0u, 0u));
        STATIC_CHECK(matches(0u, max, 0u, 0u));
        STATIC_CHECK(matches(1u, max, 0u, max));
        STATIC_CHECK(matches(max, max, max - 1u, 1u));                     // (2^64 - 1)^2 == 2^128 - 2^65 + 1
        STATIC_CHECK(matches(za::U64{1} << 32, za::U64{1} << 32, 1u, 0u)); // 2^64
        STATIC_CHECK(matches(0xFF'FF'FF'FFu, 0xFF'FF'FF'FFu, 0u, 0xFF'FF'FF'FE'00'00'00'01u));
        STATIC_CHECK(matches(10'000'000'000'000'000'000u,
                             10'000'000'000'000'000'000u,
                             5'421'010'862'427'522'170u,
                             687'399'551'400'673'280u)); // 10^38
    }

    SECTION("The portable fallback agrees with the native product")
    {
        za::U64 state = 987'654'321u;

        for (int i = 0; i < 100'000; ++i)
        {
            state           = state * 6'364'136'223'846'793'005u + 1'442'695'040'888'963'407u; // LCG
            const za::U64 a = state ^ (state >> 31);
            state           = state * 6'364'136'223'846'793'005u + 1'442'695'040'888'963'407u;
            const za::U64 b = (state ^ (state >> 27)) >> (i % 64); // vary the magnitude

            const auto p = MulWideTest::portable(a, b);
            const auto n = MulWideTest::native(a, b);
            CHECK(p.high == n.high);
            CHECK(p.low == n.low);
            CHECK(p.low == a * b);
        }
    }
}
