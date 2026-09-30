#include "StringifyStringViewUtil.hpp" // IWYU pragma: keep
#include "StringifyZbStringUtil.hpp"   // IWYU pragma: keep
#include "Tst/Tst.hpp"

#include "Zancle/Fmt/Fmt.hpp"

#include "Zancle/Fmt/FmtArgDefaultAlign.hpp"
#include "Zancle/Fmt/FmtResult.hpp"
#include "Zancle/Fmt/FmtSink.hpp"
#include "Zancle/Fmt/FmtSpec.hpp"
#include "Zancle/Fmt/FmtToString.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/StringView.hpp"
#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Base/NonDeduced.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Strlen.hpp"

#include <limits>


namespace
{
namespace FmtTest // for unity builds
{
////////////////////////////////////////////////////////////
/// Helper: capture the result of `fmtIntoBuffer` as a `za::StringView`
/// over `buffer`, returning a zero-length view on failure.
////////////////////////////////////////////////////////////
template <typename... Args>
[[nodiscard]] za::StringView formatToView(char (&buffer)[512],
                                          typename za::NonDeduced<const za::FmtString<Args...>>::type fmt,
                                          const Args&... args)
{
    char* const end = za::fmtIntoBuffer(buffer, fmt, args...);
    if (end == nullptr)
        return {};
    return za::StringView{buffer, static_cast<za::SizeT>(end - buffer)};
}


////////////////////////////////////////////////////////////
template <auto>
struct FmtStringAccepted
{
};


////////////////////////////////////////////////////////////
template <typename... Args>
concept AcceptsEmptyPrecision = requires {
    typename FmtStringAccepted<[] consteval
    {
        (void)za::FmtString<Args...>{"{:.}"};
        return true;
    }()>;
};


////////////////////////////////////////////////////////////
template <typename... Args>
concept AcceptsEmptyPrecisionF = requires {
    typename FmtStringAccepted<[] consteval
    {
        (void)za::FmtString<Args...>{"{:.f}"};
        return true;
    }()>;
};


static_assert(!AcceptsEmptyPrecision<double>);
static_assert(!AcceptsEmptyPrecisionF<double>);

} // namespace FmtTest
} // namespace


