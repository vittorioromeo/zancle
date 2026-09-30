#include "StringifyStringViewUtil.hpp" // IWYU pragma: keep
#include "StringifyZbStringUtil.hpp"   // IWYU pragma: keep
#include "Tst/Tst.hpp"

#include "Zancle/String/ToString.hpp"


TEST_CASE("[Base] Base/ToString.hpp")
{
    SECTION("Integer types")
    {
        SUBCASE("Zero")
        {
            CHECK(za::toString(0) == "0");
        }

        SUBCASE("Positive integers")
        {
            CHECK(za::toString(123) == "123");
            CHECK(za::toString(98'765) == "98765");
        }

        SUBCASE("Negative integers")
        {
            CHECK(za::toString(-456) == "-456");
            CHECK(za::toString(-1) == "-1");
        }

        SUBCASE("Integer limits")
        {
            CHECK(za::toString(int{2'147'483'647}) == "2147483647");   // INT_MAX
            CHECK(za::toString(int{-2'147'483'648}) == "-2147483648"); // INT_MIN

            CHECK(za::toString(static_cast<long long>(9'223'372'036'854'775'807ll)) == "9223372036854775807"); // LLONG_MAX
        }

        SUBCASE("Unsigned integers")
        {
            CHECK(za::toString(4'294'967'295u) == "4294967295"); // UINT_MAX
        }
    }

    SECTION("Floating-point types")
    {
        SUBCASE("Zero")
        {
            CHECK(za::toString(0.f) == "0.000000");
            CHECK(za::toString(0.0) == "0.000000");
        }

        SUBCASE("Positive floats")
        {
            // `123.456f` is actually stored as ~123.4560012817..., which matches
            // `std::to_chars` output at fixed precision 6.
            CHECK(za::toString(123.456f) == "123.456001");
            CHECK(za::toString(0.123) == "0.123000");
        }

        SUBCASE("Negative floats")
        {
            CHECK(za::toString(-78.9) == "-78.900000");
            // `-0.001f` is actually stored as ~-0.0010000000474..., matching `std::to_chars`.
            CHECK(za::toString(-0.001f) == "-0.001000");
        }

        SUBCASE("Integer-like floats")
        {
            CHECK(za::toString(100.0) == "100.000000");
            CHECK(za::toString(-50.f) == "-50.000000");
        }

        SUBCASE("Small fractional part requires padding")
        {
            CHECK(za::toString(1.01) == "1.010000");
        }
    }
}


TEST_CASE("[Base] toString of large floating-point values")
{
    CHECK(za::toString(1e13) == "10000000000000.000000"); // used to fail (assertion, or huge allocation)
    CHECK(za::toString(__FLT_MAX__) == "340282346638528859811704183484516925440.000000");
    CHECK(za::toString(__DBL_MAX__).size() == 309u + 1u + 6u);
    CHECK(za::toString(-__DBL_MAX__).size() == 1u + 309u + 1u + 6u);

    za::String s{"x="};
    za::appendToString(s, 1e20);
    CHECK(s == "x=100000000000000000000.000000");
}
