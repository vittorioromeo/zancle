#include "StringifyStringViewUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/String/ToCharsRadix.hpp"

#include "Zancle/String/StringView.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"

#include <limits>


////////////////////////////////////////////////////////////
namespace
{
// Wrap a successful `[first, end)` result from `toCharsRadix` in a
// `StringView` for readable assertions. Precondition: `end != nullptr`
// (overflow is checked separately, not via this helper).
[[nodiscard]] za::StringView view(const char* buf, const char* end)
{
    return {buf, static_cast<za::SizeT>(end - buf)};
}
} // namespace


////////////////////////////////////////////////////////////
TEST_CASE("[Base] ToCharsRadix.hpp - basic unsigned values")
{
    char buf[64]{};

    SECTION("Hex lowercase")
    {
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), 0xAB'CDu, za::Radix::Hex);
        CHECK(view(buf, end) == "abcd");
    }

    SECTION("Hex uppercase")
    {
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), 0xAB'CDu, za::Radix::Hex, /* upperHex */ true);
        CHECK(view(buf, end) == "ABCD");
    }

    SECTION("Octal")
    {
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), 511u, za::Radix::Oct);
        CHECK(view(buf, end) == "777");
    }

    SECTION("Binary")
    {
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), 0xAu, za::Radix::Bin);
        CHECK(view(buf, end) == "1010");
    }

    SECTION("Zero always emits a single digit")
    {
        const auto* eHex = za::toCharsRadix(buf, buf + sizeof(buf), 0u, za::Radix::Hex);
        CHECK(view(buf, eHex) == "0");

        const auto* eOct = za::toCharsRadix(buf, buf + sizeof(buf), 0u, za::Radix::Oct);
        CHECK(view(buf, eOct) == "0");

        const auto* eBin = za::toCharsRadix(buf, buf + sizeof(buf), 0u, za::Radix::Bin);
        CHECK(view(buf, eBin) == "0");
    }

    SECTION("upperHex is a no-op for non-hex radices")
    {
        const auto* eOct = za::toCharsRadix(buf, buf + sizeof(buf), 9u, za::Radix::Oct, /* upperHex */ true);
        CHECK(view(buf, eOct) == "11");

        const auto* eBin = za::toCharsRadix(buf, buf + sizeof(buf), 9u, za::Radix::Bin, /* upperHex */ true);
        CHECK(view(buf, eBin) == "1001");
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] ToCharsRadix.hpp - signed values emit raw bit pattern (no sign)")
{
    char buf[64]{};

    SECTION("Negative int -> 32-bit two's-complement hex")
    {
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), -1, za::Radix::Hex);
        CHECK(view(buf, end) == "ffffffff");
    }

    SECTION("Negative int -> uppercase hex")
    {
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), -1, za::Radix::Hex, /* upperHex */ true);
        CHECK(view(buf, end) == "FFFFFFFF");
    }

    SECTION("Negative signed char -> 8-bit binary")
    {
        // After integer-promotion-aware unsigned cast, -1 (signed char) ->
        // 0xFF (unsigned char) -> "11111111" in binary.
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), static_cast<signed char>(-1), za::Radix::Bin);
        CHECK(view(buf, end) == "11111111");
    }

    SECTION("Negative long long -> 64-bit hex")
    {
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), static_cast<long long>(-1), za::Radix::Hex);
        CHECK(view(buf, end) == "ffffffffffffffff");
    }

    SECTION("LLONG_MIN as 64-bit hex")
    {
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), std::numeric_limits<long long>::min(), za::Radix::Hex);
        CHECK(view(buf, end) == "8000000000000000");
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] ToCharsRadix.hpp - integer width boundaries")
{
    char buf[64]{};

    SECTION("UINT32_MAX in hex = 8 digits")
    {
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), 0xFF'FF'FF'FFu, za::Radix::Hex);
        CHECK(view(buf, end) == "ffffffff");
    }

    SECTION("UINT64_MAX in hex = 16 digits")
    {
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), 0xFF'FF'FF'FF'FF'FF'FF'FFull, za::Radix::Hex);
        CHECK(view(buf, end) == "ffffffffffffffff");
    }

    SECTION("UINT64_MAX in binary = 64 digits")
    {
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), 0xFF'FF'FF'FF'FF'FF'FF'FFull, za::Radix::Bin);
        CHECK(end != nullptr);
        CHECK((end - buf) == 64);
        CHECK(view(buf, end).size() == 64u);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] ToCharsRadix.hpp - buffer overflow returns nullptr")
{
    char small[2]{};

    SECTION("Hex value needs 4 chars, only 2 available")
    {
        CHECK(za::toCharsRadix(small, small + sizeof(small), 0xAB'CDu, za::Radix::Hex) == nullptr);
    }

    SECTION("Binary value needs 8 chars, only 2 available")
    {
        CHECK(za::toCharsRadix(small, small + sizeof(small), 0xFFu, za::Radix::Bin) == nullptr);
    }

    SECTION("Empty buffer rejects even a single-digit zero")
    {
        CHECK(za::toCharsRadix(small, small, 0u, za::Radix::Hex) == nullptr);
    }

    SECTION("Exact-fit buffer succeeds")
    {
        char        exact[4]{};
        const auto* end = za::toCharsRadix(exact, exact + sizeof(exact), 0xAB'CDu, za::Radix::Hex);
        CHECK(end != nullptr);
        CHECK(view(exact, end) == "abcd");
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] ToCharsRadix.hpp - constexpr usability")
{
    // The function is marked `constexpr`; confirm it actually evaluates at
    // compile time when given a constant-evaluation context.
    constexpr auto checkHex = []
    {
        char        buf[8]{};
        const auto* end = za::toCharsRadix(buf, buf + sizeof(buf), 0x2Au, za::Radix::Hex);
        return end != nullptr && buf[0] == '2' && buf[1] == 'a' && (end - buf) == 2;
    };

    STATIC_CHECK(checkHex());
}


namespace
{
namespace ToCharsRadixShiftTest // for unity builds
{
////////////////////////////////////////////////////////////
// `__extension__`: 128-bit integers are a GCC/Clang extension (avoids `-Wpedantic`)
__extension__ using I128 = __int128;
__extension__ using U128 = unsigned __int128;


////////////////////////////////////////////////////////////
template <typename T>
concept CanFormatRadix = requires(char* p, T v) { za::toCharsRadix(p, p, v, za::Radix::Hex); };

} // namespace ToCharsRadixShiftTest
} // namespace


TEST_CASE("[Base] toCharsRadix widest values and buffer limits")
{
    char buffer[80];

    const auto format = [&](const auto value, const za::Radix radix)
    {
        const char* const end = za::toCharsRadix(buffer, buffer + 80, value, radix);
        return za::StringView{buffer, static_cast<za::SizeT>(end - buffer)};
    };

    constexpr auto u64Max = std::numeric_limits<za::U64>::max();

    CHECK(format(u64Max, za::Radix::Bin) == "1111111111111111111111111111111111111111111111111111111111111111");
    CHECK(format(u64Max, za::Radix::Oct) == "1777777777777777777777");
    CHECK(format(u64Max, za::Radix::Hex) == "ffffffffffffffff");
    CHECK(format(za::U64{1} << 63, za::Radix::Oct) == "1000000000000000000000");
    CHECK(format(za::U8{0}, za::Radix::Bin) == "0");
    CHECK(format(za::I8{-1}, za::Radix::Oct) == "377");
    CHECK(format(za::U32{0x12'34'AB'CDu}, za::Radix::Hex) == "1234abcd");

    SECTION("Exactly enough room, and one byte too few")
    {
        char exact[4] = {'x', 'x', 'x', 'x'};
        CHECK(za::toCharsRadix(exact, exact + 3, za::U16{0xA'BCu}, za::Radix::Hex, true) == exact + 3);
        CHECK(za::StringView{exact, 3u} == "ABC");
        CHECK(exact[3] == 'x'); // nothing written past `last`

        char tooSmall[2] = {'x', 'x'};
        CHECK(za::toCharsRadix(tooSmall, tooSmall + 2, za::U16{0xA'BCu}, za::Radix::Hex) == nullptr);
        CHECK(tooSmall[0] == 'x');
    }

    SECTION("bool and 128-bit integers are rejected at compile time")
    {
        STATIC_CHECK(ToCharsRadixShiftTest::CanFormatRadix<za::U64>);
        STATIC_CHECK(!ToCharsRadixShiftTest::CanFormatRadix<bool>);
        STATIC_CHECK(!ToCharsRadixShiftTest::CanFormatRadix<ToCharsRadixShiftTest::U128>);
    }
}
