#include "Tst/Tst.hpp"

#include "Zancle/Base/IsFinite.hpp"
#include "Zancle/Base/IsInf.hpp"
#include "Zancle/Base/IsNan.hpp"

#include <limits>


ZA_TST_DECLARE_TYPE_NAME(long double, "long double")


namespace
{
////////////////////////////////////////////////////////////
// `volatile` so that the checks are not constant-folded, exercising the run-time path
template <typename T>
[[nodiscard]] T opaque(const T value)
{
    volatile T v = value;
    return v;
}

} // namespace


// Under `-ffinite-math-only` (e.g. `-ffast-math`), the compiler assumes no infinities or NaNs exist
#if !__FINITE_MATH_ONLY__

TEMPLATE_TEST_CASE("[Base] ZA_ISFINITE, ZA_ISINF, ZA_ISNAN", "", float, double, long double)
{
    using T = TestType;

    const T inf = std::numeric_limits<T>::infinity();
    const T nan = std::numeric_limits<T>::quiet_NaN();

    SECTION("Finite values")
    {
        const T finiteValues[]{T{0},
                               -T{0},
                               T{1},
                               T{-2.5},
                               std::numeric_limits<T>::max(),
                               std::numeric_limits<T>::lowest(),
                               std::numeric_limits<T>::denorm_min()};

        for (const T x : finiteValues)
        {
            CHECK(ZA_ISFINITE(opaque(x)));
            CHECK(!ZA_ISINF(opaque(x)));
            CHECK(!ZA_ISNAN(opaque(x)));
        }
    }

    SECTION("Infinities")
    {
        const T infinities[]{inf, -inf};

        for (const T x : infinities)
        {
            CHECK(!ZA_ISFINITE(opaque(x)));
            CHECK(ZA_ISINF(opaque(x)));
            CHECK(!ZA_ISNAN(opaque(x)));
        }
    }

    SECTION("NaN")
    {
        CHECK(!ZA_ISFINITE(opaque(nan)));
        CHECK(!ZA_ISINF(opaque(nan)));
        CHECK(ZA_ISNAN(opaque(nan)));
    }
}

#endif
