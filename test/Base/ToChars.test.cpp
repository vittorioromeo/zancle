#include "StringifyStringViewUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/String/ToChars.hpp"

#include "Zancle/String/StringView.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"

#include <limits>


#define CHECK_INTEGER_CONVERSION(value, expected_str)                                        \
    do                                                                                       \
    {                                                                                        \
        char              buffer[64];                                                        \
        const char* const last = buffer + sizeof(buffer);                                    \
        const char*       end  = za::toChars(buffer, last, value);                           \
                                                                                             \
        CHECK(end != nullptr);                                                               \
        CHECK(za::StringView(buffer, static_cast<za::SizeT>(end - buffer)) == expected_str); \
    } while (false)


#define CHECK_FLOAT_CONVERSION(value, expected_str, precision)                               \
    do                                                                                       \
    {                                                                                        \
        char              buffer[64];                                                        \
        const char* const last = buffer + sizeof(buffer);                                    \
        const char*       end  = za::toChars(buffer, last, value, precision);                \
                                                                                             \
        CHECK(end != nullptr);                                                               \
        CHECK(za::StringView(buffer, static_cast<za::SizeT>(end - buffer)) == expected_str); \
    } while (false)


TEST_CASE("[Base] ToChars.hpp")
{
    SECTION("Unsigned Integers")
    {
        CHECK_INTEGER_CONVERSION(0u, "0");
        CHECK_INTEGER_CONVERSION(123u, "123");
        CHECK_INTEGER_CONVERSION(std::numeric_limits<unsigned int>::max(), "4294967295");
        CHECK_INTEGER_CONVERSION(std::numeric_limits<za::U64>::max(), "18446744073709551615");
    }

    SECTION("Signed Integers")
    {
        CHECK_INTEGER_CONVERSION(0, "0");
        CHECK_INTEGER_CONVERSION(456, "456");
        CHECK_INTEGER_CONVERSION(-789, "-789");
        CHECK_INTEGER_CONVERSION(std::numeric_limits<int>::max(), "2147483647");

        // Special case: T_MIN requires correct unsigned handling
        CHECK_INTEGER_CONVERSION(std::numeric_limits<int>::min(), "-2147483648");
        CHECK_INTEGER_CONVERSION(std::numeric_limits<za::I64>::min(), "-9223372036854775808");
    }

    SECTION("Narrow signed types (must survive integer promotion of -uValue)")
    {
        // For types narrower than `int`, unary `-` on the unsigned counterpart
        // promotes to `int` first; without an explicit cast back, the result
        // would be a negative `int` and the digit-extraction loop would
        // silently print nothing after the leading '-'.
        CHECK_INTEGER_CONVERSION(static_cast<short>(-32'000), "-32000");
        CHECK_INTEGER_CONVERSION(static_cast<short>(32'000), "32000");
        CHECK_INTEGER_CONVERSION(std::numeric_limits<short>::min(), "-32768");
        CHECK_INTEGER_CONVERSION(std::numeric_limits<short>::max(), "32767");

        CHECK_INTEGER_CONVERSION(static_cast<signed char>(-100), "-100");
        CHECK_INTEGER_CONVERSION(std::numeric_limits<signed char>::min(), "-128");
        CHECK_INTEGER_CONVERSION(std::numeric_limits<signed char>::max(), "127");
    }

    SECTION("bool")
    {
        CHECK_INTEGER_CONVERSION(false, "0");
        CHECK_INTEGER_CONVERSION(true, "1");
    }

    SECTION("Narrow unsigned types: bool / unsigned char / unsigned short")
    {
        // bool
        CHECK_INTEGER_CONVERSION(static_cast<bool>(0), "0");
        CHECK_INTEGER_CONVERSION(static_cast<bool>(1), "1");

        // unsigned char: digit-count transitions 0/9, 9/10, 99/100, max
        CHECK_INTEGER_CONVERSION(static_cast<unsigned char>(0), "0");
        CHECK_INTEGER_CONVERSION(static_cast<unsigned char>(9), "9");
        CHECK_INTEGER_CONVERSION(static_cast<unsigned char>(10), "10");
        CHECK_INTEGER_CONVERSION(static_cast<unsigned char>(99), "99");
        CHECK_INTEGER_CONVERSION(static_cast<unsigned char>(100), "100");
        CHECK_INTEGER_CONVERSION(static_cast<unsigned char>(200), "200");
        CHECK_INTEGER_CONVERSION(std::numeric_limits<unsigned char>::max(), "255");

        // unsigned short
        CHECK_INTEGER_CONVERSION(static_cast<unsigned short>(0), "0");
        CHECK_INTEGER_CONVERSION(static_cast<unsigned short>(999), "999");
        CHECK_INTEGER_CONVERSION(static_cast<unsigned short>(1000), "1000");
        CHECK_INTEGER_CONVERSION(static_cast<unsigned short>(9999), "9999");
        CHECK_INTEGER_CONVERSION(static_cast<unsigned short>(10'000), "10000");
        CHECK_INTEGER_CONVERSION(std::numeric_limits<unsigned short>::max(), "65535");
    }

    SECTION("u32 / i32 digit-count transitions")
    {
        // u32: every 10^N boundary, both sides
        CHECK_INTEGER_CONVERSION(0u, "0");
        CHECK_INTEGER_CONVERSION(1u, "1");
        CHECK_INTEGER_CONVERSION(9u, "9");
        CHECK_INTEGER_CONVERSION(10u, "10");
        CHECK_INTEGER_CONVERSION(99u, "99");
        CHECK_INTEGER_CONVERSION(100u, "100");
        CHECK_INTEGER_CONVERSION(999u, "999");
        CHECK_INTEGER_CONVERSION(1000u, "1000");
        CHECK_INTEGER_CONVERSION(9999u, "9999");
        CHECK_INTEGER_CONVERSION(10'000u, "10000");
        CHECK_INTEGER_CONVERSION(99'999u, "99999");
        CHECK_INTEGER_CONVERSION(100'000u, "100000");
        CHECK_INTEGER_CONVERSION(999'999u, "999999");
        CHECK_INTEGER_CONVERSION(1'000'000u, "1000000");
        CHECK_INTEGER_CONVERSION(9'999'999u, "9999999");
        CHECK_INTEGER_CONVERSION(10'000'000u, "10000000");
        CHECK_INTEGER_CONVERSION(99'999'999u, "99999999");
        CHECK_INTEGER_CONVERSION(100'000'000u, "100000000");
        CHECK_INTEGER_CONVERSION(999'999'999u, "999999999");
        CHECK_INTEGER_CONVERSION(1'000'000'000u, "1000000000");
        CHECK_INTEGER_CONVERSION(4'294'967'295u, "4294967295");

        // i32: negative side digit-count transitions
        CHECK_INTEGER_CONVERSION(-1, "-1");
        CHECK_INTEGER_CONVERSION(-9, "-9");
        CHECK_INTEGER_CONVERSION(-10, "-10");
        CHECK_INTEGER_CONVERSION(-99, "-99");
        CHECK_INTEGER_CONVERSION(-100, "-100");
        CHECK_INTEGER_CONVERSION(-999, "-999");
        CHECK_INTEGER_CONVERSION(-1000, "-1000");
        CHECK_INTEGER_CONVERSION(-1'000'000, "-1000000");
        CHECK_INTEGER_CONVERSION(-2'147'483'647, "-2147483647");
    }

    SECTION("u64 / i64 digit-count transitions")
    {
        using za::I64;
        using za::U64;

        // Every 10^N boundary for u64 (digits 10..20)
        CHECK_INTEGER_CONVERSION(static_cast<U64>(9'999'999'999ull), "9999999999");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(10'000'000'000ull), "10000000000");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(99'999'999'999ull), "99999999999");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(100'000'000'000ull), "100000000000");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(999'999'999'999ull), "999999999999");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(1'000'000'000'000ull), "1000000000000");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(9'999'999'999'999ull), "9999999999999");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(10'000'000'000'000ull), "10000000000000");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(99'999'999'999'999ull), "99999999999999");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(100'000'000'000'000ull), "100000000000000");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(999'999'999'999'999ull), "999999999999999");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(1'000'000'000'000'000ull), "1000000000000000");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(9'999'999'999'999'999ull), "9999999999999999");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(10'000'000'000'000'000ull), "10000000000000000");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(99'999'999'999'999'999ull), "99999999999999999");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(100'000'000'000'000'000ull), "100000000000000000");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(999'999'999'999'999'999ull), "999999999999999999");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(1'000'000'000'000'000'000ull), "1000000000000000000");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(9'999'999'999'999'999'999ull), "9999999999999999999");
        CHECK_INTEGER_CONVERSION(static_cast<U64>(10'000'000'000'000'000'000ull), "10000000000000000000");
        CHECK_INTEGER_CONVERSION(std::numeric_limits<U64>::max(), "18446744073709551615");

        // i64: T_MAX and a negative T_MAX
        CHECK_INTEGER_CONVERSION(static_cast<I64>(9'223'372'036'854'775'807ll), "9223372036854775807");
        CHECK_INTEGER_CONVERSION(static_cast<I64>(-9'223'372'036'854'775'807ll), "-9223372036854775807");
        // T_MIN already covered in the "Signed Integers" section.
    }

    SECTION("Integer buffer overrun: exact-size & undersize at multiple digit counts")
    {
        char              buffer[32];
        const char* const last = buffer + sizeof(buffer);
        (void)last;

        // 1-digit (worst case for the "tail" branch)
        REQUIRE(za::toChars(buffer, buffer + 0, 7u) == nullptr);
        REQUIRE(za::toChars(buffer, buffer + 1, 7u) == buffer + 1);
        CHECK(buffer[0] == '7');

        // 2-digit (boundary of the digit-pair lookup)
        REQUIRE(za::toChars(buffer, buffer + 1, 42u) == nullptr);
        REQUIRE(za::toChars(buffer, buffer + 2, 42u) == buffer + 2);
        CHECK(za::StringView(buffer, 2) == "42");

        // 3-digit (one loop iter + tail)
        REQUIRE(za::toChars(buffer, buffer + 2, 999u) == nullptr);
        REQUIRE(za::toChars(buffer, buffer + 3, 999u) == buffer + 3);
        CHECK(za::StringView(buffer, 3) == "999");

        // 4-digit (two pair writes via the loop)
        REQUIRE(za::toChars(buffer, buffer + 3, 1234u) == nullptr);
        REQUIRE(za::toChars(buffer, buffer + 4, 1234u) == buffer + 4);
        CHECK(za::StringView(buffer, 4) == "1234");

        // 20-digit max u64
        const auto u64Max = std::numeric_limits<za::U64>::max();
        REQUIRE(za::toChars(buffer, buffer + 19, u64Max) == nullptr);
        REQUIRE(za::toChars(buffer, buffer + 20, u64Max) == buffer + 20);
        CHECK(za::StringView(buffer, 20) == "18446744073709551615");

        // Negative buffer-overrun (sign char doesn't fit)
        REQUIRE(za::toChars(buffer, buffer + 0, -1) == nullptr);
        REQUIRE(za::toChars(buffer, buffer + 1, -1) == nullptr); // '-' fits, digit doesn't
        REQUIRE(za::toChars(buffer, buffer + 2, -1) == buffer + 2);
        CHECK(za::StringView(buffer, 2) == "-1");
    }

    SECTION("Compile-time evaluation (constexpr)")
    {
        // Verify `toChars` works in a `consteval`-friendly context.
        constexpr auto fmt = [](auto v)
        {
            char        buf[32]{};
            char* const end = za::toChars(buf, buf + sizeof(buf), v);
            return end != nullptr && (end - buf) > 0;
        };

        static_assert(fmt(0));
        static_assert(fmt(42u));
        static_assert(fmt(-12'345));
        static_assert(fmt(std::numeric_limits<za::U64>::max()));
        static_assert(fmt(std::numeric_limits<za::I64>::min()));
    }

    SECTION("Buffer Overrun Checks")
    {
        // Value "12345" requires 5 characters
        const int val = 12'345;
        char      buffer[10];

        // Buffer too small by one
        REQUIRE(za::toChars(buffer, buffer + 4, val) == nullptr);

        // Buffer is exactly the right size
        const char* end = za::toChars(buffer, buffer + 5, val);
        REQUIRE(end == buffer + 5);
        REQUIRE(za::StringView(buffer, static_cast<za::SizeT>(end - buffer)) == "12345");

        // Negative value "-987" requires 4 characters
        const int negVal = -987;

        // Buffer too small by one
        REQUIRE(za::toChars(buffer, buffer + 3, negVal) == nullptr);

        // Buffer is exactly the right size
        end = za::toChars(buffer, buffer + 4, negVal);
        REQUIRE(end == buffer + 4);
        REQUIRE(za::StringView(buffer, static_cast<za::SizeT>(end - buffer)) == "-987");

        // Zero with an empty buffer
        REQUIRE(za::toChars(buffer, buffer, 0) == nullptr);
    }

    SECTION("Basic Conversions with default precision (6)")
    {
        CHECK_FLOAT_CONVERSION(0.0, "0.000000", 6);
        CHECK_FLOAT_CONVERSION(123.456, "123.456000", 6);
        CHECK_FLOAT_CONVERSION(-78.9, "-78.900000", 6);
        CHECK_FLOAT_CONVERSION(0.123456, "0.123456", 6);
        CHECK_FLOAT_CONVERSION(98765.0, "98765.000000", 6);
    }

    SECTION("Rounding behavior")
    {
        // Rounds up
        CHECK_FLOAT_CONVERSION(1.2345678, "1.234568", 6);
        // Rounds down
        CHECK_FLOAT_CONVERSION(1.1234561, "1.123456", 6);
        // Rounds up and carries over to integer part
        CHECK_FLOAT_CONVERSION(9.9999999, "10.000000", 6);
        CHECK_FLOAT_CONVERSION(-9.9999999, "-10.000000", 6);
    }

    SECTION("Custom Precision")
    {
        CHECK_FLOAT_CONVERSION(3.14159, "3.14", 2);
        CHECK_FLOAT_CONVERSION(1.234, "1.2340000000", 10);
        CHECK_FLOAT_CONVERSION(0.007, "0.01", 2); // Handles leading zeros in fraction
        CHECK_FLOAT_CONVERSION(-0.007, "-0.01", 2);

        // Precision 0 should not print a decimal point
        CHECK_FLOAT_CONVERSION(123.456, "123", 0);
        CHECK_FLOAT_CONVERSION(0.7, "1", 0); // rint(0.7) is 1
    }

#if !__FINITE_MATH_ONLY__
    // These cases test behavior that `-ffinite-math-only` deliberately turns
    // off: under that flag the compiler is allowed to assume NaN/inf never
    // appear, and `numeric_limits<double>::infinity()` / `quiet_NaN()` (and
    // even the `-0.0` literal) become UB. `toChars` documents the same
    // precondition, so the special-value contract only applies when the flag
    // is off.
    SECTION("Special values: NaN, infinity, negative zero")
    {
        // NaN: "nan", no sign (matches `std::to_chars` / IEEE-754)
        CHECK_FLOAT_CONVERSION(std::numeric_limits<double>::quiet_NaN(), "nan", 6);

        // +/- infinity
        CHECK_FLOAT_CONVERSION(std::numeric_limits<double>::infinity(), "inf", 6);
        CHECK_FLOAT_CONVERSION(-std::numeric_limits<double>::infinity(), "-inf", 6);

        // Negative zero preserves its sign (matches `std::to_chars`)
        CHECK_FLOAT_CONVERSION(-0.0, "-0.000000", 6);
        CHECK_FLOAT_CONVERSION(-0.0, "-0.00", 2);
        CHECK_FLOAT_CONVERSION(-0.0, "-0", 0);
    }
#endif

    SECTION("Banker's rounding (round-half-to-even)")
    {
        // `rint` uses the default IEEE-754 rounding mode (round-half-to-even),
        // matching `std::to_chars`. Half-cases round to the nearest even integer.
        CHECK_FLOAT_CONVERSION(0.5, "0", 0); // 0.5 → 0 (0 is even)
        CHECK_FLOAT_CONVERSION(1.5, "2", 0); // 1.5 → 2 (2 is even)
        CHECK_FLOAT_CONVERSION(2.5, "2", 0); // 2.5 → 2 (2 is even)
        CHECK_FLOAT_CONVERSION(3.5, "4", 0); // 3.5 → 4 (4 is even)
    }

    SECTION("Values that used to be out of range")
    {
        char              buffer[400];
        const char* const last = buffer + sizeof(buffer);

        // Beyond ~9.2e18: printed exactly
        const char* end = za::toChars(buffer, last, 1.0e20, 0);
        CHECK(za::StringView{buffer, static_cast<za::SizeT>(end - buffer)} == "100000000000000000000");

        end = za::toChars(buffer, last, std::numeric_limits<double>::max(), 0);
        CHECK(end - buffer == 309);

        // Precision 10 no longer overflows for 1e10
        end = za::toChars(buffer, last, 1.0e10, 10);
        CHECK(za::StringView{buffer, static_cast<za::SizeT>(end - buffer)} == "10000000000.0000000000");

        // ...but does not fit in a small buffer
        char small[16];
        CHECK(za::toChars(small, small + sizeof(small), 1.0e20, 0) == nullptr);
    }

    SECTION("Buffer Overrun Checks")
    {
        // Value 12.345 with precision 3 is "12.345" (6 chars)
        const double val  = 12.345;
        const int    prec = 3;
        char         buffer[10];

        // Buffer too small by one
        REQUIRE(za::toChars(buffer, buffer + 5, val, prec) == nullptr);

        // Buffer is exactly the right size
        const char* end = za::toChars(buffer, buffer + 6, val, prec);
        REQUIRE(end == buffer + 6);
        REQUIRE(za::StringView(buffer, static_cast<za::SizeT>(end - buffer)) == "12.345");

        // Negative value -1.2 with precision 1 is "-1.2" (4 chars)
        const double negVal  = -1.2;
        const int    negPrec = 1;

        // Buffer too small by one
        REQUIRE(za::toChars(buffer, buffer + 3, negVal, negPrec) == nullptr);

        // Buffer is exactly the right size
        end = za::toChars(buffer, buffer + 4, negVal, negPrec);
        REQUIRE(end == buffer + 4);
        REQUIRE(za::StringView(buffer, static_cast<za::SizeT>(end - buffer)) == "-1.2");

        // Case where integer part fits but rest does not
        // "123.4" (5 chars), buffer for only "123" (3 chars)
        REQUIRE(za::toChars(buffer, buffer + 3, 123.4, 1) == nullptr);
    }
}


namespace
{
namespace ToCharsLargeValuesTest // for unity builds
{
////////////////////////////////////////////////////////////
// `__extension__`: 128-bit integers are a GCC/Clang extension (avoids `-Wpedantic`)
__extension__ using I128 = __int128;
__extension__ using U128 = unsigned __int128;


////////////////////////////////////////////////////////////
[[nodiscard]] za::StringView format(char (&buffer)[400], const double value, const int precision)
{
    const char* const end = za::toChars(buffer, buffer + 400, value, precision);
    return end == nullptr ? za::StringView{"<nullptr>"} : za::StringView{buffer, static_cast<za::SizeT>(end - buffer)};
}


////////////////////////////////////////////////////////////
template <typename T>
concept CanFormat = requires(char* p, T v) { za::toChars(p, p, v); };

} // namespace ToCharsLargeValuesTest
} // namespace


TEST_CASE("[Base] toChars(double) at any magnitude")
{
    using ToCharsLargeValuesTest::format;
    char buffer[400];

    SECTION("Values that used to exceed the 9e18 limit once scaled")
    {
        CHECK(format(buffer, 1e13, 6) == "10000000000000.000000");
        CHECK(format(buffer, -123'456'789'012.5, 3) == "-123456789012.500");
        CHECK(format(buffer, 9'007'199'254'740'991.0, 2) == "9007199254740991.00"); // 2^53 - 1
        CHECK(format(buffer, 4'503'599'627'370'495.5, 1) == "4503599627370495.5");
    }

    SECTION("Values from 2^53 onwards are printed exactly")
    {
        CHECK(format(buffer, 9'007'199'254'740'992.0, 1) == "9007199254740992.0");        // 2^53
        CHECK(format(buffer, 18'446'744'073'709'551'616.0, 0) == "18446744073709551616"); // 2^64
        CHECK(format(buffer, 1e20, 2) == "100000000000000000000.00");
        CHECK(format(buffer, 1e23, 0) == "99999999999999991611392"); // exact value of the double nearest to 1e23

        CHECK(format(buffer, static_cast<double>(__FLT_MAX__), 0) == "340282346638528859811704183484516925440");
        CHECK(format(buffer, -__DBL_MAX__, 1) ==
              "-1797693134862315708145274237317043567980705675258449965989174768031572607800285387605895586327668781715"
              "4"
              "04589535143824642343213268894641827684675467035375169860499105765512820762454900903893289440758685084551"
              "33942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368."
              "0");
    }

    SECTION("Rounding carries into the integer part")
    {
        CHECK(format(buffer, 0.9999999, 6) == "1.000000");
        CHECK(format(buffer, 99.9999999, 3) == "100.000");
        CHECK(format(buffer, 2.5, 0) == "2"); // ties to even
        CHECK(format(buffer, 3.5, 0) == "4");
    }

    SECTION("Buffer too small for a large value")
    {
        char small[10];
        CHECK(za::toChars(small, small + 10, 1e20, 0) == nullptr);
    }

    SECTION("128-bit integers are rejected at compile time")
    {
        STATIC_CHECK(ToCharsLargeValuesTest::CanFormat<unsigned long long>);
        STATIC_CHECK(!ToCharsLargeValuesTest::CanFormat<ToCharsLargeValuesTest::U128>);
        STATIC_CHECK(!ToCharsLargeValuesTest::CanFormat<ToCharsLargeValuesTest::I128>);
    }
}
