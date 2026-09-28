#include "StringifyStdStringUtil.hpp"     // IWYU pragma: keep
#include "StringifyStdStringViewUtil.hpp" // IWYU pragma: keep
#include "Tst/Tst.hpp"

#include "Zancle/String/Utf.hpp"

#include <initializer_list>
#include <iterator>
#include <locale>
#include <string>
#include <string_view>


namespace
{
////////////////////////////////////////////////////////////
const auto& getFacet()
{
    return std::use_facet<std::ctype<wchar_t>>(std::locale{});
}

} // namespace

using namespace std::string_view_literals;

// Create C++17-compatible aliases for std::u8string{_view}
using u8string      = std::basic_string<decltype(u8' ')>;
using u8string_view = std::basic_string_view<decltype(u8' ')>;

// NOLINTBEGIN(readability-qualified-auto)

TEST_CASE("[System] za::Utf8")
{
    static constexpr auto utf8 = u8"Zancle 🐌"sv;

    SECTION("decode")
    {
        std::u32string output;
        for (auto begin = utf8.cbegin(); begin < utf8.cend();)
        {
            char32_t character = 0;
            begin              = za::Utf8::decode(begin, utf8.cend(), character, 0);
            output.push_back(character);
        }
        CHECK(output == U"Zancle 🐌"sv);
    }

    SECTION("encode")
    {
        u8string output;

        SECTION("Default replacement character")
        {
            za::Utf8::encode(U' ', std::back_inserter(output), 0);
            CHECK(output == u8" "sv);
            za::Utf8::encode(U'🐌', std::back_inserter(output), 0);
            CHECK(output == u8" 🐌"sv);
            za::Utf8::encode(0xFF'FF'FF'FF, std::back_inserter(output), 0);
            CHECK(output == u8" 🐌"sv);
        }

        SECTION("Custom replacement character")
        {
            za::Utf8::encode(U' ', std::back_inserter(output), '?');
            CHECK(output == u8" "sv);
            za::Utf8::encode(U'🐌', std::back_inserter(output), '?');
            CHECK(output == u8" 🐌"sv);
            za::Utf8::encode(0xFF'FF'FF'FF, std::back_inserter(output), '?');
            CHECK(output == u8" 🐌?"sv);
        }
    }

    SECTION("next")
    {
        auto next = utf8.cbegin();
        CHECK(*next == u8'Z');
        next = za::Utf8::next(next, utf8.cend());
        CHECK(*next == u8'a');
        next = za::Utf8::next(next, utf8.cend());
        CHECK(*next == u8'n');
        next = za::Utf8::next(next, utf8.cend());
        CHECK(*next == u8'c');
        next = za::Utf8::next(next, utf8.cend());
        CHECK(*next == u8'l');
        next = za::Utf8::next(next, utf8.cend());
        CHECK(*next == u8'e');
        next = za::Utf8::next(next, utf8.cend());
        CHECK(*next == u8' ');
        next = za::Utf8::next(next, utf8.cend());
        CHECK(u8string_view(&*next, 4) == u8"🐌"sv);
        next = za::Utf8::next(next, utf8.cend());
        CHECK((next == utf8.cend()));
    }

    SECTION("count")
    {
        // "Zancle 🐌" = 6 letters + 1 space + 4-byte emoji = 11 bytes, 8 codepoints.
        REQUIRE(utf8.size() == 11);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cend()) == 8);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cbegin() + 11) == 8);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cbegin() + 10) == 8);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cbegin() + 9) == 8);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cbegin() + 8) == 8);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cbegin() + 7) == 7);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cbegin() + 6) == 6);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cbegin() + 5) == 5);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cbegin() + 4) == 4);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cbegin() + 3) == 3);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cbegin() + 2) == 2);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cbegin() + 1) == 1);
        CHECK(za::Utf8::count(utf8.cbegin(), utf8.cbegin()) == 0);
    }

    SECTION("fromAnsi")
    {
        static constexpr auto ansi = "abcdefg"sv;
        u8string              output;
        za::Utf8::fromAnsi(ansi.cbegin(), ansi.cend(), std::back_inserter(output), getFacet());
        CHECK(output == u8"abcdefg"sv);
    }

    SECTION("fromWide")
    {
        static constexpr auto wide = L"abçdéfgń"sv;
        u8string              output;
        za::Utf8::fromWide(wide.cbegin(), wide.cend(), std::back_inserter(output));
        CHECK(output == u8"abçdéfgń"sv);
    }

    SECTION("fromLatin1")
    {
        static constexpr auto latin1 =
            "\xA1"
            "ab\xE7"
            "d\xE9!"sv;
        u8string output;
        za::Utf8::fromLatin1(latin1.cbegin(), latin1.cend(), std::back_inserter(output));
        CHECK(output == u8"¡abçdé!"sv);
    }

    SECTION("toAnsi")
    {
        std::string output;

        SECTION("Default replacement character")
        {
            za::Utf8::toAnsi(utf8.cbegin(), utf8.cend(), std::back_inserter(output), 0, getFacet());
            CHECK(output == "Zancle "sv); // skipped
        }

        SECTION("Custom replacement character")
        {
            za::Utf8::toAnsi(utf8.cbegin(), utf8.cend(), std::back_inserter(output), '_', getFacet());
            CHECK(output == "Zancle _"sv);
        }
    }

    SECTION("toWide")
    {
        std::wstring output;

        SECTION("Default replacement character")
        {
            za::Utf8::toWide(utf8.cbegin(), utf8.cend(), std::back_inserter(output), 0);
            CHECK(output == L"Zancle 🐌"sv);
        }

        SECTION("Custom replacement character")
        {
            za::Utf8::toWide(utf8.cbegin(), utf8.cend(), std::back_inserter(output), L'_');
            CHECK(output == L"Zancle 🐌"sv);
        }
    }

    SECTION("toLatin1")
    {
        std::string output;

        SECTION("Default replacement character")
        {
            za::Utf8::toLatin1(utf8.cbegin(), utf8.cend(), std::back_inserter(output), 0);
            CHECK(output == "Zancle \0"sv);
        }

        SECTION("Custom replacement character")
        {
            za::Utf8::toLatin1(utf8.cbegin(), utf8.cend(), std::back_inserter(output), '_');
            CHECK(output == "Zancle _"sv);
        }
    }

    SECTION("toUtf8")
    {
        u8string output;
        za::Utf8::toUtf8(utf8.cbegin(), utf8.cend(), std::back_inserter(output));
        CHECK(output == utf8);
    }

    SECTION("toUtf16")
    {
        std::u16string output;
        za::Utf8::toUtf16(utf8.cbegin(), utf8.cend(), std::back_inserter(output));
        CHECK(output == u"Zancle 🐌"sv);
    }

    SECTION("toUtf32")
    {
        std::u32string output;
        za::Utf8::toUtf32(utf8.cbegin(), utf8.cend(), std::back_inserter(output));
        CHECK(output == U"Zancle 🐌"sv);
    }
}

