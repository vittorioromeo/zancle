#include "Tst/Tst.hpp"

#include "Zancle/Container/AnkerlUnorderedDense.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Limits.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/Memset.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace
{
namespace AnkerlUnorderedDenseFloatHashTest // unity build
{

using HashF = ankerl::unordered_dense::v4_8_1::hash<float>;
using HashD = ankerl::unordered_dense::v4_8_1::hash<double>;
using HashL = ankerl::unordered_dense::v4_8_1::hash<long double>;


////////////////////////////////////////////////////////////
template <typename Hasher, typename Float>
constexpr za::U64 hashOf(Float v)
{
    return Hasher{}(v);
}

} // namespace AnkerlUnorderedDenseFloatHashTest
} // namespace


TEST_CASE("[Base] za::ankerl floating-point hashers")
{
    using AnkerlUnorderedDenseFloatHashTest::HashD;
    using AnkerlUnorderedDenseFloatHashTest::HashF;
    using AnkerlUnorderedDenseFloatHashTest::HashL;
    using AnkerlUnorderedDenseFloatHashTest::hashOf;

    SECTION("is_avalanching tag is present on all three (compiles)")
    {
        using F = typename HashF::is_avalanching;
        using D = typename HashD::is_avalanching;
        using L = typename HashL::is_avalanching;
        (void)sizeof(F*);
        (void)sizeof(D*);
        (void)sizeof(L*);
    }

    SECTION("noexcept")
    {
        STATIC_CHECK(noexcept(HashF{}(1.0F)));
        STATIC_CHECK(noexcept(HashD{}(1.0)));
        STATIC_CHECK(noexcept(HashL{}(1.0L)));
    }

    SECTION("+0.0 and -0.0 hash to the same value (float)")
    {
        CHECK(hashOf<HashF>(+0.0F) == hashOf<HashF>(-0.0F));
    }

    SECTION("+0.0 and -0.0 hash to the same value (double)")
    {
        CHECK(hashOf<HashD>(+0.0) == hashOf<HashD>(-0.0));
    }

    SECTION("+0.0 and -0.0 hash to the same value (long double)")
    {
        CHECK(hashOf<HashL>(+0.0L) == hashOf<HashL>(-0.0L));
    }

    SECTION("Distinct values hash distinctly (float)")
    {
        const float values[]{1.0F, 2.0F, -1.0F, 3.14159F, 1e-10F, 1e10F};
        for (za::SizeT i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
            for (za::SizeT j = i + 1; j < sizeof(values) / sizeof(values[0]); ++j)
                CHECK(hashOf<HashF>(values[i]) != hashOf<HashF>(values[j]));
    }

    SECTION("Distinct values hash distinctly (double)")
    {
        const double values[]{1.0, 2.0, -1.0, 3.14159265358979, 1e-100, 1e100};
        for (za::SizeT i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
            for (za::SizeT j = i + 1; j < sizeof(values) / sizeof(values[0]); ++j)
                CHECK(hashOf<HashD>(values[i]) != hashOf<HashD>(values[j]));
    }

    SECTION("Distinct values hash distinctly (long double)")
    {
        const long double values[]{1.0L, 2.0L, -1.0L, 3.14159265358979L, 1e-100L, 1e100L};
        for (za::SizeT i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
            for (za::SizeT j = i + 1; j < sizeof(values) / sizeof(values[0]); ++j)
                CHECK(hashOf<HashL>(values[i]) != hashOf<HashL>(values[j]));
    }

    SECTION("Equal values hash equally (float)")
    {
        const float a = 1.5F;
        const float b = 3.0F / 2.0F;
        CHECK(hashOf<HashF>(a) == hashOf<HashF>(b));
    }

    SECTION("Equal values hash equally (double)")
    {
        const double a = 1.5;
        const double b = 3.0 / 2.0;
        CHECK(hashOf<HashD>(a) == hashOf<HashD>(b));
    }

    SECTION("Infinity hashes consistently and differs from regular values")
    {
        const float  posInfF = 1.0F / 0.0F;
        const double posInfD = 1.0 / 0.0;
        CHECK(hashOf<HashF>(posInfF) == hashOf<HashF>(1.0F / 0.0F));
        CHECK(hashOf<HashD>(posInfD) == hashOf<HashD>(1.0 / 0.0));
        CHECK(hashOf<HashF>(posInfF) != hashOf<HashF>(1.0F));
        CHECK(hashOf<HashD>(posInfD) != hashOf<HashD>(1.0));
    }

    SECTION("Positive and negative infinity hash differently")
    {
        const float negInfF = -1.0F / 0.0F;
        const float posInfF = 1.0F / 0.0F;
        CHECK(hashOf<HashF>(posInfF) != hashOf<HashF>(negInfF));

        const double negInfD = -1.0 / 0.0;
        const double posInfD = 1.0 / 0.0;
        CHECK(hashOf<HashD>(posInfD) != hashOf<HashD>(negInfD));
    }

    SECTION("Smallest subnormal hashes distinctly from zero")
    {
        const float  subF = 1e-40F; // subnormal float
        const double subD = 1e-310; // subnormal double
        CHECK(hashOf<HashF>(subF) != hashOf<HashF>(0.0F));
        CHECK(hashOf<HashD>(subD) != hashOf<HashD>(0.0));
    }

#if ZA_LONG_DOUBLE_MANT_DIG == 64
    SECTION("x87 long double padding bytes do not affect the hash")
    {
        // Only the first 10 bytes of an x87 `long double` hold its value, the
        // remaining `sizeof(long double) - 10` bytes are padding.
        STATIC_CHECK(sizeof(long double) > 10u);

        const long double value = 3.14159265358979L;

        unsigned char bufA[sizeof(long double)];
        unsigned char bufB[sizeof(long double)];
        ZA_MEMSET(bufA, 0x00, sizeof(bufA));
        ZA_MEMSET(bufB, 0xAB, sizeof(bufB));
        ZA_MEMCPY(bufA, &value, 10u);
        ZA_MEMCPY(bufB, &value, 10u);

        long double a; // NOLINT(cppcoreguidelines-init-variables)
        long double b; // NOLINT(cppcoreguidelines-init-variables)
        ZA_MEMCPY(&a, bufA, sizeof(a));
        ZA_MEMCPY(&b, bufB, sizeof(b));

        REQUIRE(a == b);
        CHECK(hashOf<HashL>(a) == hashOf<HashL>(b));

        ankerl::unordered_dense::map<long double, int> m;
        m[a] = 42;

        const auto* const it = m.find(b);
        REQUIRE(it != m.end());
        CHECK(it->second == 42);
        CHECK(m.size() == 1u);
    }
#endif
}
