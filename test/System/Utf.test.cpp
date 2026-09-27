#include "StringifyStdStringUtil.hpp"     // IWYU pragma: keep
#include "StringifyStdStringViewUtil.hpp" // IWYU pragma: keep
#include "Tst/Tst.hpp"

#include "Zancle/String/Utf.hpp"

#include "Zancle/Base/Assert.hpp"

#include <iterator>
#include <locale>
#include <string_view>


namespace
{
////////////////////////////////////////////////////////////
// Return either argument depending on whether wchar_t is 16 or 32 bits
// Lets us write tests that work on both Windows where wchar_t is 16 bits
// and elsewhere where it is 32. Otherwise the tests would only work on
// one OS or the other.
template <typename T>
auto select(const std::basic_string_view<T>& string16, const std::basic_string_view<T>& string32)
{
    ZA_ASSERT(string16 != string32 && "Invalid to select between identical inputs");
    if constexpr (sizeof(wchar_t) == 2)
        return string16;
    else
        return string32;
}

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
            CHECK(output == "Zancle \0"sv);
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
            CHECK(output == select(L"Zancle "sv, L"Zancle 🐌"sv));
        }

        SECTION("Custom replacement character")
        {
            za::Utf8::toWide(utf8.cbegin(), utf8.cend(), std::back_inserter(output), L'_');
            CHECK(output == select(L"Zancle _"sv, L"Zancle 🐌"sv));
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
            CHECK(output == "Zancle \0"sv);
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
            CHECK(output == select(L"Zancle "sv, L"Zancle 🐌"sv));
        }

        SECTION("Custom replacement character")
        {
            za::Utf16::toWide(utf16.cbegin(), utf16.cend(), std::back_inserter(output), '_');
            CHECK(output == select(L"Zancle _"sv, L"Zancle 🐌"sv));
        }
    }

    SECTION("toLatin1")
    {
        std::string output;

        SECTION("Default replacement character")
        {
            za::Utf16::toLatin1(utf16.cbegin(), utf16.cend(), std::back_inserter(output), 0);
            CHECK(output == "Zancle \0\0"sv);
        }

        SECTION("Custom replacement character")
        {
            za::Utf16::toLatin1(utf16.cbegin(), utf16.cend(), std::back_inserter(output), '_');
            CHECK(output == "Zancle __"sv);
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
            CHECK(output == "Zancle \0"sv);
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
            CHECK(output == select(L"Zancle "sv, L"Zancle 🐌"sv));
        }

        SECTION("Custom replacement character")
        {
            za::Utf32::toWide(utf32.cbegin(), utf32.cend(), std::back_inserter(output), L'_');
            CHECK(output == select(L"Zancle _"sv, L"Zancle 🐌"sv));
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
            CHECK(output == " _a\0"sv);
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
            CHECK(output == select(L" _a"sv, L" _a🐌"sv));
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
            CHECK(output == select(L" _a?"sv, L" _a🐌"sv));
        }
    }
}

// NOLINTEND(readability-qualified-auto)
