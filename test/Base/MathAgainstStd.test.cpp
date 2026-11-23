#include "Tst/Tst.hpp"

#include "Zancle/Algorithm/NthElement.hpp"
#include "Zancle/Algorithm/StableSort.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Random/FastNonCryptoRng.hpp"

#include "Zancle/Vocabulary/Pair.hpp"

#include "Zancle/Math/Abs.hpp"
#include "Zancle/Math/Atan2.hpp"
#include "Zancle/Math/Cbrt.hpp"
#include "Zancle/Math/Ceil.hpp"
#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/Copysign.hpp"
#include "Zancle/Math/Exp2.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/Floor.hpp"
#include "Zancle/Math/Fmax.hpp"
#include "Zancle/Math/Fmin.hpp"
#include "Zancle/Math/Fmod.hpp"
#include "Zancle/Math/Hypot.hpp"
#include "Zancle/Math/Llround.hpp"
#include "Zancle/Math/Log2.hpp"
#include "Zancle/Math/Lround.hpp"
#include "Zancle/Math/MinMax.hpp"
#include "Zancle/Math/Remainder.hpp"
#include "Zancle/Math/Round.hpp"
#include "Zancle/Math/Trunc.hpp"

#include "Zancle/Base/BitCast.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/IsFinite.hpp"
#include "Zancle/Base/IsInf.hpp"
#include "Zancle/Base/IsNan.hpp"
#include "Zancle/Base/Limits.hpp"

#include <algorithm>
#include <utility>

#include <cmath>
#include <cstdlib>


// Zancle's math and algorithms against the standard library's, on edge values (signed zeros, halfway cases, the angle
// wrap's angles, huge, tiny, subnormal, infinite and NaN inputs): every result the same to the last bit, so that code
// moving from the standard library to Zancle keeps its exact behavior. (Ported from Quake VR's `vr_zancle_math_test`.)


