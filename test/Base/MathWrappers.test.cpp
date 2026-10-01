#include "Tst/Tst.hpp"

#include "Zancle/Math/Acos.hpp"
#include "Zancle/Math/Asin.hpp"
#include "Zancle/Math/Atan.hpp"
#include "Zancle/Math/Atan2.hpp"
#include "Zancle/Math/Cbrt.hpp"
#include "Zancle/Math/Ceil.hpp"
#include "Zancle/Math/Copysign.hpp"
#include "Zancle/Math/Cos.hpp"
#include "Zancle/Math/Cosh.hpp"
#include "Zancle/Math/Exp.hpp"
#include "Zancle/Math/Exp2.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/Floor.hpp"
#include "Zancle/Math/Fmax.hpp"
#include "Zancle/Math/Fmin.hpp"
#include "Zancle/Math/Fmod.hpp"
#include "Zancle/Math/Hypot.hpp"
#include "Zancle/Math/Llround.hpp"
#include "Zancle/Math/Log.hpp"
#include "Zancle/Math/Log10.hpp"
#include "Zancle/Math/Log2.hpp"
#include "Zancle/Math/Lround.hpp"
#include "Zancle/Math/Nextafter.hpp"
#include "Zancle/Math/Pow.hpp"
#include "Zancle/Math/Rint.hpp"
#include "Zancle/Math/Round.hpp"
#include "Zancle/Math/Sin.hpp"
#include "Zancle/Math/Sqrt.hpp"
#include "Zancle/Math/Tan.hpp"
#include "Zancle/Math/Trunc.hpp"

#include "Zancle/Base/BitCast.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Limits.hpp"

#include "Zancle/Trait/IsSame.hpp"

#include <limits>


ZA_TST_DECLARE_TYPE_NAME(long double, "long double")


namespace
{
////////////////////////////////////////////////////////////
// `volatile` so that the calls are not constant-folded, exercising the run-time path
template <typename T>
[[nodiscard]] T opaque(const T value)
{
    volatile T v = value;
    return v;
}


////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] tst::Approx approx(const double value)
{
    // `float` results are only accurate to about 7 significant digits
    return tst::Approx(value).epsilon(ZA_IS_SAME(T, float) ? 1e-6 : 1e-12);
}

} // namespace


