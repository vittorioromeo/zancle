#include "Tst/Tst.hpp"

#include "Zancle/String/FromCharsRadix.hpp"

#include "Zancle/String/ToCharsRadix.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Strlen.hpp"

#include <initializer_list>


////////////////////////////////////////////////////////////
namespace
{
template <typename T>
[[nodiscard]] za::FromCharsResult parse(const char* s, T& value, const za::Radix radix)
{
    const char* const last = s + ZA_STRLEN(s);
    return za::fromCharsRadix(s, last, value, radix);
}
} // namespace


////////////////////////////////////////////////////////////
TEST_CASE("[Base] FromCharsRadix.hpp - basic parses")
{
    SECTION("Hex (lowercase)")
    {
        za::U32    v = 0u;
        const auto r = parse("abcd", v, za::Radix::Hex);
        CHECK(v == 0xAB'CDu);
        CHECK(r.ec == za::FromCharsError::None);
        CHECK(*r.ptr == '\0'); // Consumed entire input.
    }

    SECTION("Hex (uppercase)")
    {
        za::U32    v = 0u;
        const auto r = parse("DEADBEEF", v, za::Radix::Hex);
        CHECK(v == 0xDE'AD'BE'EFu);
        CHECK(r.ec == za::FromCharsError::None);
    }

    SECTION("Hex (mixed case)")
    {
        za::U32    v = 0u;
        const auto r = parse("AbCd", v, za::Radix::Hex);
        CHECK(v == 0xAB'CDu);
        CHECK(r.ec == za::FromCharsError::None);
    }

    SECTION("Octal")
    {
        za::U32    v = 0u;
        const auto r = parse("777", v, za::Radix::Oct);
        CHECK(v == 511u);
        CHECK(r.ec == za::FromCharsError::None);
    }

    SECTION("Binary")
    {
        za::U32    v = 0u;
        const auto r = parse("11111111", v, za::Radix::Bin);
        CHECK(v == 0xFFu);
        CHECK(r.ec == za::FromCharsError::None);
    }

    SECTION("Zero")
    {
        za::U32    v = 1234u;
        const auto r = parse("0", v, za::Radix::Hex);
        CHECK(v == 0u);
        CHECK(r.ec == za::FromCharsError::None);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] FromCharsRadix.hpp - stops at first non-digit")
{
    SECTION("Hex stops at non-hex char")
    {
        const char s[] = "ffXY";
        za::U32    v   = 0u;
        const auto r   = za::fromCharsRadix(s, s + 4, v, za::Radix::Hex);
        CHECK(v == 0xFFu);
        CHECK(r.ec == za::FromCharsError::None);
        CHECK(r.ptr == s + 2); // Stopped at 'X'.
    }

    SECTION("Octal stops at out-of-radix digit '8'")
    {
        const char s[] = "78";
        za::U32    v   = 0u;
        const auto r   = za::fromCharsRadix(s, s + 2, v, za::Radix::Oct);
        CHECK(v == 7u);
        CHECK(r.ec == za::FromCharsError::None);
        CHECK(r.ptr == s + 1);
    }

    SECTION("Binary stops at out-of-radix digit '2'")
    {
        const char s[] = "1012";
        za::U32    v   = 0u;
        const auto r   = za::fromCharsRadix(s, s + 4, v, za::Radix::Bin);
        CHECK(v == 0b101u);
        CHECK(r.ec == za::FromCharsError::None);
        CHECK(r.ptr == s + 3);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] FromCharsRadix.hpp - failure modes")
{
    SECTION("Empty input is InvalidArgument")
    {
        za::U32    v = 0u;
        const auto r = za::fromCharsRadix(nullptr, nullptr, v, za::Radix::Hex);
        CHECK(r.ec == za::FromCharsError::InvalidArgument);
    }

    SECTION("No valid digits is InvalidArgument")
    {
        za::U32    v = 0u;
        const auto r = parse("zzz", v, za::Radix::Hex);
        CHECK(r.ec == za::FromCharsError::InvalidArgument);
    }

    SECTION("Leading sign is not accepted")
    {
        za::U32    v = 0u;
        const auto r = parse("-1", v, za::Radix::Hex);
        CHECK(r.ec == za::FromCharsError::InvalidArgument);
    }

    SECTION("Overflow on a too-large hex value")
    {
        za::U8     v = 0u;
        const auto r = parse("100", v, za::Radix::Hex); // 0x100 doesn't fit in U8
        CHECK(r.ec == za::FromCharsError::ResultOutOfRange);
    }

    SECTION("Maximum value fits exactly")
    {
        za::U32    v = 0u;
        const auto r = parse("ffffffff", v, za::Radix::Hex);
        CHECK(v == 0xFF'FF'FF'FFu);
        CHECK(r.ec == za::FromCharsError::None);
    }

    SECTION("Just-over-maximum is rejected")
    {
        za::U32    v = 0u;
        const auto r = parse("100000000", v, za::Radix::Hex);
        CHECK(r.ec == za::FromCharsError::ResultOutOfRange);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] FromCharsRadix.hpp - round-trip with toCharsRadix")
{
    // Sanity: parse what we format. Decoupled, but the two halves should agree.
    for (za::U32 v : {0u, 1u, 0xFu, 0x10u, 0xAB'CDu, 0xDE'AD'BE'EFu, 0xFF'FF'FF'FFu})
    {
        for (auto radix : {za::Radix::Bin, za::Radix::Oct, za::Radix::Hex})
        {
            char        buf[64]{};
            const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), v, radix);
            REQUIRE(end != nullptr);

            za::U32    parsed = 0u;
            const auto r      = za::fromCharsRadix(buf, end, parsed, radix);
            CHECK(r.ec == za::FromCharsError::None);
            CHECK(r.ptr == end);
            CHECK(parsed == v);
        }
    }
}


namespace
{
namespace FromCharsRadixShiftTest // for unity builds
{
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] za::FromCharsError parse(const char* text, T& value, const za::Radix radix)
{
    return za::fromCharsRadix(text, text + ZA_STRLEN(text), value, radix).ec;
}


////////////////////////////////////////////////////////////
template <typename T>
concept CanParseRadix = requires(const char* p, T& v) { za::fromCharsRadix(p, p, v, za::Radix::Bin); };

} // namespace FromCharsRadixShiftTest
} // namespace


TEST_CASE("[Base] fromCharsRadix overflow at every radix")
{
    using FromCharsRadixShiftTest::parse;

    za::U8 u8 = 0;
    CHECK(parse("11111111", u8, za::Radix::Bin) == za::FromCharsError::None);
    CHECK(u8 == 255);
    CHECK(parse("100000000", u8, za::Radix::Bin) == za::FromCharsError::ResultOutOfRange);
    CHECK(parse("377", u8, za::Radix::Oct) == za::FromCharsError::None);
    CHECK(u8 == 255);
    CHECK(parse("400", u8, za::Radix::Oct) == za::FromCharsError::ResultOutOfRange);
    CHECK(parse("FF", u8, za::Radix::Hex) == za::FromCharsError::None);
    CHECK(parse("100", u8, za::Radix::Hex) == za::FromCharsError::ResultOutOfRange);

    za::U64 u64 = 0;
    CHECK(parse("1777777777777777777777", u64, za::Radix::Oct) == za::FromCharsError::None);
    CHECK(u64 == ~za::U64{0});
    CHECK(parse("2000000000000000000000", u64, za::Radix::Oct) == za::FromCharsError::ResultOutOfRange);
    CHECK(parse("11111111111111111111111111111111111111111111111111111111111111111", u64, za::Radix::Bin) ==
          za::FromCharsError::ResultOutOfRange); // 65 digits
    CHECK(parse("ffffffffffffffff", u64, za::Radix::Hex) == za::FromCharsError::None);
    CHECK(parse("10000000000000000", u64, za::Radix::Hex) == za::FromCharsError::ResultOutOfRange);

    // Leading zeros never overflow
    CHECK(parse("00000000000000000000000000000000000000001", u8, za::Radix::Bin) == za::FromCharsError::None);
    CHECK(u8 == 1);

    // `bool` is not an integer to parse
    STATIC_CHECK(FromCharsRadixShiftTest::CanParseRadix<za::U32>);
    STATIC_CHECK(!FromCharsRadixShiftTest::CanParseRadix<bool>);
}


TEST_CASE("[Base] fromCharsRadix pointer semantics")
{
    SECTION("Overflow consumes the whole number and leaves the value untouched")
    {
        za::U8            u8   = 42;
        const char* const text = "1ffffffffffffffffz";
        const auto        r    = za::fromCharsRadix(text, text + ZA_STRLEN(text), u8, za::Radix::Hex);
        CHECK(r.ec == za::FromCharsError::ResultOutOfRange);
        CHECK(r.ptr == text + 17); // at 'z'
        CHECK(u8 == 42);

        const char* const bin  = "0101010101201";
        const auto        rBin = za::fromCharsRadix(bin, bin + ZA_STRLEN(bin), u8, za::Radix::Bin);
        CHECK(rBin.ec == za::FromCharsError::ResultOutOfRange);
        CHECK(rBin.ptr == bin + 10); // at '2', not a binary digit
        CHECK(u8 == 42);
    }

    SECTION("Invalid inputs consume nothing")
    {
        za::U32           v    = 42u;
        const char* const text = "-1";
        const auto        r    = za::fromCharsRadix(text, text + 2, v, za::Radix::Hex);
        CHECK(r.ec == za::FromCharsError::InvalidArgument);
        CHECK(r.ptr == text);
        CHECK(v == 42u);
    }
}