TEST_CASE("[System] za::Utf16")
{
    static constexpr auto utf16 = u"Zancle 🐌"sv;

    SECTION("decode")
    {
        std::u32string output;
        for (auto begin = utf16.cbegin(); begin < utf16.cend();)
        {
            char32_t character = 0;
            begin              = za::Utf16::decode(begin, utf16.cend(), character, 0);
            output.push_back(character);
        }
        CHECK(output == U"Zancle 🐌"sv);
    }

    SECTION("encode")
    {
        std::u16string output;

        SECTION("Default replacement character")
        {
            za::Utf16::encode(U' ', std::back_inserter(output), 0);
            CHECK(output == u" "sv);
            za::Utf16::encode(U'🐌', std::back_inserter(output), 0);
            CHECK(output == u" 🐌"sv);
            za::Utf16::encode(0xFF'FF'FF'FF, std::back_inserter(output), 0);
            CHECK(output == u" 🐌"sv);
        }

        SECTION("Custom replacement character")
        {
            za::Utf16::encode(U' ', std::back_inserter(output), '?');
            CHECK(output == u" "sv);
            za::Utf16::encode(U'🐌', std::back_inserter(output), '?');
            CHECK(output == u" 🐌"sv);
            za::Utf16::encode(0xFF'FF'FF'FF, std::back_inserter(output), '?');
            CHECK(output == u" 🐌?"sv);
        }
    }

    SECTION("next")
    {
        auto next = utf16.cbegin();
        CHECK(*next == u'Z');
        next = za::Utf16::next(next, utf16.cend());
        CHECK(*next == u'a');
        next = za::Utf16::next(next, utf16.cend());
        CHECK(*next == u'n');
        next = za::Utf16::next(next, utf16.cend());
        CHECK(*next == u'c');
        next = za::Utf16::next(next, utf16.cend());
        CHECK(*next == u'l');
        next = za::Utf16::next(next, utf16.cend());
        CHECK(*next == u'e');
        next = za::Utf16::next(next, utf16.cend());
        CHECK(*next == u' ');
        next = za::Utf16::next(next, utf16.cend());
        CHECK(std::u16string_view(&*next, 2) == u"🐌"sv);
        next = za::Utf16::next(next, utf16.cend());
        CHECK((next == utf16.cend()));
    }

    SECTION("count")
    {
        // "Zancle 🐌" = 6 letters + 1 space + 2 surrogates = 9 code units, 8 codepoints.
        REQUIRE(utf16.size() == 9);
        CHECK(za::Utf16::count(utf16.cbegin(), utf16.cend()) == 8);
        CHECK(za::Utf16::count(utf16.cbegin(), utf16.cbegin() + 9) == 8);
        CHECK(za::Utf16::count(utf16.cbegin(), utf16.cbegin() + 8) == 8);
        CHECK(za::Utf16::count(utf16.cbegin(), utf16.cbegin() + 7) == 7);
        CHECK(za::Utf16::count(utf16.cbegin(), utf16.cbegin() + 6) == 6);
        CHECK(za::Utf16::count(utf16.cbegin(), utf16.cbegin() + 5) == 5);
        CHECK(za::Utf16::count(utf16.cbegin(), utf16.cbegin() + 4) == 4);
        CHECK(za::Utf16::count(utf16.cbegin(), utf16.cbegin() + 3) == 3);
        CHECK(za::Utf16::count(utf16.cbegin(), utf16.cbegin() + 2) == 2);
        CHECK(za::Utf16::count(utf16.cbegin(), utf16.cbegin() + 1) == 1);
        CHECK(za::Utf16::count(utf16.cbegin(), utf16.cbegin()) == 0);
    }

    SECTION("fromAnsi")
    {
        static constexpr auto ansi = "abcdefg"sv;
        std::u16string        output;
        za::Utf16::fromAnsi(ansi.cbegin(), ansi.cend(), std::back_inserter(output), getFacet());
        CHECK(output == u"abcdefg"sv);
    }

    SECTION("fromWide")
    {
        static constexpr auto wide = L"abçdéfgń"sv;
        std::u16string        output;
        za::Utf16::fromWide(wide.cbegin(), wide.cend(), std::back_inserter(output));
        CHECK(output == u"abçdéfgń"sv);
    }

    SECTION("fromLatin1")
    {
        static constexpr auto latin1 =
            "\xA1"
            "ab\xE7"
            "d\xE9!"sv;
        std::u16string output;
        za::Utf16::fromLatin1(latin1.cbegin(), latin1.cend(), std::back_inserter(output));
        CHECK(output == u"¡abçdé!"sv);
    }

    SECTION("toAnsi")
    {
        std::string output;

        SECTION("Default replacement character")
        {
            za::Utf16::toAnsi(utf16.cbegin(), utf16.cend(), std::back_inserter(output), 0, getFacet());
            CHECK(output == "Zancle "sv); // skipped
        }

        SECTION("Custom replacement character")
        {
            za::Utf16::toAnsi(utf16.cbegin(), utf16.cend(), std::back_inserter(output), '_', getFacet());
            CHECK(output == "Zancle _"sv);
        }
    }

    SECTION("toWide")
    {
        std::wstring output;

        SECTION("Default replacement character")
        {
            za::Utf16::toWide(utf16.cbegin(), utf16.cend(), std::back_inserter(output), 0);
            CHECK(output == L"Zancle 🐌"sv);
        }

        SECTION("Custom replacement character")
        {
            za::Utf16::toWide(utf16.cbegin(), utf16.cend(), std::back_inserter(output), '_');
            CHECK(output == L"Zancle 🐌"sv);
        }
    }

    SECTION("toLatin1")
    {
        std::string output;

        SECTION("Default replacement character")
        {
            za::Utf16::toLatin1(utf16.cbegin(), utf16.cend(), std::back_inserter(output), 0);
            CHECK(output == "Zancle \0"sv);
        }

        SECTION("Custom replacement character")
        {
            za::Utf16::toLatin1(utf16.cbegin(), utf16.cend(), std::back_inserter(output), '_');
            CHECK(output == "Zancle _"sv);
        }
    }

    SECTION("toUtf8")
    {
        u8string output;
        za::Utf16::toUtf8(utf16.cbegin(), utf16.cend(), std::back_inserter(output));
        CHECK(output == u8"Zancle 🐌"sv);
    }

    SECTION("toUtf16")
    {
        std::u16string output;
        za::Utf16::toUtf16(utf16.cbegin(), utf16.cend(), std::back_inserter(output));
        CHECK(output == utf16);
    }

    SECTION("toUtf32")
    {
        std::u32string output;
        za::Utf16::toUtf32(utf16.cbegin(), utf16.cend(), std::back_inserter(output));
        CHECK(output == U"Zancle 🐌"sv);
    }
}

TEST_CASE("[System] za::Utf32")
{
    static constexpr auto utf32 = U"Zancle 🐌"sv;

    SECTION("decode")
    {
        std::u32string output;
        for (auto begin = utf32.cbegin(); begin < utf32.cend();)
        {
            char32_t character = 0;
            begin              = za::Utf32::decode(begin, {}, character, 0);
            output.push_back(character);
        }
        CHECK(output == utf32);
    }

    SECTION("encode")
    {
        std::u32string output;
        for (const auto character : utf32)
            za::Utf32::encode(character, std::back_inserter(output), 0);
        CHECK(output == utf32);
    }

    SECTION("next")
    {
        auto next = utf32.cbegin();
        CHECK(*next == U'Z');
        next = za::Utf32::next(next, utf32.cend());
        CHECK(*next == U'a');
        next = za::Utf32::next(next, utf32.cend());
        CHECK(*next == U'n');
        next = za::Utf32::next(next, utf32.cend());
        CHECK(*next == U'c');
        next = za::Utf32::next(next, utf32.cend());
        CHECK(*next == U'l');
        next = za::Utf32::next(next, utf32.cend());
        CHECK(*next == U'e');
        next = za::Utf32::next(next, utf32.cend());
        CHECK(*next == U' ');
        next = za::Utf32::next(next, utf32.cend());
        CHECK(*next == U'🐌');
        next = za::Utf32::next(next, utf32.cend());
        CHECK((next == utf32.cend()));
    }

    SECTION("count")
    {
        // "Zancle 🐌" = 6 letters + 1 space + 1 emoji codepoint = 8 in UTF-32.
        REQUIRE(utf32.size() == 8);
        CHECK(za::Utf32::count(utf32.cbegin(), utf32.cend()) == 8);
        CHECK(za::Utf32::count(utf32.cbegin(), utf32.cbegin() + 8) == 8);
        CHECK(za::Utf32::count(utf32.cbegin(), utf32.cbegin() + 7) == 7);
        CHECK(za::Utf32::count(utf32.cbegin(), utf32.cbegin() + 6) == 6);
        CHECK(za::Utf32::count(utf32.cbegin(), utf32.cbegin() + 5) == 5);
        CHECK(za::Utf32::count(utf32.cbegin(), utf32.cbegin() + 4) == 4);
        CHECK(za::Utf32::count(utf32.cbegin(), utf32.cbegin() + 3) == 3);
        CHECK(za::Utf32::count(utf32.cbegin(), utf32.cbegin() + 2) == 2);
        CHECK(za::Utf32::count(utf32.cbegin(), utf32.cbegin() + 1) == 1);
        CHECK(za::Utf32::count(utf32.cbegin(), utf32.cbegin()) == 0);
    }

    SECTION("fromAnsi")
    {
        static constexpr auto ansi = "abcdefg"sv;
        std::u32string        output;
        za::Utf32::fromAnsi(ansi.cbegin(), ansi.cend(), std::back_inserter(output), getFacet());
        CHECK(output == U"abcdefg"sv);
    }

    SECTION("fromWide")
    {
        static constexpr auto wide = L"abçdéfgń"sv;
        std::u32string        output;
        za::Utf32::fromWide(wide.cbegin(), wide.cend(), std::back_inserter(output));
        CHECK(output == U"abçdéfgń"sv);
    }

    SECTION("fromLatin1")
    {
        static constexpr auto latin1 =
            "\xA1"
            "ab\xE7"
            "d\xE9!"sv;
        std::u32string output;
        za::Utf32::fromLatin1(latin1.cbegin(), latin1.cend(), std::back_inserter(output));
        CHECK(output == U"¡abçdé!"sv);
    }

    SECTION("toAnsi")
    {
        std::string output;

        SECTION("Default replacement character")
        {
            za::Utf32::toAnsi(utf32.cbegin(), utf32.cend(), std::back_inserter(output), 0, getFacet());
            CHECK(output == "Zancle "sv); // skipped
        }

        SECTION("Custom replacement character")
        {
            za::Utf32::toAnsi(utf32.cbegin(), utf32.cend(), std::back_inserter(output), '_', getFacet());
            CHECK(output == "Zancle _"sv);
        }
    }

    SECTION("toWide")
    {
        std::wstring output;

        SECTION("Default replacement character")
        {
            za::Utf32::toWide(utf32.cbegin(), utf32.cend(), std::back_inserter(output), 0);
            CHECK(output == L"Zancle 🐌"sv);
        }

        SECTION("Custom replacement character")
        {
            za::Utf32::toWide(utf32.cbegin(), utf32.cend(), std::back_inserter(output), L'_');
            CHECK(output == L"Zancle 🐌"sv);
        }
    }

    SECTION("toLatin1")
    {
        std::string output;

        SECTION("Default replacement character")
        {
            za::Utf32::toLatin1(utf32.cbegin(), utf32.cend(), std::back_inserter(output), 0);
            CHECK(output == "Zancle \0"sv);
        }

        SECTION("Custom replacement character")
        {
            za::Utf32::toLatin1(utf32.cbegin(), utf32.cend(), std::back_inserter(output), '_');
            CHECK(output == "Zancle _"sv);
        }
    }

    SECTION("toUtf8")
    {
        u8string output;
        za::Utf32::toUtf8(utf32.cbegin(), utf32.cend(), std::back_inserter(output));
        CHECK(output == u8"Zancle 🐌"sv);
    }

    SECTION("toUtf16")
    {
        std::u16string output;
        za::Utf32::toUtf16(utf32.cbegin(), utf32.cend(), std::back_inserter(output));
        CHECK(output == u"Zancle 🐌"sv);
    }

    SECTION("toUtf32")
    {
        std::u32string output;
        za::Utf32::toUtf32(utf32.cbegin(), utf32.cend(), std::back_inserter(output));
        CHECK(output == utf32);
    }

    SECTION("decodeAnsi")
    {
        CHECK(za::Utf32::decodeAnsi('\0', getFacet()) == U'\0');
        CHECK(za::Utf32::decodeAnsi(' ', getFacet()) == U' ');
        CHECK(za::Utf32::decodeAnsi('a', getFacet()) == U'a');
        CHECK(za::Utf32::decodeAnsi('A', getFacet()) == U'A');
    }

    SECTION("decodeWide")
    {
        CHECK(za::Utf32::decodeWide(L'\0') == U'\0');
        CHECK(za::Utf32::decodeWide(L' ') == U' ');
        CHECK(za::Utf32::decodeWide(L'a') == U'a');
        CHECK(za::Utf32::decodeWide(L'A') == U'A');
        CHECK(za::Utf32::decodeWide(L'é') == U'é');
        CHECK(za::Utf32::decodeWide(L'ń') == U'ń');
    }

    SECTION("encodeAnsi")
    {
        std::string output;

        SECTION("Default replacement character")
        {
            za::Utf32::encodeAnsi(U' ', std::back_inserter(output), 0, getFacet());
            CHECK(output == " "sv);
            za::Utf32::encodeAnsi(U'_', std::back_inserter(output), 0, getFacet());
            CHECK(output == " _"sv);
            za::Utf32::encodeAnsi(U'a', std::back_inserter(output), 0, getFacet());
            CHECK(output == " _a"sv);
            za::Utf32::encodeAnsi(U'🐌', std::back_inserter(output), 0, getFacet());
            CHECK(output == " _a"sv); // skipped
        }

        SECTION("Custom replacement character")
        {
            za::Utf32::encodeAnsi(U' ', std::back_inserter(output), '?', getFacet());
            CHECK(output == " "sv);
            za::Utf32::encodeAnsi(U'_', std::back_inserter(output), '?', getFacet());
            CHECK(output == " _"sv);
            za::Utf32::encodeAnsi(U'a', std::back_inserter(output), '?', getFacet());
            CHECK(output == " _a"sv);
            za::Utf32::encodeAnsi(U'🐌', std::back_inserter(output), '?', getFacet());
            CHECK(output == " _a?"sv);
        }
    }

    SECTION("encodeWide")
    {
        std::wstring output;

        SECTION("Default replacement character")
        {
            za::Utf32::encodeWide(U' ', std::back_inserter(output), 0);
            CHECK(output == L" "sv);
            za::Utf32::encodeWide(U'_', std::back_inserter(output), 0);
            CHECK(output == L" _"sv);
            za::Utf32::encodeWide(U'a', std::back_inserter(output), 0);
            CHECK(output == L" _a"sv);
            za::Utf32::encodeWide(U'🐌', std::back_inserter(output), 0);
            CHECK(output == L" _a🐌"sv);
        }

        SECTION("Custom replacement character")
        {
            za::Utf32::encodeWide(U' ', std::back_inserter(output), L'?');
            CHECK(output == L" "sv);
            za::Utf32::encodeWide(U'_', std::back_inserter(output), L'?');
            CHECK(output == L" _"sv);
            za::Utf32::encodeWide(U'a', std::back_inserter(output), L'?');
            CHECK(output == L" _a"sv);
            za::Utf32::encodeWide(U'🐌', std::back_inserter(output), L'?');
            CHECK(output == L" _a🐌"sv);
        }
    }
}


namespace
{
namespace UtfValidationTest // for unity builds
{
////////////////////////////////////////////////////////////
// Decode every codepoint of `bytes` (with U+FFFD as replacement)
[[nodiscard]] std::u32string decodeAllUtf8(const std::initializer_list<unsigned char> bytes)
{
    const std::string input(bytes.begin(), bytes.end());

    std::u32string result;
    for (auto it = input.cbegin(); it != input.cend();)
    {
        char32_t codepoint = 0;
        it                 = za::Utf8::decode(it, input.cend(), codepoint, U'�');
        result += codepoint;
    }

    return result;
}


////////////////////////////////////////////////////////////
[[nodiscard]] std::u32string decodeAllUtf16(const std::u16string& input)
{
    std::u32string result;
    for (auto it = input.cbegin(); it != input.cend();)
    {
        char32_t codepoint = 0;
        it                 = za::Utf16::decode(it, input.cend(), codepoint, U'�');
        result += codepoint;
    }

    return result;
}

} // namespace UtfValidationTest
} // namespace


TEST_CASE("[System] za::Utf8 decoding rejects ill-formed input")
{
    using UtfValidationTest::decodeAllUtf8;

    constexpr char32_t r = U'�';

    SECTION("Valid boundaries")
    {
        CHECK(decodeAllUtf8({0x7F}) == U"\u007F");
        CHECK(decodeAllUtf8({0xC2, 0x80}) == U"\u0080");
        CHECK(decodeAllUtf8({0xDF, 0xBF}) == U"߿");
        CHECK(decodeAllUtf8({0xE0, 0xA0, 0x80}) == U"ࠀ");
        CHECK(decodeAllUtf8({0xED, 0x9F, 0xBF}) == U"퟿");
        CHECK(decodeAllUtf8({0xEE, 0x80, 0x80}) == U"");
        CHECK(decodeAllUtf8({0xEF, 0xBF, 0xBF}) == U"￿");
        CHECK(decodeAllUtf8({0xF0, 0x90, 0x80, 0x80}) == U"\U00010000");
        CHECK(decodeAllUtf8({0xF4, 0x8F, 0xBF, 0xBF}) == U"\U0010FFFF");
    }

    SECTION("A broken sequence does not swallow the next character")
    {
        CHECK(decodeAllUtf8({0xC3, 0x41}) == std::u32string{r, U'A'});
        CHECK(decodeAllUtf8({0xE3, 0x81, 0x41, 0x42}) == std::u32string{r, U'A', U'B'});
        CHECK(decodeAllUtf8({0xF0, 0x9F, 0x98, 0x41}) == std::u32string{r, U'A'});
    }

    SECTION("Stray continuation bytes and invalid lead bytes")
    {
        CHECK(decodeAllUtf8({0x80, 0x41}) == std::u32string{r, U'A'});
        CHECK(decodeAllUtf8({0xBF}) == std::u32string{r});
        CHECK(decodeAllUtf8({0xF5, 0x80}) == std::u32string{r, r});
        CHECK(decodeAllUtf8({0xFF}) == std::u32string{r});
        CHECK(decodeAllUtf8({0xF8, 0x88, 0x80, 0x80, 0x80}) == std::u32string(5, r)); // old 5-byte form
    }

    SECTION("Overlong encodings")
    {
        CHECK(decodeAllUtf8({0xC0, 0x80}) == std::u32string{r, r}); // U+0000
        CHECK(decodeAllUtf8({0xC0, 0xAF}) == std::u32string{r, r}); // '/', classic path traversal trick
        CHECK(decodeAllUtf8({0xC1, 0xBF}) == std::u32string{r, r});
        CHECK(decodeAllUtf8({0xE0, 0x80, 0xAF}) == std::u32string{r, r, r}); // '/'
        CHECK(decodeAllUtf8({0xE0, 0x9F, 0xBF}) == std::u32string{r, r, r});
        CHECK(decodeAllUtf8({0xF0, 0x8F, 0xBF, 0xBF}) == std::u32string{r, r, r, r});
    }

    SECTION("Surrogates and codepoints above U+10FFFF")
    {
        CHECK(decodeAllUtf8({0xED, 0xA0, 0x80}) == std::u32string{r, r, r});          // U+D800
        CHECK(decodeAllUtf8({0xED, 0xBF, 0xBF}) == std::u32string{r, r, r});          // U+DFFF
        CHECK(decodeAllUtf8({0xF4, 0x90, 0x80, 0x80}) == std::u32string{r, r, r, r}); // U+110000
    }

    SECTION("Truncated sequences at the end of the input")
    {
        CHECK(decodeAllUtf8({0x41, 0xC3}) == std::u32string{U'A', r});
        CHECK(decodeAllUtf8({0xE3, 0x81}) == std::u32string{r});
        CHECK(decodeAllUtf8({0xF0, 0x9F, 0x98}) == std::u32string{r});
    }

    SECTION("Every valid codepoint round-trips, and every surrogate is rejected by the encoder")
    {
        bool allRoundTrip = true;
        bool noSurrogates = true;

        for (char32_t codepoint = 0; codepoint <= 0x10'FF'FF; ++codepoint)
        {
            std::string encoded;
            za::Utf8::encode(codepoint, std::back_inserter(encoded), '?');

            if (codepoint >= 0xD8'00 && codepoint <= 0xDF'FF)
            {
                noSurrogates &= encoded == "?";
                continue;
            }

            char32_t   decoded = 0;
            const auto end     = za::Utf8::decode(encoded.cbegin(), encoded.cend(), decoded, U'�');
            allRoundTrip &= decoded == codepoint && end == encoded.cend();
        }

        CHECK(allRoundTrip);
        CHECK(noSurrogates);
    }

    SECTION("count and next agree with decode")
    {
        const std::string input = "a\xC3\x41\xE2\x82\xAC\xF0";
        CHECK(za::Utf8::count(input.cbegin(), input.cend()) == 5u); // a, U+FFFD, A, U+20AC, U+FFFD
    }
}


TEST_CASE("[System] za::Utf conversions of ill-formed input do not produce U+0000")
{
    const std::string truncated = "ab\xE3";

    SECTION("To UTF-32 and UTF-16: U+FFFD")
    {
        std::u32string utf32;
        za::Utf8::toUtf32(truncated.cbegin(), truncated.cend(), std::back_inserter(utf32));
        CHECK(utf32 == U"ab�");

        std::u16string utf16;
        za::Utf8::toUtf16(truncated.cbegin(), truncated.cend(), std::back_inserter(utf16));
        CHECK(utf16 == u"ab�");

        const std::u16string loneSurrogate = {u'a', char16_t{0xD8'00}, u'b'};
        std::string          utf8;
        za::Utf16::toUtf8(loneSurrogate.cbegin(), loneSurrogate.cend(), std::back_inserter(utf8));
        CHECK(utf8 ==
              "a\xEF\xBF\xBD"
              "b");
    }

    SECTION("To encodings with a replacement parameter: the replacement, or nothing")
    {
        std::string latin1;
        za::Utf8::toLatin1(truncated.cbegin(), truncated.cend(), std::back_inserter(latin1), '?');
        CHECK(latin1 == "ab?");

        std::string ansi;
        za::Utf8::toAnsi(truncated.cbegin(), truncated.cend(), std::back_inserter(ansi), 0, getFacet());
        CHECK(ansi == "ab");

        std::wstring wide;
        za::Utf8::toWide(truncated.cbegin(), truncated.cend(), std::back_inserter(wide), L'?');
        CHECK(wide == L"ab?");
    }

    SECTION("UTF-32 input is validated too")
    {
        const std::u32string invalid = {U'a', char32_t{0xD8'00}, char32_t{0x11'00'00}};

        std::string utf8;
        za::Utf32::toUtf8(invalid.cbegin(), invalid.cend(), std::back_inserter(utf8));
        CHECK(utf8 == "a\xEF\xBF\xBD\xEF\xBF\xBD");

        std::u32string copy;
        za::Utf32::encode(char32_t{0xDC'00}, std::back_inserter(copy), U'?');
        za::Utf32::encode(char32_t{0x11'00'00}, std::back_inserter(copy), 0);
        CHECK(copy == U"?");
    }
}


TEST_CASE("[System] za::Utf16 decoding of unpaired surrogates")
{
    using UtfValidationTest::decodeAllUtf16;

    constexpr char16_t high = 0xD8'3D;
    constexpr char16_t low  = 0xDE'00;

    CHECK(decodeAllUtf16({high, low}) == U"\U0001F600");
    CHECK(decodeAllUtf16({high, u'A', u'B'}) == U"�AB"); // the 'A' is not swallowed
    CHECK(decodeAllUtf16({low, u'A'}) == U"�A");
    CHECK(decodeAllUtf16({u'A', high}) == U"A�");
    CHECK(decodeAllUtf16({high, high, low}) == U"�\U0001F600");

    const std::u16string input = {high, u'A', u'B'};
    CHECK(za::Utf16::count(input.cbegin(), input.cend()) == 3u);

    SECTION("toLatin1 works on codepoints, not code units")
    {
        const std::u16string withPair = {u'a', high, low, u'b'};

        std::string latin1;
        za::Utf16::toLatin1(withPair.cbegin(), withPair.cend(), std::back_inserter(latin1), '?');
        CHECK(latin1 == "a?b");
    }
}


TEST_CASE("[System] za::Utf wide strings are UTF-16 where wchar_t is 16-bit")
{
    const std::wstring wide = L"a\U0001F600b"; // a surrogate pair on Windows

    std::string utf8;
    za::Utf8::fromWide(wide.cbegin(), wide.cend(), std::back_inserter(utf8));
    CHECK(utf8 ==
          "a\xF0\x9F\x98\x80"
          "b");

    std::u16string utf16;
    za::Utf16::fromWide(wide.cbegin(), wide.cend(), std::back_inserter(utf16));
    CHECK(utf16 == u"a\U0001F600b");

    std::u32string utf32;
    za::Utf32::fromWide(wide.cbegin(), wide.cend(), std::back_inserter(utf32));
    CHECK(utf32 == U"a\U0001F600b");

    std::wstring roundTrip;
    za::Utf8::toWide(utf8.cbegin(), utf8.cend(), std::back_inserter(roundTrip), L'?');
    CHECK(roundTrip == wide);

    if constexpr (sizeof(wchar_t) == 2)
    {
        const std::wstring loneSurrogate = {L'a', static_cast<wchar_t>(0xD8'00), L'b'};

        std::u32string decoded;
        za::Utf32::fromWide(loneSurrogate.cbegin(), loneSurrogate.cend(), std::back_inserter(decoded));
        CHECK(decoded == U"a�b");
    }
}


TEST_CASE("[System] za::Utf toAnsi does not truncate codepoints that wchar_t cannot hold")
{
    const std::u32string input = {U'x', char32_t{0x1'00'41}}; // would alias 'A' if truncated to 16 bits

    std::string ansi;
    za::Utf32::toAnsi(input.cbegin(), input.cend(), std::back_inserter(ansi), '?', getFacet());
    CHECK(ansi == "x?");
}


TEST_CASE("[System] za::Utf raw pointers as output iterators")
{
    char        utf8[8]{};
    char* const utf8End = za::Utf8::encode(U'é', utf8, '?');
    CHECK(utf8End - utf8 == 2);
    CHECK(static_cast<unsigned char>(utf8[0]) == 0xC3);
    CHECK(static_cast<unsigned char>(utf8[1]) == 0xA9);

    const char32_t input[] = {U'a', U'\U0001F600'};
    char32_t       utf32[4]{};
    CHECK(za::Utf32::toUtf32(input, input + 2, utf32) == utf32 + 2);
    CHECK(utf32[1] == U'\U0001F600');

    const char latin1[] = "\xE9";
    char16_t   utf16[2]{};
    CHECK(za::Utf16::fromLatin1(latin1, latin1 + 1, utf16) == utf16 + 1);
    CHECK(utf16[0] == u'é');
}

// NOLINTEND(readability-qualified-auto)
