#include "Tst/Tst.hpp"

#include "Zancle/Base/Limits.hpp"

#include "Zancle/Trait/IsSame.hpp"

#include <limits>

#include <cfloat>
#include <climits>
#include <cmath>


namespace
{
////////////////////////////////////////////////////////////
// Same value and same type as the standard macro
#define ZA_TEST_LIMIT(zaMacro, stdMacro)                                   \
    static_assert(zaMacro == stdMacro, #zaMacro " must equal " #stdMacro); \
    static_assert(za::isSame<decltype(zaMacro), decltype(stdMacro)>, #zaMacro " type must match " #stdMacro)

ZA_TEST_LIMIT(ZA_CHAR_BIT, CHAR_BIT);

ZA_TEST_LIMIT(ZA_SIGNED_CHAR_MIN, SCHAR_MIN);
ZA_TEST_LIMIT(ZA_SIGNED_CHAR_MAX, SCHAR_MAX);
ZA_TEST_LIMIT(ZA_UNSIGNED_CHAR_MAX, UCHAR_MAX);
ZA_TEST_LIMIT(ZA_CHAR_MIN, CHAR_MIN);
ZA_TEST_LIMIT(ZA_CHAR_MAX, CHAR_MAX);

ZA_TEST_LIMIT(ZA_SHORT_MIN, SHRT_MIN);
ZA_TEST_LIMIT(ZA_SHORT_MAX, SHRT_MAX);
ZA_TEST_LIMIT(ZA_UNSIGNED_SHORT_MAX, USHRT_MAX);

ZA_TEST_LIMIT(ZA_INT_MIN, INT_MIN);
ZA_TEST_LIMIT(ZA_INT_MAX, INT_MAX);
ZA_TEST_LIMIT(ZA_UNSIGNED_INT_MAX, UINT_MAX);

ZA_TEST_LIMIT(ZA_LONG_MIN, LONG_MIN);
ZA_TEST_LIMIT(ZA_LONG_MAX, LONG_MAX);
ZA_TEST_LIMIT(ZA_UNSIGNED_LONG_MAX, ULONG_MAX);

ZA_TEST_LIMIT(ZA_LONG_LONG_MIN, LLONG_MIN);
ZA_TEST_LIMIT(ZA_LONG_LONG_MAX, LLONG_MAX);
ZA_TEST_LIMIT(ZA_UNSIGNED_LONG_LONG_MAX, ULLONG_MAX);

ZA_TEST_LIMIT(ZA_FLOAT_MAX, FLT_MAX);
ZA_TEST_LIMIT(ZA_FLOAT_MIN, FLT_MIN);
ZA_TEST_LIMIT(ZA_FLOAT_TRUE_MIN, FLT_TRUE_MIN);
ZA_TEST_LIMIT(ZA_FLOAT_EPSILON, FLT_EPSILON);
ZA_TEST_LIMIT(ZA_FLOAT_MANT_DIG, FLT_MANT_DIG);

ZA_TEST_LIMIT(ZA_DOUBLE_MAX, DBL_MAX);
ZA_TEST_LIMIT(ZA_DOUBLE_MIN, DBL_MIN);
ZA_TEST_LIMIT(ZA_DOUBLE_TRUE_MIN, DBL_TRUE_MIN);
ZA_TEST_LIMIT(ZA_DOUBLE_EPSILON, DBL_EPSILON);
ZA_TEST_LIMIT(ZA_DOUBLE_MANT_DIG, DBL_MANT_DIG);

// Values only: MSVC's `<cfloat>` defines `LDBL_MAX` & co. as `double` constants (its `long double` is `double`)
static_assert(ZA_LONG_DOUBLE_MAX == LDBL_MAX);
static_assert(ZA_LONG_DOUBLE_MIN == LDBL_MIN);
static_assert(ZA_LONG_DOUBLE_TRUE_MIN == LDBL_TRUE_MIN);
static_assert(ZA_LONG_DOUBLE_EPSILON == LDBL_EPSILON);
ZA_TEST_LIMIT(ZA_LONG_DOUBLE_MANT_DIG, LDBL_MANT_DIG);

static_assert(za::isSame<decltype(ZA_LONG_DOUBLE_MAX), long double>);
static_assert(za::isSame<decltype(ZA_LONG_DOUBLE_MIN), long double>);
static_assert(za::isSame<decltype(ZA_LONG_DOUBLE_TRUE_MIN), long double>);
static_assert(za::isSame<decltype(ZA_LONG_DOUBLE_EPSILON), long double>);

#undef ZA_TEST_LIMIT

} // namespace


TEST_CASE("[Base] Base/Limits.hpp")
{
    // All checks are compile-time (see above)
    STATIC_CHECK(ZA_UNSIGNED_INT_MAX == 4'294'967'295u);
    STATIC_CHECK(ZA_INT_MIN < 0 && ZA_INT_MAX > 0);
}


TEST_CASE("[Base] Base/Limits.hpp - infinities and NaNs")
{
    STATIC_CHECK(ZA_IS_SAME(decltype(ZA_FLOAT_INFINITY), float));
    STATIC_CHECK(ZA_IS_SAME(decltype(ZA_DOUBLE_NAN), double));
    STATIC_CHECK(ZA_IS_SAME(decltype(ZA_LONG_DOUBLE_NAN), long double));

    // Constant expressions
    STATIC_CHECK(ZA_FLOAT_INFINITY > ZA_FLOAT_MAX);
    STATIC_CHECK(ZA_DOUBLE_INFINITY > ZA_DOUBLE_MAX);
    STATIC_CHECK(ZA_LONG_DOUBLE_INFINITY > ZA_LONG_DOUBLE_MAX);
    STATIC_CHECK(ZA_FLOAT_NAN != ZA_FLOAT_NAN);
    STATIC_CHECK(ZA_DOUBLE_NAN != ZA_DOUBLE_NAN);
    STATIC_CHECK(ZA_LONG_DOUBLE_NAN != ZA_LONG_DOUBLE_NAN);

    CHECK(ZA_FLOAT_INFINITY == std::numeric_limits<float>::infinity());
    CHECK(ZA_DOUBLE_INFINITY == std::numeric_limits<double>::infinity());
    CHECK(ZA_LONG_DOUBLE_INFINITY == std::numeric_limits<long double>::infinity());

    CHECK(std::isnan(ZA_FLOAT_NAN));
    CHECK(std::isnan(ZA_DOUBLE_NAN));
    CHECK(std::isnan(ZA_LONG_DOUBLE_NAN));
    CHECK(!std::signbit(ZA_FLOAT_NAN)); // a positive quiet NaN, like `quiet_NaN()`
}
