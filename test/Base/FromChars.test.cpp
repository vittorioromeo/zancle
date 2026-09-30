#include "Tst/Tst.hpp"

#include "Zancle/String/FromChars.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/ToString.hpp"

#include "Zancle/Base/IntTypes.hpp"

#include <initializer_list>
#include <limits>

#include <cmath>
#include <cstdio>
#include <cstring>


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

        CHECK(parse(za::String{"1e308"}, value) == za::FromCharsError::None);
        CHECK(value == 1e308);

        CHECK(parse(za::String{"1e309"}, value) == za::FromCharsError::ResultOutOfRange);
        CHECK(value == 1e308); // untouched
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


namespace
{
namespace FromCharsExactTest // for unity builds
{
////////////////////////////////////////////////////////////
template <typename T>
struct Parsed
{
    za::FromCharsError ec;
    int                consumed; // number of characters consumed
    T                  value;    // `T(42)` if left untouched
};


////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] Parsed<T> parse(const za::StringView text)
{
    T          value  = T(42);
    const auto result = za::fromChars(text.data(), text.data() + text.size(), value);
    return {result.ec, static_cast<int>(result.ptr - text.data()), value};
}


////////////////////////////////////////////////////////////
/// \brief Whether `text` parses fully into exactly `expected` (bit for bit, e.g. the sign of zero)
///
template <typename T>
[[nodiscard]] bool parsesTo(const za::StringView text, const T expected)
{
    const auto parsed = parse<T>(text);

    return parsed.ec == za::FromCharsError::None && parsed.consumed == static_cast<int>(text.size()) &&
           std::memcmp(&parsed.value, &expected, sizeof(T)) == 0;
}


////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] bool isNegative(const T value)
{
    return std::signbit(value);
}


////////////////////////////////////////////////////////////
/// \brief Whether `text` fails with `ResultOutOfRange`, consuming it fully and leaving the value untouched
///
template <typename T>
[[nodiscard]] bool isOutOfRange(const za::StringView text)
{
    const auto parsed = parse<T>(text);

    return parsed.ec == za::FromCharsError::ResultOutOfRange && parsed.consumed == static_cast<int>(text.size()) &&
           parsed.value == T(42);
}


////////////////////////////////////////////////////////////
/// \brief Whether `text` fails with `InvalidArgument`, consuming nothing and leaving the value untouched
///
template <typename T>
[[nodiscard]] bool isInvalid(const za::StringView text)
{
    const auto parsed = parse<T>(text);
    return parsed.ec == za::FromCharsError::InvalidArgument && parsed.consumed == 0 && parsed.value == T(42);
}


////////////////////////////////////////////////////////////
[[nodiscard]] za::String repeated(const char c, const int count)
{
    za::String result;

    for (int i = 0; i < count; ++i)
        result += c;

    return result;
}


////////////////////////////////////////////////////////////
// Exact decimal values of halfway points between adjacent doubles
constexpr const char* doubleOneHalfway = "1.00000000000000011102230246251565404236316680908203125"; // 1 + 2^-53

constexpr const char* doubleMaxHalfway = // `DBL_MAX` + half an ulp
    "1.79769313486231580793728971405303415079934132710037826936173778980444968292764750946649017977587207096330286416"
    "6928879109465555478519404026306574886715058206819089020007083836762738548458177115317644757302700698555713669596"
    "22842914819860834936475292719074168444365510704342711559699508093042880177904174497792e+308";

constexpr const char* doubleSubnormalHalfway = // 2^-1075, half of the smallest subnormal
    "2.47032822920623272088284396434110686182529901307162382212792841250337753635104375932649918180817996189898282347"
    "7228588654633283551779698981993873980053909390631503565951557022639229085839244910518443593180284993653615250031"
    "9370457678249219365623669863658480757001585769269903706311928279558551332927834338409351978015531246597263579574"
    "6227664652728272200563740064854999770965994704540208281662262378573934507363390079677619305775067401763246736009"
    "6895134053553745851666113422376667860416215968046191446729184030053005753084904876539171138659164623952491262365"
    "3881879636239373280423891018672348497668235089863388587925628302755995657524455507255189313690836254779186948667"
    "994968324049705821028513185451396213837722826145437693412532098591327667236328125e-324";

constexpr const char* doubleThreeSubnormalHalfway = // 3 * 2^-1075, between 1 and 2 times the smallest subnormal
    "7.41098468761869816264853189302332058547589703921487146638378523751013260905313127797949754542453988569694847043"
    "1685765963899850655339096945981621940161728171894510697854671067917687257517734731555330779540854980960845750095"
    "8111373034747658096871009590975442271004757307809711118935784838675653998783503015228055934046593739791790738723"
    "8682993958184816601691220194564999312897984113620624844986787135721803522090170239032857917325202205289740208029"
    "0685402160661237554998340267130003581248647904138574340187552090159017259254714629617513415977493871857473787096"
    "1645638908718119841271673056017045493004705269590165763776884908267986972573366521765567941072508764337560846003"
    "984904972149117463085539556354188641513168478436313080237596295773983001708984375e-324";

constexpr const char* doubleMinNormalHalfway = // between the largest subnormal and the smallest normal
    "2.22507385850720113605740979670913197593481954635164564802342610972482222202107694551652952390813508791414915891"
    "3039621106870086438694594645527657207407820621743379988141063267329253552286881372149012981122451451889849057222"
    "3072852551331557550159143974763979834118019993239625482890171070818506906306666559949382757725720157630626906633"
    "3264756530000924588831643303777979186961204949739037782970490505108060994073026293712895895000358379996720725430"
    "4360284078895771796150945516748243471030702609144621572289880258182545180325707018860872113128079512233426288368"
    "6223215037756666225039825343359745688844239002654981983854879482922068947216898310996983658468140228542433306603"
    "3985088644580400103493397042756718644338377048603786162277173854562306587467901408672332763671875e-308";

} // namespace FromCharsExactTest
} // namespace