TEMPLATE_TEST_CASE("[Base] Math wrappers", "", float, double, long double)
{
    using T = TestType;

    SECTION("Return types match the argument type")
    {
        STATIC_CHECK(za::isSame<decltype(za::sqrt(T{})), T>);
        STATIC_CHECK(za::isSame<decltype(za::acos(T{})), T>);
        STATIC_CHECK(za::isSame<decltype(za::pow(T{}, T{})), T>);
        STATIC_CHECK(za::isSame<decltype(za::atan2(T{}, T{})), T>);
        STATIC_CHECK(za::isSame<decltype(za::nextafter(T{}, T{})), T>);
        STATIC_CHECK(za::isSame<decltype(za::lround(T{})), long>);
        STATIC_CHECK(za::isSame<decltype(za::llround(T{})), long long>);
        STATIC_CHECK(za::isSame<decltype(za::hypot(T{}, T{})), T>);
        STATIC_CHECK(za::isSame<decltype(za::copysign(T{}, T{})), T>);
        STATIC_CHECK(za::isSame<decltype(za::trunc(T{})), T>);
    }

    SECTION("Trigonometric")
    {
        CHECK(static_cast<double>(za::sin(opaque(T{0.5}))) == approx<T>(0.479425538604203));
        CHECK(static_cast<double>(za::cos(opaque(T{0.5}))) == approx<T>(0.8775825618903728));
        CHECK(static_cast<double>(za::tan(opaque(T{0.5}))) == approx<T>(0.5463024898437905));
        CHECK(static_cast<double>(za::asin(opaque(T{0.5}))) == approx<T>(0.5235987755982989));
        CHECK(static_cast<double>(za::acos(opaque(T{0.5}))) == approx<T>(1.0471975511965979));
        CHECK(static_cast<double>(za::atan(opaque(T{1}))) == approx<T>(0.7853981633974483));
        CHECK(static_cast<double>(za::atan2(opaque(T{1}), opaque(T{-1}))) == approx<T>(2.356194490192345));
        CHECK(static_cast<double>(za::cosh(opaque(T{1}))) == approx<T>(1.5430806348152437));
    }

    SECTION("Exponential and logarithmic")
    {
        CHECK(static_cast<double>(za::exp(opaque(T{1}))) == approx<T>(2.718281828459045));
        CHECK(static_cast<double>(za::log(opaque(T{2}))) == approx<T>(0.6931471805599453));
        CHECK(static_cast<double>(za::log10(opaque(T{1000}))) == approx<T>(3.0));
        CHECK(static_cast<double>(za::pow(opaque(T{2}), opaque(T{10}))) == approx<T>(1024.0));
        CHECK(static_cast<double>(za::sqrt(opaque(T{2}))) == approx<T>(1.4142135623730951));
        CHECK(static_cast<double>(za::cbrt(opaque(T{27}))) == approx<T>(3.0));
        CHECK(static_cast<double>(za::cbrt(opaque(T{-8}))) == approx<T>(-2.0)); // defined for negative values
        CHECK(static_cast<double>(za::log2(opaque(T{8}))) == approx<T>(3.0));
        CHECK(static_cast<double>(za::log2(opaque(T{3}))) == approx<T>(1.584962500721156));
        CHECK(static_cast<double>(za::exp2(opaque(T{10}))) == approx<T>(1024.0));
        CHECK(static_cast<double>(za::exp2(opaque(T{-0.5}))) == approx<T>(0.7071067811865476));
        CHECK(static_cast<double>(za::hypot(opaque(T{3}), opaque(T{4}))) == approx<T>(5.0));
        CHECK(static_cast<double>(za::hypot(opaque(T{-5}), opaque(T{12}))) == approx<T>(13.0));
    }

    SECTION("Rounding and remainder")
    {
        CHECK(za::floor(opaque(T{-1.5})) == T{-2});
        CHECK(za::ceil(opaque(T{-1.5})) == T{-1});
        CHECK(za::round(opaque(T{2.5})) == T{3}); // halfway cases away from zero
        CHECK(za::round(opaque(T{-2.5})) == T{-3});
        CHECK(za::rint(opaque(T{2.5})) == T{2}); // default rounding mode: halfway cases to even
        CHECK(za::lround(opaque(T{-2.5})) == -3L);
        CHECK(za::llround(opaque(T{-2.5})) == -3LL);
        CHECK(za::llround(opaque(T{8'589'934'592})) == 8'589'934'592LL); // 2^33, beyond a 32-bit `long`
        CHECK(za::trunc(opaque(T{-1.7})) == T{-1});                      // toward zero
        CHECK(za::trunc(opaque(T{2.9})) == T{2});
        CHECK(za::fmod(opaque(T{-7}), opaque(T{3})) == T{-1});
    }

    SECTION("Next representable value")
    {
        const T up   = za::nextafter(opaque(T{1}), opaque(T{2}));
        const T down = za::nextafter(opaque(T{1}), opaque(T{0}));

        CHECK(up > T{1});
        CHECK(down < T{1});
        CHECK(up - T{1} == std::numeric_limits<T>::epsilon());
        CHECK(za::nextafter(opaque(T{1}), opaque(T{1})) == T{1});
        CHECK(za::nextafter(up, T{0}) == T{1});
    }

    SECTION("Sign")
    {
        CHECK(za::copysign(opaque(T{3}), opaque(T{-1})) == T{-3});
        CHECK(za::copysign(opaque(T{-3}), opaque(T{1})) == T{3});

        // The sign of zero counts, unlike with comparisons
        const T negativeZero = za::copysign(opaque(T{0}), opaque(T{-1}));
        CHECK(negativeZero == T{0});
        CHECK(za::copysign(opaque(T{2}), negativeZero) == T{-2});
    }

    SECTION("Absolute value, minimum, maximum")
    {
        CHECK(za::fabs(opaque(T{-3})) == T{3});
        CHECK(za::fmax(opaque(T{-1}), opaque(T{2})) == T{2});
        CHECK(za::fmin(opaque(T{-1}), opaque(T{2})) == T{-1});

        // Unlike `<`-based min/max, a NaN operand is ignored
        const T nan = std::numeric_limits<T>::quiet_NaN();
        CHECK(za::fmax(opaque(nan), opaque(T{2})) == T{2});
        CHECK(za::fmin(opaque(T{2}), opaque(nan)) == T{2});
    }
}


TEST_CASE("[Base] Math limit constants")
{
    STATIC_CHECK(ZA_FLOAT_MAX == std::numeric_limits<float>::max());
    STATIC_CHECK(ZA_DOUBLE_MAX == std::numeric_limits<double>::max());
    STATIC_CHECK(ZA_LONG_DOUBLE_MAX == std::numeric_limits<long double>::max());
    STATIC_CHECK(ZA_FLOAT_EPSILON == std::numeric_limits<float>::epsilon());
}


TEST_CASE("[Base] Math/Fabs.hpp: sign-bit fallback")
{
    // Used when `__builtin_fabs` is unavailable (e.g. MSVC): must be exact and usable at compile time
    STATIC_CHECK(za::priv::fabsViaSignBit(-3.5f) == 3.5f);
    STATIC_CHECK(za::priv::fabsViaSignBit(3.5f) == 3.5f);
    STATIC_CHECK(za::priv::fabsViaSignBit(-3.5) == 3.5);
    STATIC_CHECK(za::priv::fabsViaSignBit(-ZA_FLOAT_MAX) == ZA_FLOAT_MAX);
    STATIC_CHECK(za::priv::fabsViaSignBit(-ZA_DOUBLE_TRUE_MIN) == ZA_DOUBLE_TRUE_MIN);

    // `-0` becomes `+0`, which only the bits tell apart
    STATIC_CHECK(ZA_BIT_CAST(za::U32, za::priv::fabsViaSignBit(-0.f)) == 0u);
    STATIC_CHECK(ZA_BIT_CAST(za::U64, za::priv::fabsViaSignBit(-0.0)) == 0u);

    // Infinities and NaNs only lose their sign
    constexpr float inf = ZA_BIT_CAST(float, za::U32{0x7F'80'00'00u});
    STATIC_CHECK(za::priv::fabsViaSignBit(-inf) == inf);

    constexpr za::U32 negativeNaNBits = 0xFF'C0'00'01u;
    STATIC_CHECK(ZA_BIT_CAST(za::U32, za::priv::fabsViaSignBit(ZA_BIT_CAST(float, negativeNaNBits))) == 0x7F'C0'00'01u);

    // Agrees with the wrapper
    CHECK(za::priv::fabsViaSignBit(-1.25f) == za::fabs(-1.25f));
    CHECK(za::priv::fabsViaSignBit(-1.25) == za::fabs(-1.25));
}