using namespace FmtTest;


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - format() basics")
{
    SECTION("No args, plain text")
    {
        CHECK(za::fmtToString("hello") == za::String{"hello"});
        CHECK(za::fmtToString("") == za::String{""});
    }

    SECTION("Single integer arg")
    {
        CHECK(za::fmtToString("{}", 0) == za::String{"0"});
        CHECK(za::fmtToString("{}", 42) == za::String{"42"});
        CHECK(za::fmtToString("{}", -7) == za::String{"-7"});
        CHECK(za::fmtToString("[{}]", 100) == za::String{"[100]"});
    }

    SECTION("Single floating-point arg (default precision = 6)")
    {
        CHECK(za::fmtToString("{}", 3.5) == za::String{"3.500000"});
        CHECK(za::fmtToString("{}", -0.25) == za::String{"-0.250000"});
    }

    SECTION("Every arithmetic type")
    {
        CHECK(za::fmtToString("{}", false) == za::String{"false"});
        CHECK(za::fmtToString("{}", 'A') == za::String{"A"});
        CHECK(za::fmtToString("{}", static_cast<signed char>(-1)) == za::String{"-1"});
        CHECK(za::fmtToString("{}", static_cast<unsigned char>(1)) == za::String{"1"});
        CHECK(za::fmtToString("{}", static_cast<short>(-1)) == za::String{"-1"});
        CHECK(za::fmtToString("{}", static_cast<unsigned short>(1)) == za::String{"1"});
        CHECK(za::fmtToString("{}", -1) == za::String{"-1"});
        CHECK(za::fmtToString("{}", 1u) == za::String{"1"});
        CHECK(za::fmtToString("{}", -1l) == za::String{"-1"});
        CHECK(za::fmtToString("{}", 1ul) == za::String{"1"});
        CHECK(za::fmtToString("{}", -1ll) == za::String{"-1"});
        CHECK(za::fmtToString("{}", 1ull) == za::String{"1"});
        CHECK(za::fmtToString("{}", 1.f) == za::String{"1.000000"});
        CHECK(za::fmtToString("{}", 1.) == za::String{"1.000000"});
        CHECK(za::fmtToString("{}", 1.L) == za::String{"1.000000"});
    }

    SECTION("Single string-like arg")
    {
        CHECK(za::fmtToString("{}", "world") == za::String{"world"});
        CHECK(za::fmtToString("{}", za::StringView{"hello"}) == za::String{"hello"});
        CHECK(za::fmtToString("{}", za::String{"abc"}) == za::String{"abc"});
    }

    SECTION("Multiple mixed args")
    {
        CHECK(za::fmtToString("{} = {}", "x", 42) == za::String{"x = 42"});
        CHECK(za::fmtToString("{}, {}, {}", 1, 2, 3) == za::String{"1, 2, 3"});
        CHECK(za::fmtToString("{} + {} = {}", 2, 3, 5) == za::String{"2 + 3 = 5"});
    }

    SECTION("Brace escapes")
    {
        CHECK(za::fmtToString("{{}}") == za::String{"{}"});
        CHECK(za::fmtToString("{{{}}}", 7) == za::String{"{7}"});
        CHECK(za::fmtToString("a{{b}}c") == za::String{"a{b}c"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - precision spec {:.Nf}")
{
    SECTION("Float precision")
    {
        CHECK(za::fmtToString("{:.0f}", 3.7) == za::String{"4"});
        CHECK(za::fmtToString("{:.1f}", 3.14) == za::String{"3.1"});
        CHECK(za::fmtToString("{:.2f}", 3.14) == za::String{"3.14"});
        CHECK(za::fmtToString("{:.3f}", 3.14159) == za::String{"3.142"});
        CHECK(za::fmtToString("{:.6f}", 1.0) == za::String{"1.000000"});
    }

    SECTION("Negative floats")
    {
        CHECK(za::fmtToString("{:.1f}", -2.5) == za::String{"-2.5"});
        CHECK(za::fmtToString("{:.3f}", -0.001) == za::String{"-0.001"});
    }

    SECTION("Float precision without `f` tag works the same")
    {
        CHECK(za::fmtToString("{:.2}", 3.14159) == za::String{"3.14"});
    }

    // Precision is only valid for floating-point arguments. Integer/string
    // formatters assert on precision in debug builds.
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - width spec {:N}")
{
    SECTION("Numbers default to right-align")
    {
        CHECK(za::fmtToString("[{:5}]", 42) == za::String{"[   42]"});
        CHECK(za::fmtToString("[{:3}]", 1) == za::String{"[  1]"});
    }

    SECTION("Strings default to left-align")
    {
        CHECK(za::fmtToString("[{:5}]", "hi") == za::String{"[hi   ]"});
        CHECK(za::fmtToString("[{:6}]", za::StringView{"abc"}) == za::String{"[abc   ]"});
    }

    SECTION("Content equal to width -- no padding")
    {
        CHECK(za::fmtToString("[{:3}]", 123) == za::String{"[123]"});
        CHECK(za::fmtToString("[{:3}]", "abc") == za::String{"[abc]"});
    }

    SECTION("Content wider than width -- no truncation")
    {
        CHECK(za::fmtToString("[{:3}]", 12'345) == za::String{"[12345]"});
        CHECK(za::fmtToString("[{:2}]", "long string") == za::String{"[long string]"});
    }

    SECTION("Width of zero (degenerate but valid)")
    {
        CHECK(za::fmtToString("[{:0}]", 42) == za::String{"[42]"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - explicit alignment {:<N}, {:>N}, {:^N}")
{
    SECTION("Right-align '>'")
    {
        CHECK(za::fmtToString("[{:>5}]", "hi") == za::String{"[   hi]"});
        CHECK(za::fmtToString("[{:>5}]", 42) == za::String{"[   42]"});
    }

    SECTION("Left-align '<'")
    {
        CHECK(za::fmtToString("[{:<5}]", "hi") == za::String{"[hi   ]"});
        CHECK(za::fmtToString("[{:<5}]", 42) == za::String{"[42   ]"});
    }

    SECTION("Center align '^'")
    {
        // Even-width slack splits evenly (extra goes right).
        CHECK(za::fmtToString("[{:^6}]", "hi") == za::String{"[  hi  ]"});
        // Odd-width slack: extra goes to right side.
        CHECK(za::fmtToString("[{:^5}]", "hi") == za::String{"[ hi  ]"});
        CHECK(za::fmtToString("[{:^5}]", 1) == za::String{"[  1  ]"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - custom fill character")
{
    SECTION("Zero-pad on the left")
    {
        CHECK(za::fmtToString("[{:0>5}]", 42) == za::String{"[00042]"});
        CHECK(za::fmtToString("[{:0>5}]", 7) == za::String{"[00007]"});
    }

    SECTION("Dot-fill on the left")
    {
        CHECK(za::fmtToString("[{:.>5}]", "ab") == za::String{"[...ab]"});
    }

    SECTION("Dash-center-fill")
    {
        CHECK(za::fmtToString("[{:-^7}]", "hi") == za::String{"[--hi---]"});
    }

    SECTION("Custom fill on left-align")
    {
        CHECK(za::fmtToString("[{:*<5}]", "ab") == za::String{"[ab***]"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - combined specs (width + precision)")
{
    SECTION("Width + precision, right-aligned")
    {
        CHECK(za::fmtToString("[{:>10.3f}]", 3.14159) == za::String{"[     3.142]"});
    }

    SECTION("Width + precision, zero-padded")
    {
        CHECK(za::fmtToString("[{:0>10.2f}]", 3.14) == za::String{"[0000003.14]"});
    }

    SECTION("Width + precision, left-aligned")
    {
        CHECK(za::fmtToString("[{:<10.2f}]", 3.14) == za::String{"[3.14      ]"});
    }

    SECTION("Width + precision, center-aligned")
    {
        CHECK(za::fmtToString("[{:^10.2f}]", 3.14) == za::String{"[   3.14   ]"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - integer radix specs {:x}, {:X}, {:o}, {:b}, {:d}")
{
    SECTION("Lowercase hex")
    {
        CHECK(za::fmtToString("{:x}", 0xAB'CDu) == za::String{"abcd"});
        CHECK(za::fmtToString("{:x}", 0u) == za::String{"0"});
        CHECK(za::fmtToString("{:x}", 0xDE'AD'BE'EFu) == za::String{"deadbeef"});
    }

    SECTION("Uppercase hex")
    {
        CHECK(za::fmtToString("{:X}", 0xAB'CDu) == za::String{"ABCD"});
        CHECK(za::fmtToString("{:X}", 0xDE'AD'BE'EFu) == za::String{"DEADBEEF"});
    }

    SECTION("Octal")
    {
        CHECK(za::fmtToString("{:o}", 8u) == za::String{"10"});
        CHECK(za::fmtToString("{:o}", 0u) == za::String{"0"});
        CHECK(za::fmtToString("{:o}", 511u) == za::String{"777"});
    }

    SECTION("Binary")
    {
        CHECK(za::fmtToString("{:b}", 0u) == za::String{"0"});
        CHECK(za::fmtToString("{:b}", 5u) == za::String{"101"});
        CHECK(za::fmtToString("{:b}", 0xFFu) == za::String{"11111111"});
    }

    SECTION("Explicit decimal tag matches default")
    {
        CHECK(za::fmtToString("{:d}", 42) == za::String{"42"});
        CHECK(za::fmtToString("{:d}", -42) == za::String{"-42"});
    }

    SECTION("Negative signed: hex/oct/bin emit the bit pattern, no sign")
    {
        // 32-bit `int`: two's-complement of -1 is 0xFFFFFFFF.
        CHECK(za::fmtToString("{:x}", -1) == za::String{"ffffffff"});
        CHECK(za::fmtToString("{:X}", -1) == za::String{"FFFFFFFF"});
        CHECK(za::fmtToString("{:b}", static_cast<signed char>(-1)) == za::String{"11111111"});
    }

    SECTION("Width + fill: zero-padded hex scancode (Keyboard.cpp use case)")
    {
        CHECK(za::fmtToString("{:0>4x}", 0x1Bu) == za::String{"001b"});
        CHECK(za::fmtToString("{:0>4X}", 0x1Bu) == za::String{"001B"});
        CHECK(za::fmtToString("{:0>8b}", 0xAu) == za::String{"00001010"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - mixed-spec multi-arg")
{
    const auto result = za::fmtToString("[{:>5}|{:<5}|{:^5}]", 1, 2, 3);
    CHECK(result == za::String{"[    1|2    |  3  ]"});

    const auto hud = za::fmtToString("FPS: {:>5.1f} | Frame: {:>4}ms | Cells: {:0>4}", 59.95, 16, 42);
    CHECK(hud == za::String{"FPS:  60.0 | Frame:   16ms | Cells: 0042"});
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - integer type variety")
{
    SECTION("signed/unsigned")
    {
        CHECK(za::fmtToString("{}", -32'000) == za::String{"-32000"});
        CHECK(za::fmtToString("{}", static_cast<unsigned>(4'000'000'000u)) == za::String{"4000000000"});

        // Narrow signed types must survive `toChars` integer-promotion path
        CHECK(za::fmtToString("{}", static_cast<short>(-32'000)) == za::String{"-32000"});
        CHECK(za::fmtToString("{}", static_cast<signed char>(-100)) == za::String{"-100"});
    }

    SECTION("long long")
    {
        CHECK(za::fmtToString("{}", static_cast<long long>(-1'234'567'890'123LL)) ==
              za::String{"-123456789012"
                         "3"});
        CHECK(za::fmtToString("{}", static_cast<unsigned long long>(18'000'000'000'000ULL)) ==
              za::String{"180000"
                         "000000"
                         "00"});
    }

    SECTION("char formats as a glyph by default (matches libfmt)")
    {
        CHECK(za::fmtToString("{}", 'A') == za::String{"A"});
        CHECK(za::fmtToString("{}", '\n') == za::String{"\n"});
        CHECK(za::fmtToString("[{}]", '"') == za::String{"[\"]"});
        CHECK(za::fmtToString("{}{}{}", 'a', 'b', 'c') == za::String{"abc"});
    }

    SECTION("`:c` on char also emits the glyph")
    {
        CHECK(za::fmtToString("{:c}", 'A') == za::String{"A"});
        CHECK(za::fmtToString("{:c}", '\n') == za::String{"\n"});
    }

    SECTION("`:d` on char emits the numeric code")
    {
        CHECK(za::fmtToString("{:d}", 'A') == za::String{"65"});
        CHECK(za::fmtToString("{:d}", '0') == za::String{"48"});
    }

    SECTION("`:x` / `:X` / `:o` / `:b` on char emit the numeric code in that radix")
    {
        CHECK(za::fmtToString("{:x}", 'A') == za::String{"41"});
        CHECK(za::fmtToString("{:X}", 'A') == za::String{"41"});
        CHECK(za::fmtToString("{:o}", 'A') == za::String{"101"});
        CHECK(za::fmtToString("{:b}", 'A') == za::String{"1000001"});
    }

    SECTION("`:c` on an integer emits the lowest byte as a glyph")
    {
        CHECK(za::fmtToString("{:c}", 65) == za::String{"A"});
        CHECK(za::fmtToString("{:c}", 0x41u) == za::String{"A"});
    }

    SECTION("char default alignment is left, like a string")
    {
        CHECK(za::fmtToString("[{:3}]", 'A') == za::String{"[A  ]"});
        CHECK(za::fmtToString("[{:>3}]", 'A') == za::String{"[  A]"});
        CHECK(za::fmtToString("[{:^3}]", 'A') == za::String{"[ A ]"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - bool formats as 'true'/'false'")
{
    SECTION("Default presentation matches std::format / fmt::format")
    {
        CHECK(za::fmtToString("{}", true) == za::String{"true"});
        CHECK(za::fmtToString("{}", false) == za::String{"false"});
    }

    SECTION("Multiple bools in one format string")
    {
        CHECK(za::fmtToString("{}/{}", true, false) == za::String{"true/false"});
        CHECK(za::fmtToString("a={} b={}", false, true) == za::String{"a=false b=true"});
    }

    SECTION("Default alignment is left (matches string-like types)")
    {
        CHECK(za::fmtToString("[{:8}]", true) == za::String{"[true    ]"});
        CHECK(za::fmtToString("[{:8}]", false) == za::String{"[false   ]"});
    }

    SECTION("Explicit alignment overrides the default")
    {
        CHECK(za::fmtToString("[{:>8}]", true) == za::String{"[    true]"});
        CHECK(za::fmtToString("[{:>8}]", false) == za::String{"[   false]"});
        CHECK(za::fmtToString("[{:^8}]", true) == za::String{"[  true  ]"});
    }

    SECTION("Custom fill character")
    {
        CHECK(za::fmtToString("[{:.>8}]", true) == za::String{"[....true]"});
        CHECK(za::fmtToString("[{:*<8}]", false) == za::String{"[false***]"});
    }

    SECTION("Width equal to or smaller than content -- no padding")
    {
        CHECK(za::fmtToString("[{:4}]", true) == za::String{"[true]"});
        CHECK(za::fmtToString("[{:3}]", false) == za::String{"[false]"}); // wider than width
    }

    SECTION("Numeric form available via explicit int cast")
    {
        CHECK(za::fmtToString("{}/{}", static_cast<int>(true), static_cast<int>(false)) ==
              za::String{"1/"
                         "0"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - fmtIntoBuffer raw API")
{
    char buf[512];

    SECTION("Returns end pointer on success (no implicit null terminator)")
    {
        const auto* end = za::fmtIntoBuffer(buf, "x={}", 42);
        REQUIRE(end != nullptr);
        CHECK(za::StringView{buf, static_cast<za::SizeT>(end - buf)} == za::StringView{"x=42"});
    }

    SECTION("Returns nullptr on buffer overflow")
    {
        char small[4];
        // "hello" is 5 bytes, doesn't fit in 4.
        CHECK(za::fmtIntoBuffer(small, "hello") == nullptr);
    }

    SECTION("Caller may write their own null terminator")
    {
        // Reserve an extra byte for '\0' and write it manually.
        char        cstr[16];
        auto* const end = za::fmtIntoBuffer(cstr, sizeof(cstr) - 1u, "n={}", 7);
        REQUIRE(end != nullptr);
        *end = '\0';
        CHECK(za::StringView{cstr} == za::StringView{"n=7"});
    }

    SECTION("Returns nullptr when arg formatting overflows")
    {
        char small[4];
        CHECK(za::fmtIntoBuffer(small, "{}", 1'234'567) == nullptr);
    }

    SECTION("Helper produces same result as format()")
    {
        const auto view = formatToView(buf, "Hello, {}!", "world");
        CHECK(view == za::StringView{"Hello, world!"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - fmtTo() into za::String")
{
    SECTION("Appends to non-empty sink")
    {
        za::String s = "prefix:";
        (void)za::fmtTo(s, " {} = {:.2f}", "pi", 3.14159);
        CHECK(s == za::String{"prefix: pi = 3.14"});
    }

    SECTION("Format into fresh sink")
    {
        za::String s;
        (void)za::fmtTo(s, "{:>8}", "abc");
        CHECK(s == za::String{"     abc"});
    }

    SECTION("Repeated fmtTo accumulates")
    {
        za::String s;
        (void)za::fmtTo(s, "{}-", 1);
        (void)za::fmtTo(s, "{}-", 2);
        (void)za::fmtTo(s, "{}", 3);
        CHECK(s == za::String{"1-2-3"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - fmtTo() into Utf8String")
{
    SECTION("Build a HUD-style string")
    {
        za::Utf8String text;
        CHECK(za::fmtTo(text, "Frame: {:>5}ms / Delta: {:.2f}", 16, 0.016667) == za::FmtResult::Ok);

        const auto bytes = text.asBytes();
        CHECK(za::StringView{bytes.data(), bytes.size()} == za::StringView{"Frame:    16ms / Delta: 0.02"});
    }

    SECTION("Appends to a Utf8String already containing UTF-8 content")
    {
        za::Utf8String text{u8"café "};
        CHECK(za::fmtTo(text, "= {}", 42) == za::FmtResult::Ok);

        const auto bytes = text.asBytes();
        CHECK(za::StringView{bytes.data(), bytes.size()} == za::StringView{"café = 42"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - edge cases")
{
    SECTION("Empty format string")
    {
        CHECK(za::fmtToString("") == za::String{""});
    }

    SECTION("Only escapes")
    {
        CHECK(za::fmtToString("{{") == za::String{"{"});
        CHECK(za::fmtToString("}}") == za::String{"}"});
        CHECK(za::fmtToString("{{{{}}}}") == za::String{"{{}}"});
    }

    SECTION("Empty string arg with width")
    {
        CHECK(za::fmtToString("[{:4}]", "") == za::String{"[    ]"});
        CHECK(za::fmtToString("[{:>4}]", "") == za::String{"[    ]"});
    }

    SECTION("Adjacent placeholders")
    {
        CHECK(za::fmtToString("{}{}{}", 1, 2, 3) == za::String{"123"});
    }
}


////////////////////////////////////////////////////////////
// Custom types: ADL `fmtArg` overloads exercising every supported pattern.
//
// Wrapped in an anonymous namespace for internal linkage; the named
// inner `customtypes` namespace is what ADL keys on for these types.
////////////////////////////////////////////////////////////
namespace
{
namespace customtypes
{
////////////////////////////////////////////////////////////
// Pattern A: zero-scratch, just a byte copy.
////////////////////////////////////////////////////////////
struct FixedTag
{
    const char* text;
};

[[nodiscard]] inline za::FmtResult fmtArg(za::FmtSink& sink, const FixedTag& t, const za::FmtSpec&) noexcept
{
    return sink.append(t.text, ZA_STRLEN(t.text));
}


////////////////////////////////////////////////////////////
// Pattern B: small simple composite -- recurse via `sink.format`.
////////////////////////////////////////////////////////////
struct Vec2f
{
    float x;
    float y;
};

[[nodiscard]] inline za::FmtResult fmtArg(za::FmtSink& sink, const Vec2f& v, const za::FmtSpec&)
{
    return sink.fmt("({}, {})", v.x, v.y);
}


////////////////////////////////////////////////////////////
// Pattern C: composite container -- recursion over elements.
//
// Note: this composes over any T, so `MiniVec<Vec2f>` and even
// `MiniVec<MiniVec<Vec2f>>` Just Work.
////////////////////////////////////////////////////////////
template <typename T>
struct MiniVec
{
    const T*  data;
    za::SizeT size;
};

template <typename T>
[[nodiscard]] inline za::FmtResult fmtArg(za::FmtSink& sink, const MiniVec<T>& v, const za::FmtSpec&)
{
    ZA_FMT_TRY(sink.appendChar('['));
    for (za::SizeT i = 0u; i < v.size; ++i)
    {
        if (i != 0u)
            ZA_FMT_TRY(sink.append(", ", 2u));
        ZA_FMT_TRY(sink.fmt("{}", v.data[i]));
    }
    return sink.appendChar(']');
}


////////////////////////////////////////////////////////////
// Pattern D: defaults to right-align (specialized variable template).
////////////////////////////////////////////////////////////
struct RightHex
{
    unsigned int value;
};

[[nodiscard]] inline za::FmtResult fmtArg(za::FmtSink& sink, const RightHex& h, const za::FmtSpec&)
{
    char         buf[12]{};
    za::SizeT    n = 0u;
    unsigned int v = h.value;
    char         tmp[10];

    if (v == 0u)
    {
        tmp[n++] = '0';
    }
    else
    {
        while (v != 0u && n < sizeof(tmp))
        {
            const unsigned int digit = v & 0xFu;
            tmp[n++]                 = static_cast<char>(digit < 10u ? '0' + digit : 'a' + digit - 10u);
            v >>= 4;
        }
    }

    // Reverse into `buf`.
    for (za::SizeT i = 0u; i < n; ++i)
        buf[i] = tmp[n - 1u - i];

    return sink.append(buf, n);
}


////////////////////////////////////////////////////////////
// Pattern E: formatter-level failure, distinct from destination overflow.
////////////////////////////////////////////////////////////
struct FailTag
{
};

[[nodiscard]] inline za::FmtResult fmtArg(za::FmtSink&, const FailTag&, const za::FmtSpec&) noexcept
{
    return za::FmtResult::Failed;
}


////////////////////////////////////////////////////////////
struct RetryTag
{
    int* attemptCount;
};

[[nodiscard]] inline za::FmtResult fmtArg(za::FmtSink& sink, const RetryTag& t, const za::FmtSpec&) noexcept
{
    ++*t.attemptCount;

    for (int i = 0; i < 600; ++i)
        ZA_FMT_TRY(sink.appendChar('x'));

    return za::FmtResult::Ok;
}
} // namespace customtypes
} // anonymous namespace

// Default to right-align for `RightHex` (it's a "numeric-looking" type).
// Specialization lives in `namespace za` to match the primary template;
// the type itself has internal linkage via the anonymous namespace above.
namespace za
{
template <>
inline constexpr char fmtArgDefaultAlign<customtypes::RightHex> = '>';
} // namespace za


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - custom fmtArg (Pattern A: zero-scratch byte copy)")
{
    using customtypes::FixedTag;

    SECTION("Default formatting")
    {
        CHECK(za::fmtToString("{}", FixedTag{"hello"}) == za::String{"hello"});
        CHECK(za::fmtToString("[{}]", FixedTag{"world"}) == za::String{"[world]"});
    }

    SECTION("Padding (default left-align)")
    {
        CHECK(za::fmtToString("[{:7}]", FixedTag{"hi"}) == za::String{"[hi     ]"});
        CHECK(za::fmtToString("[{:>7}]", FixedTag{"hi"}) == za::String{"[     hi]"});
        CHECK(za::fmtToString("[{:^7}]", FixedTag{"hi"}) == za::String{"[  hi   ]"});
    }

    SECTION("Custom fill")
    {
        CHECK(za::fmtToString("[{:*>6}]", FixedTag{"xy"}) == za::String{"[****xy]"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - custom fmtArg (Pattern B: recursive composition)")
{
    using customtypes::Vec2f;

    SECTION("Single Vec2f")
    {
        CHECK(za::fmtToString("pos = {}", Vec2f{1.5f, 2.0f}) == za::String{"pos = (1.500000, 2.000000)"});
    }

    SECTION("Multiple Vec2f in one format")
    {
        CHECK(za::fmtToString("a={} b={}", Vec2f{1.f, 2.f}, Vec2f{3.f, 4.f}) ==
              za::String{"a=(1.000000, "
                         "2.000000) "
                         "b=(3.000000, "
                         "4.000000)"});
    }

    SECTION("Outer width pads the whole composed result")
    {
        // Rendered inner: "(0.000000, 0.000000)" -- exactly 20 chars; width 24 right-pads by 4.
        CHECK(za::fmtToString("[{:>24}]", Vec2f{0.0f, 0.0f}) == za::String{"[    (0.000000, 0.000000)]"});
    }

    SECTION("Outer precision is NOT propagated to nested calls (each inner '{}' uses its own spec)")
    {
        // The outer spec applies to the outer placeholder's content (the whole rendered Vec2f),
        // not to the inner float placeholders. So inner floats use default precision 6.
        const auto result = za::fmtToString("{:.2}", Vec2f{1.5f, 2.5f});
        CHECK(result == za::String{"(1.500000, 2.500000)"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - custom fmtArg (Pattern C: nested containers)")
{
    using customtypes::MiniVec;
    using customtypes::Vec2f;

    SECTION("Container of integers")
    {
        const int          elems[] = {1, 2, 3, 4};
        const MiniVec<int> v{elems, 4u};
        CHECK(za::fmtToString("{}", v) == za::String{"[1, 2, 3, 4]"});
    }

    SECTION("Empty container")
    {
        const MiniVec<int> v{nullptr, 0u};
        CHECK(za::fmtToString("{}", v) == za::String{"[]"});
    }

    SECTION("Container of Vec2f -- two levels of recursion")
    {
        const Vec2f          points[] = {{1.f, 2.f}, {3.f, 4.f}};
        const MiniVec<Vec2f> v{points, 2u};
        CHECK(za::fmtToString("{}", v) == za::String{"[(1.000000, 2.000000), (3.000000, 4.000000)]"});
    }

    SECTION("Container of container -- three levels of recursion")
    {
        const int                   row0[] = {1, 2, 3};
        const int                   row1[] = {4, 5};
        const MiniVec<int>          rows[] = {{row0, 3u}, {row1, 2u}};
        const MiniVec<MiniVec<int>> matrix{rows, 2u};

        CHECK(za::fmtToString("{}", matrix) == za::String{"[[1, 2, 3], [4, 5]]"});
    }

    SECTION("Container of containers of Vec2f -- mixed recursion")
    {
        const customtypes::Vec2f                   a[]    = {{1.f, 1.f}};
        const customtypes::Vec2f                   b[]    = {{2.f, 2.f}, {3.f, 3.f}};
        const MiniVec<customtypes::Vec2f>          rows[] = {{a, 1u}, {b, 2u}};
        const MiniVec<MiniVec<customtypes::Vec2f>> grid{rows, 2u};

        CHECK(za::fmtToString("{}", grid) == za::String{"[[(1.000000, 1.000000)], [(2.000000, 2.000000), "
                                                        "(3.000000, 3.000000)]]"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - custom fmtArgDefaultAlign specialization")
{
    using customtypes::RightHex;

    SECTION("Default alignment is now '>' (specialized)")
    {
        CHECK(za::fmtToString("[{:6}]", RightHex{0xabu}) == za::String{"[    ab]"});
    }

    SECTION("Explicit '<' overrides the specialized default")
    {
        CHECK(za::fmtToString("[{:<6}]", RightHex{0xabu}) == za::String{"[ab    ]"});
    }

    SECTION("Zero value")
    {
        CHECK(za::fmtToString("[{:>5}]", RightHex{0u}) == za::String{"[    0]"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - FmtSink direct use (low-level API)")
{
    SECTION("Plain bytes + format mix")
    {
        char        buf[64];
        za::FmtSink sink{buf, sizeof(buf)};

        CHECK(sink.append("[", 1u) == za::FmtResult::Ok);
        CHECK(sink.fmt("{}", 42) == za::FmtResult::Ok);
        CHECK(sink.append("] ", 2u) == za::FmtResult::Ok);
        CHECK(sink.appendChar('=') == za::FmtResult::Ok);
        CHECK(sink.fmt(" {:.1f}", 3.14) == za::FmtResult::Ok);

        CHECK(za::StringView{buf, sink.size()} == za::StringView{"[42] = 3.1"});
    }

    SECTION("Overflow is returned eagerly and does not change the sink")
    {
        char        buf[5];
        za::FmtSink sink{buf, sizeof(buf)};

        CHECK(sink.append("hello", 5u) == za::FmtResult::Ok); // exactly fills buffer

        CHECK(sink.append("!", 1u) == za::FmtResult::Overflow);
        CHECK(sink.size() == 5u);

        char        larger[16];
        za::FmtSink fresh{larger, sizeof(larger)};
        CHECK(fresh.append(" world", 6u) == za::FmtResult::Ok);
        CHECK(fresh.size() == 6u);
    }

    SECTION("size() as checkpoint + atOffset survives interleaved writes")
    {
        char        buf[64];
        za::FmtSink sink{buf, sizeof(buf)};

        CHECK(sink.append("aa", 2u) == za::FmtResult::Ok);
        const za::SizeT m = sink.size();
        CHECK(sink.append("bb", 2u) == za::FmtResult::Ok);

        CHECK(m == 2u);
        CHECK(sink.atOffset(m) == buf + 2);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - formatter failure is not treated as overflow")
{
    using customtypes::FailTag;

    SECTION("fmtIntoBuffer returns nullptr")
    {
        char buf[64];
        CHECK(za::fmtIntoBuffer(buf, "{}", FailTag{}) == nullptr);
    }

    SECTION("fmtTo returns failed without appending partial output")
    {
        za::String out = "prefix";
        (void)za::fmtTo(out, "{}{}", "ok", FailTag{});
        CHECK(out == za::String{"prefix"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - invalid built-in specs fail in release")
{
#ifndef ZA_DEBUG
    za::String  out = "prefix";
    char        buffer[16];
    za::FmtSink sink{buffer, sizeof(buffer)};

    CHECK(za::fmtTo(out, "{:f}", 42) == za::FmtResult::Failed);
    CHECK(za::fmtArg(sink, 42, za::FmtSpec{.type = 'q'}) == za::FmtResult::Failed);
    CHECK(za::fmtTo(out, "{:x}", 1.5) == za::FmtResult::Failed);
    CHECK(za::fmtTo(out, "{:.2}", "abc") == za::FmtResult::Failed);
    CHECK(za::fmtTo(out, "{:.2}", za::StringView{"abc"}) == za::FmtResult::Failed);
    CHECK(za::fmtTo(out, "{:.2}", 'a') == za::FmtResult::Failed);
    CHECK(za::fmtTo(out, "{:x}", true) == za::FmtResult::Failed);
    CHECK(out == za::String{"prefix"});
#endif
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - fmtTo() large output (heap fallback path)")
{
    SECTION("Output larger than 512-byte staging buffer succeeds")
    {
        // Build a format-args list that produces > 512 bytes of output.
        za::String big;
        for (int i = 0; i < 100; ++i)
            big += za::String{"0123456789"}; // 1000 chars

        za::String out;
        (void)za::fmtTo(out, "[{}]", za::StringView{big.data(), big.size()});

        REQUIRE(out.size() == big.size() + 2u);
        CHECK(out.data()[0] == '[');
        CHECK(out.data()[out.size() - 1u] == ']');
        CHECK(za::StringView{out.data() + 1, big.size()} == za::StringView{big.data(), big.size()});
    }

    SECTION("Output near staging boundary")
    {
        // Exactly fits the 512-byte staging buffer.
        za::String mid;
        for (int i = 0; i < 50; ++i)
            mid += za::String{"0123456789"}; // 500 chars

        za::String out;
        (void)za::fmtTo(out, "{}", za::StringView{mid.data(), mid.size()});

        CHECK(out.size() == mid.size());
    }

    SECTION("Custom formatter is retried after overflow")
    {
        int        attemptCount = 0;
        za::String out;

        CHECK(za::fmtTo(out, "{}", customtypes::RetryTag{&attemptCount}) == za::FmtResult::Ok);
        CHECK(attemptCount == 2);
        CHECK(out.size() == 600u);
    }

    SECTION("Destination sink overflow is propagated")
    {
        char        buffer[1];
        za::FmtSink sink{buffer, sizeof(buffer)};

        CHECK(za::fmtTo(sink, "too large") == za::FmtResult::Overflow);
        CHECK(za::fmtTo(sink, "{:>600}", "") == za::FmtResult::Overflow);
        CHECK(sink.size() == 0u);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - boundary conditions")
{
    SECTION("Exact fit -- buffer size equals output (no terminator reserved)")
    {
        char        buf[5];
        auto* const end = za::fmtIntoBuffer(buf, "hello");
        REQUIRE(end != nullptr);
        CHECK(za::StringView{buf, static_cast<za::SizeT>(end - buf)} == za::StringView{"hello"});
    }

    SECTION("Off-by-one -- one byte short")
    {
        char        buf[4];
        auto* const end = za::fmtIntoBuffer(buf, "hello");
        CHECK(end == nullptr);
    }

    SECTION("Very wide width is honored")
    {
        za::String out;
        (void)za::fmtTo(out, "[{:>1000}]", "x");
        CHECK(out.size() == 1002u);
        CHECK(out.data()[0] == '[');
        CHECK(out.data()[out.size() - 2u] == 'x');
        CHECK(out.data()[out.size() - 1u] == ']');

        // First 1000 chars after '[' should be spaces, last is 'x'.
        for (za::SizeT i = 1u; i < 1000u; ++i)
            CHECK(out.data()[i] == ' ');
    }

    SECTION("Padding to width 0 is a no-op")
    {
        CHECK(za::fmtToString("[{:0}]", "abc") == za::String{"[abc]"});
    }

    SECTION("Float at boundary of supported precision (10)")
    {
        CHECK(za::fmtToString("{:.10f}", 0.5) == za::String{"0.5000000000"});
    }

    SECTION("Large finite double inside the toChars range")
    {
        // The float backend (`toChars`) uses an `int64` scaled
        // representation: `|value| * 10^precision` must fit in
        // `int64` (~9.2e18). 9e12 * 10^6 (default precision) = 9e18,
        // safely below.
        za::String out;

        CHECK(za::fmtTo(out, "{}", 9'000'000'000'000.0) == za::FmtResult::Ok);
        CHECK(out == za::String{"9000000000000.000000"});
    }

    SECTION("Large double recovered by reducing precision")
    {
        // 10^13 * 10^3 = 10^16 < ~9.2e18, so a smaller precision
        // formats values that overflow at the default precision.
        za::String out;

        CHECK(za::fmtTo(out, "{:.3f}", 10'000'000'000'000.0) == za::FmtResult::Ok);
        CHECK(out == za::String{"10000000000000.000"});
    }

    SECTION("Doubles of any magnitude are formatted")
    {
        // Used to fail beyond ~9.2e18 once scaled by `10^precision` (e.g. 10^13 at the default precision)
        za::String out = "prefix:";

        CHECK(za::fmtTo(out, "{}", 10'000'000'000'000.0) == za::FmtResult::Ok);
        CHECK(out == za::String{"prefix:10000000000000.000000"});

        za::String large;
        CHECK(za::fmtTo(large, "{:.2f}", -std::numeric_limits<double>::max()) == za::FmtResult::Ok);
        CHECK(large.size() == 1u + 309u + 1u + 2u);
        CHECK(large.toStringView().substrByPosLen(0u, 6u) == "-17976");

        za::String largest;
        CHECK(za::fmtTo(largest, "{:.0f}", std::numeric_limits<double>::max()) == za::FmtResult::Ok);
        CHECK(largest.size() == 309u);
    }


    SECTION("Long double")
    {
        CHECK(za::fmtToString("{}", 123.5L) == za::String{"123.500000"});
    }

    SECTION("Recursive format from a custom type writes more than 512 bytes")
    {
        // 60 Vec2f values, each emits "(1.000000, 1.000000)" (20 chars) + ", " separator
        // → 60*20 + 59*2 = 1318 chars + brackets = 1320 bytes. Forces the heap path.
        customtypes::Vec2f values[60];
        for (auto& v : values)
        {
            v.x = 1.f;
            v.y = 1.f;
        }
        const customtypes::MiniVec<customtypes::Vec2f> v{values, 60u};

        za::String out;
        (void)za::fmtTo(out, "{}", v);

        CHECK(out.size() == 1320u);
        CHECK(out.data()[0] == '[');
        CHECK(out.data()[out.size() - 1u] == ']');
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Fmt.hpp - composition + width interaction")
{
    SECTION("Outer width pads composed inner content uniformly")
    {
        using customtypes::Vec2f;

        // Rendered inner: "(1.000000, 2.000000)" -- 20 chars.
        CHECK(za::fmtToString("[{:<25}]", Vec2f{1.f, 2.f}) == za::String{"[(1.000000, 2.000000)     ]"});
    }

    SECTION("Multiple levels of recursion + outer padding")
    {
        const int                       elems[] = {7, 8};
        const customtypes::MiniVec<int> v{elems, 2u};

        // Rendered inner: "[7, 8]" (6 chars). Outer width 10, right-align:
        CHECK(za::fmtToString("[{:>10}]", v) == za::String{"[    [7, 8]]"});
    }
}