namespace
{
////////////////////////////////////////////////////////////
// The same value to the last bit (any two NaNs alike; -0 and +0 told apart)
[[nodiscard]] bool same(const float a, const float b) noexcept
{
    return (ZA_ISNAN(a) && ZA_ISNAN(b)) || ZA_BIT_CAST(za::U32, a) == ZA_BIT_CAST(za::U32, b);
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool same(const double a, const double b) noexcept
{
    return (ZA_ISNAN(a) && ZA_ISNAN(b)) || ZA_BIT_CAST(za::U64, a) == ZA_BIT_CAST(za::U64, b);
}


////////////////////////////////////////////////////////////
// Not `constexpr`: the run-time code paths, not constant folding
const float edgeValues[] =
    {0.f,
     -0.f,
     0.5f,
     -0.5f,
     1.5f,
     -1.5f,
     2.5f,
     -2.5f,
     0.49999997f,
     1.f,
     -1.f,
     3.14159265f,
     179.f,
     -179.f,
     179.99f,
     180.f,
     -180.f,
     180.01f,
     181.f,
     -181.f,
     270.f,
     -270.f,
     359.f,
     -359.f,
     360.f,
     -360.f,
     361.f,
     540.f,
     -540.f,
     719.f,
     720.f,
     -720.f,
     1e-40f,
     -1e-40f,
     1e-7f,
     123456.78f,
     -98765.43f,
     8388607.5f,
     1e9f,
     -1e9f,
     3e38f,
     -3e38f,
     ZA_FLOAT_INFINITY,
     -ZA_FLOAT_INFINITY,
     ZA_FLOAT_NAN};

const float divisors[] = {360.f, 6.28318531f, 1.f, 0.5f, 7.f, -360.f};


////////////////////////////////////////////////////////////
// Reports the first few failures with their inputs, and counts them all
struct Checker
{
    int checks   = 0;
    int failures = 0;

    void operator()(const bool ok, const char* const what, const double a, const double b)
    {
        ++checks;

        if (!ok && ++failures <= 12)
            FAIL_CHECK(what << " (" << a << ", " << b << ")");
    }
};

} // namespace


TEST_CASE("[Base] Math against the standard library, bit for bit")
{
    Checker check;

    SECTION("One argument")
    {
        for (const float a : edgeValues)
        {
            const double d = a;

            check(same(za::floor(a), std::floor(a)) && same(za::floor(d), std::floor(d)), "floor", a, 0);
            check(same(za::ceil(a), std::ceil(a)) && same(za::ceil(d), std::ceil(d)), "ceil", a, 0);
            check(same(za::round(a), std::round(a)) && same(za::round(d), std::round(d)), "round", a, 0);
            check(same(za::trunc(a), std::trunc(a)) && same(za::trunc(d), std::trunc(d)), "trunc", a, 0);
            check(same(za::cbrt(a), std::cbrt(a)) && same(za::cbrt(d), std::cbrt(d)), "cbrt", a, 0);
            check(same(za::log2(a), std::log2(a)) && same(za::log2(d), std::log2(d)), "log2", a, 0);
            check(same(za::exp2(a), std::exp2(a)) && same(za::exp2(d), std::exp2(d)), "exp2", a, 0);
            check(same(za::abs(a), std::abs(a)) && same(za::abs(d), std::abs(d)), "abs", a, 0);
            check(ZA_ISFINITE(a) == std::isfinite(a) && ZA_ISNAN(a) == std::isnan(a) && ZA_ISINF(a) == std::isinf(a),
                  "isfinite/isnan/isinf",
                  a,
                  0);

            if (ZA_ISFINITE(a) && za::fabs(a) < 1e9f) // `lround` and `llround` out of their range: unspecified
            {
                check(za::lround(a) == std::lround(a) && za::lround(d) == std::lround(d), "lround", a, 0);
                check(za::llround(a) == std::llround(a) && za::llround(d) == std::llround(d), "llround", a, 0);

                const int i = static_cast<int>(a);
                check(za::abs(i) == std::abs(i), "abs (int)", a, 0);
            }
        }

        CHECK(check.checks > 0);
        CHECK(check.failures == 0);
    }

    SECTION("Two arguments, min/max of two to four, clamp")
    {
        for (const float a : edgeValues)
            for (const float b : edgeValues)
            {
                check(same(za::atan2(a, b), std::atan2(a, b)), "atan2", a, b);
                check(same(za::copysign(a, b), std::copysign(a, b)), "copysign", a, b);
                check(same(za::hypot(a, b), std::hypot(a, b)), "hypot", a, b);
                check(same(za::fmin(a, b), std::fmin(a, b)) && same(za::fmax(a, b), std::fmax(a, b)), "fmin/fmax", a, b);
                check(same(za::min(a, b), std::min(a, b)) && same(za::max(a, b), std::max(a, b)), "min/max", a, b);

                // Of three or four: `std::min` and `std::max` folded from the left; against `std`'s list overloads too
                // where there is no NaN (no order then: MSVC's vectorized ones can give other results)
                const float m3 = std::min(std::min(a, b), -b);
                const float M3 = std::max(std::max(a, b), -b);
                const float m4 = std::min(std::min(std::min(b, a), a), b);
                const float M4 = std::max(std::max(std::max(b, a), a), b);

                check(same(za::min(a, b, -b), m3) && same(za::max(a, b, -b), M3) && same(za::min(b, a, a, b), m4) &&
                          same(za::max(b, a, a, b), M4) &&
                          (ZA_ISNAN(a) || ZA_ISNAN(b) ||
                           (same(m3, std::min({a, b, -b})) && same(M3, std::max({a, b, -b})) &&
                            same(m4, std::min({b, a, a, b})) && same(M4, std::max({b, a, a, b})))),
                      "min/max (3, 4)",
                      a,
                      b);

                if (ZA_ISNAN(a) || ZA_ISNAN(b))
                    continue;

                const float lo = za::min(a, b);
                const float hi = za::max(a, b);

                for (const float v : edgeValues)
                    check(same(za::clamp(v, lo, hi), std::clamp(v, lo, hi)), "clamp", v, a);
            }

        CHECK(check.checks > 0);
        CHECK(check.failures == 0);
    }

    SECTION("fmod and remainder")
    {
        for (const float a : edgeValues)
            for (const float b : divisors)
            {
                check(same(za::fmod(a, b), std::fmod(a, b)), "fmod", a, b);
                check(same(za::remainder(a, b), std::remainder(a, b)) &&
                          same(za::remainder(double{a}, double{b}), std::remainder(double{a}, double{b})),
                      "remainder",
                      a,
                      b);
            }

        // The angle wrap: every whole and half degree from -1080 to 1080 lands in [-180, 180], as with `std::remainder`
        for (int k = -2160; k <= 2160; ++k)
        {
            const float a = static_cast<float>(k) * 0.5f;
            const float r = za::remainder(a, 360.f);

            check(r >= -180.f && r <= 180.f && same(r, std::remainder(a, 360.f)), "the angle wrap", a, 360.0);
        }

        CHECK(check.checks > 0);
        CHECK(check.failures == 0);
    }
}


TEST_CASE("[Base] Algorithms against the standard library")
{
    // Fixed seed: the same arrays every run
    za::FastNonCryptoRng rng{20'261'001u};
    Checker              check;

    for (int round = 0; round < 200; ++round)
    {
        // `stableSort` is `std::stable_sort` to the element: keys with many ties, each tagged with its place
        const int n     = rng.getI(1, 300);
        const int range = rng.getI(1, 40);

        za::Vector<za::Pair<int, int>>  mine;
        za::Vector<std::pair<int, int>> ref;

        for (int i = 0; i < n; ++i)
        {
            const int key = rng.getI(0, range);
            mine.pushBack(za::Pair<int, int>{key, i});
            ref.pushBack(std::pair<int, int>{key, i});
        }

        za::stableSort(mine.begin(), mine.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        std::stable_sort(ref.begin(), ref.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

        bool sameOrder = true;
        for (int i = 0; i < n; ++i)
            sameOrder = sameOrder && mine[static_cast<za::SizeT>(i)].first == ref[static_cast<za::SizeT>(i)].first &&
                        mine[static_cast<za::SizeT>(i)].second == ref[static_cast<za::SizeT>(i)].second;

        check(sameOrder, "stableSort", n, range);

        // `nthElement` puts `std::nth_element`'s value at the nth place, nothing after it less and nothing before it
        // greater (which of equal keys goes where is its own)
        za::Vector<int> keys;
        za::Vector<int> refKeys;

        for (int i = 0; i < n; ++i)
        {
            keys.pushBack(rng.getI(0, range));
            refKeys.pushBack(keys.back());
        }

        const int nth = rng.getI(0, n - 1);
        za::nthElement(keys.begin(), keys.begin() + nth, keys.end(), [](const int a, const int b) { return a < b; });
        std::nth_element(refKeys.begin(), refKeys.begin() + nth, refKeys.end());

        const int nthKey      = keys[static_cast<za::SizeT>(nth)];
        bool      partitioned = nthKey == refKeys[static_cast<za::SizeT>(nth)];

        for (int i = 0; i < n; ++i)
            partitioned = partitioned && (i < nth ? keys[static_cast<za::SizeT>(i)] <= nthKey
                                                  : keys[static_cast<za::SizeT>(i)] >= nthKey);

        check(partitioned, "nthElement", n, nth);
    }

    CHECK(check.checks == 400);
    CHECK(check.failures == 0);
}