TEST_CASE("[Base] fromChars floating-point exactness")
{
    using FromCharsExactTest::isOutOfRange;
    using FromCharsExactTest::parse;
    using FromCharsExactTest::parsesTo;
    using FromCharsExactTest::repeated;

    SECTION("Correctly rounded, whatever the path")
    {
        CHECK(parsesTo("0.1", 0.1));
        CHECK(parsesTo("123.456", 123.456));
        CHECK(parsesTo("1e22", 1e22));
        CHECK(parsesTo("1e23", 1e23)); // not exactly representable
        CHECK(parsesTo("0.30000000000000004", 0.30000000000000004));
        CHECK(parsesTo("8.98846567431158e307", 8.98846567431158e307));
        CHECK(parsesTo("4.35679e-10", 4.35679e-10));
        CHECK(parsesTo("123456789012345678901234567890", 123456789012345678901234567890.0));
        CHECK(parsesTo("2.2250738585072014e-308", 2.2250738585072014e-308)); // `DBL_MIN`
        CHECK(parsesTo("2.2250738585072011e-308", 2.2250738585072011e-308)); // largest subnormal
        CHECK(parsesTo("1.7976931348623157e308", 1.7976931348623157e308));   // `DBL_MAX`
        CHECK(parsesTo("1.7976931348623158e308", 1.7976931348623157e308));   // below the halfway point
        CHECK(parsesTo("4.9406564584124654e-324", 4.9406564584124654e-324)); // smallest subnormal
        CHECK(parsesTo("2.4703282292062328e-324", 4.9406564584124654e-324)); // just above half of it

        CHECK(parsesTo("0.1", 0.1f));
        CHECK(parsesTo("3.4028235e38", 3.4028235e38f)); // `FLT_MAX`
        CHECK(parsesTo("1.4e-45", 1.4e-45f));           // smallest subnormal
        CHECK(parsesTo("7.1e-46", 1.4e-45f));           // just above half of it
        CHECK(parsesTo("1.17549435e-38", 1.17549435e-38f));
    }

    SECTION("Halfway points round to even")
    {
        CHECK(parsesTo("9007199254740993", 9007199254740992.0));
        CHECK(parsesTo("9007199254740995", 9007199254740996.0));
        CHECK(parsesTo("9007199254740993.0000000000000000000000001", 9007199254740994.0));
        CHECK(parsesTo("9007199254740992.9999999999999999999999999", 9007199254740992.0));

        const za::String one = FromCharsExactTest::doubleOneHalfway;
        CHECK(parsesTo(one, 1.0));
        CHECK(parsesTo(one + "1", 1.0000000000000002));
        CHECK(parsesTo("1.00000000000000011102230246251565404236316680908203124999", 1.0));

        // Beyond the 800 significant digits that are kept exactly
        CHECK(parsesTo(one + repeated('0', 900), 1.0));
        CHECK(parsesTo(one + repeated('0', 900) + "1", 1.0000000000000002));
        CHECK(parsesTo(repeated('0', 900) + one + repeated('0', 900), 1.0));

        CHECK(parsesTo(FromCharsExactTest::doubleThreeSubnormalHalfway, 2 * 4.9406564584124654e-324));
        CHECK(parsesTo(FromCharsExactTest::doubleMinNormalHalfway, 2.2250738585072014e-308));

        CHECK(parsesTo("16777217", 16777216.f));
        CHECK(parsesTo("16777219", 16777220.f));
        CHECK(parsesTo("1.000000059604644775390625", 1.f));
        CHECK(parsesTo("1.0000000596046447753906250000001", 1.0000001f));
        CHECK(parsesTo("340282356779733661637539395458142568447", 3.4028235e38f)); // just below `FLT_MAX` + half
    }

    SECTION("Out of range: overflows and nonzero values rounding to zero")
    {
        CHECK(isOutOfRange<double>(FromCharsExactTest::doubleMaxHalfway));       // ties to even: `DBL_MAX` is odd
        CHECK(isOutOfRange<double>(FromCharsExactTest::doubleSubnormalHalfway)); // ties to even: zero
        CHECK(isOutOfRange<double>("2.4703282292062327e-324"));
        CHECK(isOutOfRange<double>("1e400"));
        CHECK(isOutOfRange<double>("-1e400"));
        CHECK(isOutOfRange<double>("1e-400"));
        CHECK(isOutOfRange<double>("1e99999999999999999999"));
        CHECK(isOutOfRange<double>("1e-99999999999999999999"));

        CHECK(isOutOfRange<float>("340282356779733661637539395458142568448")); // `FLT_MAX` + half
        CHECK(isOutOfRange<float>("1e39"));
        CHECK(isOutOfRange<float>("7e-46"));

        // The pointer is past the whole number, so that parsing can resume after it
        const auto parsed = parse<double>("1e400,2");
        CHECK(parsed.ec == za::FromCharsError::ResultOutOfRange);
        CHECK(parsed.consumed == 5);
        CHECK(parsed.value == 42.0);
    }

    SECTION("Round trips")
    {
        // Printing with enough digits (17 for `double`, 9 for `float`) and parsing back is exact
        za::U64 state = 12'345u;
        char    buffer[64];

        for (int i = 0; i < 20'000; ++i)
        {
            state              = state * 6'364'136'223'846'793'005u + 1'442'695'040'888'963'407u; // LCG
            const za::U64 bits = state ^ (state >> 29);

            double d = 0.0;
            std::memcpy(&d, &bits, sizeof(double));

            if (std::isfinite(d))
            {
                std::snprintf(buffer, sizeof(buffer), "%.17e", d);
                CHECK(parsesTo(buffer, d));

                std::snprintf(buffer, sizeof(buffer), "%.30e", d); // beyond 19 digits
                CHECK(parsesTo(buffer, d));
            }

            float f = 0.f;
            std::memcpy(&f, &bits, sizeof(float));

            if (std::isfinite(f))
            {
                std::snprintf(buffer, sizeof(buffer), "%.9e", static_cast<double>(f));
                CHECK(parsesTo(buffer, f));
            }
        }
    }
}


TEST_CASE("[Base] fromChars floating-point syntax")
{
    using FromCharsExactTest::isInvalid;
    using FromCharsExactTest::isNegative;
    using FromCharsExactTest::parse;
    using FromCharsExactTest::parsesTo;

    SECTION("Exponents")
    {
        CHECK(parsesTo("1e5", 1e5));
        CHECK(parsesTo("1E+05", 1e5));
        CHECK(parsesTo("2.5e-3", 2.5e-3));
        CHECK(parsesTo(".5e1", 5.0));
        CHECK(parsesTo("5.e1", 50.0));
        CHECK(parsesTo("0e99999999999", 0.0));
        CHECK(parsesTo("0.000000000000000000000000000000000000000000000000000000001e57", 1.0));

        // Consumed only if digits follow
        for (const char* text : {"1e", "1e+", "1e-", "1E", "1em", "1e+m"})
        {
            const auto parsed = parse<double>(text);
            CHECK(parsed.ec == za::FromCharsError::None);
            CHECK(parsed.consumed == 1);
            CHECK(parsed.value == 1.0);
        }

        CHECK(isInvalid<double>("e5"));
    }

    SECTION("Signs")
    {
        CHECK(parsesTo("+1.5", 1.5));
        CHECK(parsesTo("-1.5", -1.5));
        CHECK(parsesTo("-0", -0.0));
        CHECK(isNegative(parse<double>("-0").value));
        CHECK(isNegative(parse<double>("-0.0e10").value));
        CHECK(!isNegative(parse<double>("+0").value));

        CHECK(isInvalid<double>("+-1"));
        CHECK(isInvalid<double>("-+1"));
        CHECK(isInvalid<double>("- 1"));
    }

    SECTION("Infinity and NaN")
    {
        constexpr double infinity = std::numeric_limits<double>::infinity();

        CHECK(parsesTo("inf", infinity));
        CHECK(parsesTo("INF", infinity));
        CHECK(parsesTo("Infinity", infinity));
        CHECK(parsesTo("-infinity", -infinity));
        CHECK(parsesTo("+iNf", infinity));
        CHECK(parsesTo("inf", std::numeric_limits<float>::infinity()));

        // Longest match: "infin" is "inf" followed by "in"
        const auto infin = parse<double>("infin");
        CHECK(infin.ec == za::FromCharsError::None);
        CHECK(infin.consumed == 3);
        CHECK(infin.value == infinity);

        CHECK(isInvalid<double>("in"));
        CHECK(isInvalid<double>("i"));
        CHECK(isInvalid<double>("n"));
        CHECK(isInvalid<double>("na"));
        CHECK(isInvalid<float>("-in"));

        const auto checkNan = [](const char* text, const int consumed, const bool negative)
        {
            const auto parsed = parse<double>(text);
            CHECK(parsed.ec == za::FromCharsError::None);
            CHECK(parsed.consumed == consumed);
            CHECK(std::isnan(parsed.value));
            CHECK(isNegative(parsed.value) == negative);
        };

        checkNan("nan", 3, false);
        checkNan("NaN", 3, false);
        checkNan("-nan", 4, true);
        checkNan("nan(abc_123)", 12, false);
        checkNan("nan()", 5, false);
        checkNan("nan(", 3, false); // unterminated: not part of the number
        checkNan("nan(abc", 3, false);
        checkNan("nan(a-b)", 3, false); // '-' is not allowed in the sequence

        const auto nanFloat = parse<float>("NAN");
        CHECK(nanFloat.ec == za::FromCharsError::None);
        CHECK(std::isnan(nanFloat.value));
    }

    SECTION("Partial parsing and invalid inputs")
    {
        const auto parsed = parse<double>("1.5.5");
        CHECK(parsed.ec == za::FromCharsError::None);
        CHECK(parsed.consumed == 3);
        CHECK(parsed.value == 1.5);

        CHECK(isInvalid<double>(""));
        CHECK(isInvalid<double>("."));
        CHECK(isInvalid<double>("-."));
        CHECK(isInvalid<double>("..5"));
        CHECK(isInvalid<double>(" 1"));
    }

    SECTION("long double is parsed as double")
    {
        long double value  = 0.0L;
        const char* text   = "0.1";
        const auto  result = za::fromChars(text, text + 3, value);
        CHECK(result.ec == za::FromCharsError::None);
        CHECK(value == static_cast<long double>(0.1));
    }
}


TEST_CASE("[Base] fromChars integer pointer semantics")
{
    using FromCharsExactTest::isInvalid;
    using FromCharsExactTest::parse;

    SECTION("Overflow consumes the whole number")
    {
        const auto i8 = parse<za::I8>("128,1");
        CHECK(i8.ec == za::FromCharsError::ResultOutOfRange);
        CHECK(i8.consumed == 3);
        CHECK(i8.value == 42);

        const auto i32 = parse<int>("-99999999999999999999999x");
        CHECK(i32.ec == za::FromCharsError::ResultOutOfRange);
        CHECK(i32.consumed == 24);
        CHECK(i32.value == 42);
    }

    SECTION("Invalid inputs consume nothing")
    {
        CHECK(isInvalid<unsigned int>("-5"));
        CHECK(isInvalid<unsigned int>("-0"));
        CHECK(isInvalid<int>("-"));
        CHECK(isInvalid<int>("+"));
        CHECK(isInvalid<int>("- 5"));
        CHECK(isInvalid<int>("+-5"));
    }

    SECTION("A leading plus sign is accepted")
    {
        const auto parsed = parse<int>("+5");
        CHECK(parsed.ec == za::FromCharsError::None);
        CHECK(parsed.consumed == 2);
        CHECK(parsed.value == 5);
    }
}
