#include "Tst/Tst.hpp"

#include "Zancle/String/FromChars.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/ToString.hpp"

#include "Zancle/Base/IntTypes.hpp"

#include <limits>


TEST_CASE("[Base] FromChars.hpp")
{
    SECTION("fromChars - Integral Types")
    {
        SECTION("Valid Signed Integers")
        {
            int         value  = 0;
            const char* str    = "12345";
            auto        result = za::fromChars(str, str + 5, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 5);
            CHECK(value == 12'345);

            str    = "+678";
            result = za::fromChars(str, str + 4, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 4);
            CHECK(value == 678);

            str    = "-987";
            result = za::fromChars(str, str + 4, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 4);
            CHECK(value == -987);

            str    = "0";
            result = za::fromChars(str, str + 1, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 1);
            CHECK(value == 0);
        }

        SECTION("Valid Unsigned Integers")
        {
            unsigned int value  = 0;
            const char*  str    = "12345";
            auto         result = za::fromChars(str, str + 5, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 5);
            CHECK(value == 12'345);

            str    = "+678";
            result = za::fromChars(str, str + 4, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 4);
            CHECK(value == 678);
        }

        SECTION("Partial Parsing")
        {
            int         value  = 0;
            const char* str    = "99bottles";
            const auto  result = za::fromChars(str, str + 9, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 2); // Should point to 'b'
            CHECK(value == 99);
        }

        SECTION("Max/Min Values")
        {
            // Signed int max
            int        iValue  = 0;
            za::String iMaxStr = za::toString(std::numeric_limits<int>::max());
            auto       result  = za::fromChars(iMaxStr.cStr(), iMaxStr.cStr() + iMaxStr.size(), iValue);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(iValue == std::numeric_limits<int>::max());

            // Signed int min
            za::String iMinStr = za::toString(std::numeric_limits<int>::min());
            result             = za::fromChars(iMinStr.cStr(), iMinStr.cStr() + iMinStr.size(), iValue);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(iValue == std::numeric_limits<int>::min());

            // Signed long long min
            long long  llValue  = 0;
            za::String llMinStr = za::toString(std::numeric_limits<long long>::min());
            result              = za::fromChars(llMinStr.cStr(), llMinStr.cStr() + llMinStr.size(), llValue);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(llValue == std::numeric_limits<long long>::min());

            // Unsigned long long max
            unsigned long long ullValue     = 0;
            za::String         ullMaxString = za::toString(std::numeric_limits<unsigned long long>::max());
            result = za::fromChars(ullMaxString.cStr(), ullMaxString.cStr() + ullMaxString.size(), ullValue);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(ullValue == std::numeric_limits<unsigned long long>::max());
        }

        SECTION("Error: Invalid Argument")
        {
            int         value  = 1; // Should not be modified
            const char* str    = "";
            auto        result = za::fromChars(str, str, value);
            CHECK(result.ec == za::FromCharsError::InvalidArgument);
            CHECK(value == 1);

            str    = "+";
            result = za::fromChars(str, str + 1, value);
            CHECK(result.ec == za::FromCharsError::InvalidArgument);
            CHECK(value == 1);

            str    = "-";
            result = za::fromChars(str, str + 1, value);
            CHECK(result.ec == za::FromCharsError::InvalidArgument);
            CHECK(value == 1);

            str    = "abc";
            result = za::fromChars(str, str + 3, value);
            CHECK(result.ec == za::FromCharsError::InvalidArgument);
            CHECK(value == 1);

            // Negative sign for unsigned type
            unsigned int uValue = 1;
            str                 = "-123";
            result              = za::fromChars(str, str + 4, uValue);
            CHECK(result.ec == za::FromCharsError::InvalidArgument);
            CHECK(uValue == 1);
        }

        SECTION("Error: Result Out Of Range")
        {
            // Overflow for signed int
            int        iValue       = 1; // Should not be modified
            za::String iOverflowStr = za::toString(std::numeric_limits<long long>::max());
            auto       result = za::fromChars(iOverflowStr.cStr(), iOverflowStr.cStr() + iOverflowStr.size(), iValue);
            CHECK(result.ec == za::FromCharsError::ResultOutOfRange);
            CHECK(iValue == 1);

            // Overflow for uint8_t
            za::U8      u8Value = 1;
            const char* str     = "256";
            result              = za::fromChars(str, str + 3, u8Value);
            CHECK(result.ec == za::FromCharsError::ResultOutOfRange);
            CHECK(u8Value == 1);

            str    = "1000";
            result = za::fromChars(str, str + 4, u8Value);
            CHECK(result.ec == za::FromCharsError::ResultOutOfRange);
            CHECK(u8Value == 1);
        }
    }

    SECTION("fromChars - Floating-Point Types")
    {
        SECTION("Valid Floats")
        {
            double      value  = 0.0;
            const char* str    = "123.456";
            auto        result = za::fromChars(str, str + 7, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 7);
            CHECK(value == tst::Approx(123.456));

            str    = "-0.123";
            result = za::fromChars(str, str + 6, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 6);
            CHECK(value == tst::Approx(-0.123));

            str    = "+789.";
            result = za::fromChars(str, str + 5, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 5);
            CHECK(value == tst::Approx(789.0));

            str    = "500";
            result = za::fromChars(str, str + 3, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 3);
            CHECK(value == tst::Approx(500.0));

            float fValue = 0.0f;
            str          = ".25";
            result       = za::fromChars(str, str + 3, fValue);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 3);
            CHECK(fValue == tst::Approx(0.25));
        }

        SECTION("Zero")
        {
            double      value  = 1.0;
            const char* str    = "0.0";
            auto        result = za::fromChars(str, str + 3, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(value == tst::Approx(0.0));

            str    = "0";
            result = za::fromChars(str, str + 1, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(value == tst::Approx(0.0));
        }

        SECTION("Partial Parsing")
        {
            double      value  = 0.0;
            const char* str    = "3.14159andthensome";
            auto        result = za::fromChars(str, str + 18, value);
            CHECK(result.ec == za::FromCharsError::None);
            CHECK(result.ptr == str + 7); // Should point to 'a'
            CHECK(value == tst::Approx(3.14159));
        }

        SECTION("Error: Invalid Argument")
        {
            double      value  = 1.0; // Should not be modified
            const char* str    = "";
            auto        result = za::fromChars(str, str, value);
            CHECK(result.ec == za::FromCharsError::InvalidArgument);
            CHECK(value == tst::Approx(1.0));

            str    = "+";
            result = za::fromChars(str, str + 1, value);
            CHECK(result.ec == za::FromCharsError::InvalidArgument);
            CHECK(value == tst::Approx(1.0));

            str    = "-";
            result = za::fromChars(str, str + 1, value);
            CHECK(result.ec == za::FromCharsError::InvalidArgument);
            CHECK(value == tst::Approx(1.0));

            str    = ".";
            result = za::fromChars(str, str + 1, value);
            CHECK(result.ec == za::FromCharsError::InvalidArgument);
            CHECK(value == tst::Approx(1.0));

            str    = "xyz";
            result = za::fromChars(str, str + 3, value);
            CHECK(result.ec == za::FromCharsError::InvalidArgument);
            CHECK(value == tst::Approx(1.0));
        }
    }
}


namespace
{
namespace FromCharsRangeTest // for unity builds
{
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] za::FromCharsError parse(const za::String& text, T& value)
{
    return za::fromChars(text.data(), text.data() + text.size(), value).ec;
}


////////////////////////////////////////////////////////////
template <typename T>
concept CanParse = requires(const char* p, T& v) { za::fromChars(p, p, v); };

} // namespace FromCharsRangeTest
} // namespace


TEST_CASE("[Base] fromChars floating-point range errors")
{
    using FromCharsRangeTest::parse;

    za::String huge{"1"};
    for (int i = 0; i < 400; ++i)
        huge += '0';

    za::String tiny{"0."};
    for (int i = 0; i < 400; ++i)
        tiny += '0';
    tiny += '1';

    SECTION("double")
    {
        double value = 42.0;

        CHECK(parse(huge, value) == za::FromCharsError::ResultOutOfRange);
        CHECK(value == 42.0); // untouched

        CHECK(parse(tiny, value) == za::FromCharsError::ResultOutOfRange);
        CHECK(value == 42.0);

        CHECK(parse(za::String{"-"} + huge, value) == za::FromCharsError::ResultOutOfRange);

        // Zero is not an underflow
        CHECK(parse(za::String{"0.000000000000000000000000000000000000000000000000000000000"}, value) ==
              za::FromCharsError::None);
        CHECK(value == 0.0);

        CHECK(parse(za::String{"1e308"}, value) == za::FromCharsError::None); // exponents are not parsed: "1"
    }

    SECTION("float")
    {
        float value = 42.f;

        CHECK(parse(za::String{"1000000000000000000000000000000000000000"}, value) ==
              za::FromCharsError::ResultOutOfRange); // 1e39 > FLT_MAX
        CHECK(value == 42.f);

        CHECK(parse(za::String{"340000000000000000000000000000000000000"}, value) == za::FromCharsError::None);
        CHECK(value > 3.39e38f);

        CHECK(parse(za::String{"0.0000000000000000000000000000000000000000000000000001"}, value) ==
              za::FromCharsError::ResultOutOfRange); // 1e-52 underflows `float`
    }
}


TEST_CASE("[Base] fromChars integer overflow boundaries")
{
    using FromCharsRangeTest::parse;

    za::I8 i8 = 0;
    CHECK(parse(za::String{"-128"}, i8) == za::FromCharsError::None);
    CHECK(i8 == -128);
    CHECK(parse(za::String{"127"}, i8) == za::FromCharsError::None);
    CHECK(i8 == 127);
    CHECK(parse(za::String{"-129"}, i8) == za::FromCharsError::ResultOutOfRange);
    CHECK(parse(za::String{"128"}, i8) == za::FromCharsError::ResultOutOfRange);

    za::U8 u8 = 0;
    CHECK(parse(za::String{"255"}, u8) == za::FromCharsError::None);
    CHECK(u8 == 255);
    CHECK(parse(za::String{"256"}, u8) == za::FromCharsError::ResultOutOfRange);

    za::I64 i64 = 0;
    CHECK(parse(za::String{"-9223372036854775808"}, i64) == za::FromCharsError::None);
    CHECK(i64 == std::numeric_limits<za::I64>::min());
    CHECK(parse(za::String{"-9223372036854775809"}, i64) == za::FromCharsError::ResultOutOfRange);
    CHECK(parse(za::String{"9223372036854775808"}, i64) == za::FromCharsError::ResultOutOfRange);

    // `bool` is not an integer to parse
    STATIC_CHECK(FromCharsRangeTest::CanParse<int>);
    STATIC_CHECK(!FromCharsRangeTest::CanParse<bool>);
}
