#include "Tst/Tst.hpp"

#include "Zancle/Math/Abs.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Signbit.hpp"

#include "Zancle/Trait/IsSame.hpp"

#include <cstdlib>


namespace
{
namespace AbsTest // for unity builds
{
////////////////////////////////////////////////////////////
template <typename T>
concept CanAbs = requires(T x) { za::abs(x); };

} // namespace AbsTest
} // namespace


TEST_CASE("[Base] Math/Abs.hpp")
{
    SECTION("Integers, with std::abs's result types")
    {
        STATIC_CHECK(za::abs(-5) == 5);
        STATIC_CHECK(za::abs(5) == 5);
        STATIC_CHECK(za::abs(0) == 0);
        STATIC_CHECK(za::abs(-5LL) == 5LL);

        STATIC_CHECK(ZA_IS_SAME(decltype(za::abs(za::I8{-3})), int)); // promoted, like `std::abs`
        STATIC_CHECK(ZA_IS_SAME(decltype(za::abs(short{-3})), int));
        STATIC_CHECK(ZA_IS_SAME(decltype(za::abs(-3L)), long));
        STATIC_CHECK(ZA_IS_SAME(decltype(za::abs(-3LL)), long long));
        STATIC_CHECK(za::abs(za::I8{-128}) == 128); // fine after the promotion

        int mismatches = 0;
        for (int i = -1000; i <= 1000; ++i)
            mismatches += za::abs(i) != std::abs(i);

        CHECK(mismatches == 0);
    }

    SECTION("Floating-point numbers")
    {
        STATIC_CHECK(ZA_IS_SAME(decltype(za::abs(-2.5f)), float));
        STATIC_CHECK(ZA_IS_SAME(decltype(za::abs(-2.5)), double));
        STATIC_CHECK(ZA_IS_SAME(decltype(za::abs(-2.5L)), long double));

        volatile float opaque = -2.5f; // run time
        CHECK(za::abs(opaque) == 2.5f);
        CHECK(za::abs(-2.5) == 2.5);
        CHECK(za::abs(-2.5L) == 2.5L);
        CHECK(!ZA_SIGNBIT(za::abs(-0.f)));
    }

    SECTION("Unsigned integers and bool are rejected, as by std::abs")
    {
        STATIC_CHECK(AbsTest::CanAbs<int>);
        STATIC_CHECK(AbsTest::CanAbs<float>);
        STATIC_CHECK(!AbsTest::CanAbs<unsigned int>);
        STATIC_CHECK(!AbsTest::CanAbs<za::U8>);
        STATIC_CHECK(!AbsTest::CanAbs<bool>);
    }
}
