#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Fmt/FmtAppendMixin.hpp"
#include "Zancle/Fmt/FmtNumeric.hpp" // IWYU pragma: keep -- enables int/float `fmtArg`

#include "Zancle/String/String.hpp"
#include "Zancle/String/StringView.hpp"
#include "Zancle/String/Utf8StringCodepoints.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/InitializerList.hpp"
#include "Zancle/Base/PtrDiffT.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Swap.hpp"

#include "Zancle/Trait/IsSame.hpp"


namespace
{
namespace Utf8StringTest // for unity builds
{
////////////////////////////////////////////////////////////
[[nodiscard]] za::Vector<char32_t> collect(const za::Utf8String& s)
{
    za::Vector<char32_t> out;
    for (const char32_t cp : s.codepoints())
        out.pushBack(cp);
    return out;
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool equals(const za::Vector<char32_t>& vec, std::initializer_list<char32_t> expected)
{
    if (vec.size() != expected.size())
        return false;

    const char32_t* it = expected.begin();
    for (za::SizeT i = 0; i < vec.size(); ++i, ++it)
        if (vec[i] != *it)
            return false;

    return true;
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - construction and basic accessors")
{
    SECTION("Default construction is empty")
    {
        const za::Utf8String s;
        CHECK(s.empty());
        CHECK(s.byteSize() == 0u);
        CHECK(s.codepointCount() == 0u);
    }

    SECTION("Construct from const char* (ASCII)")
    {
        const za::Utf8String s = "hello";
        CHECK(!s.empty());
        CHECK(s.byteSize() == 5u);
        CHECK(s.codepointCount() == 5u);
    }

    SECTION("Construct from const char* (multibyte)")
    {
        // "Hé" -> 'H' (1 byte) + 'é' = U+00E9 (2 bytes UTF-8: 0xC3 0xA9)
        const za::Utf8String s = "H\xC3\xA9";
        CHECK(s.byteSize() == 3u);
        CHECK(s.codepointCount() == 2u);
    }

    SECTION("Construct from za::StringView")
    {
        const za::StringView view = "view";
        const za::Utf8String s    = view;
        CHECK(s.byteSize() == 4u);
        CHECK(s.codepointCount() == 4u);
    }

    SECTION("Construct from za::String (move)")
    {
        za::String           owning = "moved";
        const za::Utf8String s      = static_cast<za::String&&>(owning);
        CHECK(s.byteSize() == 5u);
        CHECK(s.codepointCount() == 5u);
    }

    SECTION("Construct from empty const char*")
    {
        const za::Utf8String s = "";
        CHECK(s.empty());
        CHECK(s.byteSize() == 0u);
        CHECK(s.codepointCount() == 0u);
    }

    SECTION("Construct from (const char*, byte count) -- buffer not null-terminated")
    {
        // 5-byte ASCII slice taken from a larger buffer.
        const char           buffer[] = {'h', 'e', 'l', 'l', 'o', 'X', 'Y', 'Z'};
        const za::Utf8String s{buffer, 5u};
        CHECK(s.byteSize() == 5u);
        CHECK(s.codepointCount() == 5u);
        CHECK(s == "hello");
    }

    SECTION("Construct from (const char*, byte count) -- subslice of a UTF-8 buffer")
    {
        // "café" is 5 bytes ('c','a','f',0xC3,0xA9). Slice the first 3 bytes -> "caf".
        const char           buffer[] = "caf\xC3\xA9";
        const za::Utf8String s{buffer, 3u};
        CHECK(s.byteSize() == 3u);
        CHECK(s.codepointCount() == 3u);
        CHECK(s == "caf");
    }

    SECTION("Construct from (const char*, 0u) yields an empty string")
    {
        const char           buffer[] = "ignored";
        const za::Utf8String s{buffer, 0u};
        CHECK(s.empty());
    }

    SECTION("Construct from u8 literal (ASCII)")
    {
        const za::Utf8String s = u8"hello";
        CHECK(s.byteSize() == 5u);
        CHECK(s.codepointCount() == 5u);
        CHECK(s == "hello");
    }

    SECTION("Construct from u8 literal (multibyte)")
    {
        // "カタツムリ" -- 5 codepoints, 15 UTF-8 bytes.
        const za::Utf8String s = u8"カタツムリ";
        CHECK(s.byteSize() == 15u);
        CHECK(s.codepointCount() == 5u);
        CHECK(equals(collect(s), {U'カ', U'タ', U'ツ', U'ム', U'リ'}));
    }

    SECTION("Construct from u8 literal (emoji, 4-byte sequence)")
    {
        // U+1F40C snail emoji -- 4 bytes in UTF-8.
        const za::Utf8String s = u8"🐌";
        CHECK(s.byteSize() == 4u);
        CHECK(s.codepointCount() == 1u);
        CHECK(equals(collect(s), {U'\U0001F40C'}));
    }

    SECTION("Construct from (const char8_t*, byte count)")
    {
        const char8_t        buffer[] = u8"caf\xC3\xA9XYZ"; // "café" + trailing junk
        const za::Utf8String s{buffer, 5u};
        CHECK(s.byteSize() == 5u);
        CHECK(s.codepointCount() == 4u);
        // the test framework cannot stringify `char8_t[]`, so compare against a same-typed temporary.
        CHECK(s == za::Utf8String{u8"café"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - codepoint iteration")
{
    SECTION("ASCII")
    {
        const za::Utf8String s = "abc";
        CHECK(equals(collect(s), {U'a', U'b', U'c'}));
    }

    SECTION("Empty range yields no codepoints")
    {
        const za::Utf8String s;
        CHECK(collect(s).empty());
    }

    SECTION("2-byte sequence (Latin-1 supplement)")
    {
        // "café" -> 'c','a','f' (1 byte each), 'é' = U+00E9 (2 bytes)
        const za::Utf8String s = "caf\xC3\xA9";
        CHECK(s.byteSize() == 5u);
        CHECK(s.codepointCount() == 4u);
        CHECK(equals(collect(s), {U'c', U'a', U'f', U'é'}));
    }

    SECTION("3-byte sequence (BMP / CJK)")
    {
        // "カタツムリ" (Japanese, snail) -- 5 codepoints, 3 bytes each
        const za::Utf8String s = "\xE3\x82\xAB\xE3\x82\xBF\xE3\x83\x84\xE3\x83\xA0\xE3\x83\xAA";
        CHECK(s.byteSize() == 15u);
        CHECK(s.codepointCount() == 5u);
        CHECK(equals(collect(s), {U'カ', U'タ', U'ツ', U'ム', U'リ'}));
    }

    SECTION("4-byte sequence (supplementary plane / emoji)")
    {
        // "🐌" U+1F40C -- 4 bytes in UTF-8: 0xF0 0x9F 0x90 0x8C
        const za::Utf8String s = "\xF0\x9F\x90\x8C";
        CHECK(s.byteSize() == 4u);
        CHECK(s.codepointCount() == 1u);
        CHECK(equals(collect(s), {U'\U0001F40C'}));
    }

    SECTION("Mixed ASCII + multibyte")
    {
        // "a" (1) + "é" (2) + "カ" (3) + "🐌" (4) = 10 bytes, 4 codepoints
        const za::Utf8String s = "a\xC3\xA9\xE3\x82\xAB\xF0\x9F\x90\x8C";
        CHECK(s.byteSize() == 10u);
        CHECK(s.codepointCount() == 4u);
        CHECK(equals(collect(s), {U'a', U'é', U'カ', U'\U0001F40C'}));
    }

    SECTION("Truncated 4-byte sequence yields one replacement")
    {
        // Leading byte of a 4-byte sequence with only 2 bytes available.
        const za::Utf8String s = "\xF0\x9F";
        // The decoder consumes the rest in one step, emitting U+FFFD.
        const auto cps = collect(s);
        CHECK(cps.size() == 1u);
        CHECK(cps[0] == za::Utf8String::replacementCodepoint);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - forCodepoints")
{
    SECTION("ASCII walk")
    {
        const za::Utf8String s = "abc";
        za::Vector<char32_t> out;
        s.forCodepoints([&](char32_t cp) { out.pushBack(cp); });
        CHECK(equals(out, {U'a', U'b', U'c'}));
    }

    SECTION("Multibyte walk yields same codepoints as the iterator")
    {
        // "a" (1) + "é" (2) + "カ" (3) + "🐌" (4)
        const za::Utf8String s = "a\xC3\xA9\xE3\x82\xAB\xF0\x9F\x90\x8C";

        za::Vector<char32_t> viaCallback;
        s.forCodepoints([&](char32_t cp) { viaCallback.pushBack(cp); });

        const auto viaIterator = collect(s);

        CHECK(viaCallback.size() == viaIterator.size());
        for (za::SizeT i = 0; i < viaCallback.size(); ++i)
            CHECK(viaCallback[i] == viaIterator[i]);
    }

    SECTION("Empty string never invokes the callback")
    {
        const za::Utf8String s;
        int                  invocations = 0;
        s.forCodepoints([&](char32_t) { ++invocations; });
        CHECK(invocations == 0);
    }

    SECTION("Truncated trailing sequence yields one replacement codepoint")
    {
        const za::Utf8String s = "\xF0\x9F";
        za::Vector<char32_t> out;
        s.forCodepoints([&](char32_t cp) { out.pushBack(cp); });
        CHECK(out.size() == 1u);
        CHECK(out[0] == za::Utf8String::replacementCodepoint);
    }

    SECTION("Callback may mutate captured state freely")
    {
        const za::Utf8String s          = "hello";
        za::SizeT            asciiCount = 0;
        s.forCodepoints([&](char32_t cp)
        {
            if (cp < 0x80u)
                ++asciiCount;
        });
        CHECK(asciiCount == 5u);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - mutation")
{
    SECTION("appendCodepoint round-trips through codepoints()")
    {
        za::Utf8String s;
        CHECK(s.appendCodepoint(U'a'));
        CHECK(s.appendCodepoint(U'é'));          // 2-byte
        CHECK(s.appendCodepoint(U'カ'));         // 3-byte
        CHECK(s.appendCodepoint(U'\U0001F40C')); // 4-byte

        CHECK(s.byteSize() == 1u + 2u + 3u + 4u);
        CHECK(s.codepointCount() == 4u);
        CHECK(equals(collect(s), {U'a', U'é', U'カ', U'\U0001F40C'}));
    }

    SECTION("appendCodepoint reports invalid codepoints and leaves buffer unchanged")
    {
        za::Utf8String s;
        CHECK(!s.appendCodepoint(0xD8'00u));    // surrogate, invalid
        CHECK(!s.appendCodepoint(0x11'00'00u)); // above U+10FFFF, invalid
        CHECK(s.empty());

        CHECK(s.appendCodepoint(U'x'));
        CHECK(s.byteSize() == 1u);
        CHECK(s.codepointCount() == 1u);
    }

    SECTION("operator+= concatenates byte-wise")
    {
        za::Utf8String s = "ab";
        s += za::Utf8String{"cd"};
        CHECK(s.byteSize() == 4u);
        CHECK(static_cast<bool>(s == za::Utf8String{"abcd"}));
    }

    SECTION("operator+= with StringView")
    {
        za::Utf8String s = "ab";
        s += za::StringView{"cd"};
        CHECK(static_cast<bool>(s == za::Utf8String{"abcd"}));
    }

    SECTION("operator+ produces a new string")
    {
        const za::Utf8String a = "foo";
        const za::Utf8String b = "bar";
        CHECK(static_cast<bool>(a + b == za::Utf8String{"foobar"}));
    }

    SECTION("clear empties the string")
    {
        za::Utf8String s = "non-empty";
        CHECK(!s.empty());
        s.clear();
        CHECK(s.empty());
        CHECK(s.byteSize() == 0u);
    }

    SECTION("pushBack appends raw bytes")
    {
        za::Utf8String s;
        s.pushBack('a');
        s.pushBack('b');
        CHECK(s == "ab");
    }

    SECTION("append overloads")
    {
        za::Utf8String s = "ab";
        s.append(za::Utf8String{"cd"});
        s.append(za::StringView{"ef"});
        s.append("gh");
        s.append("ij+ignored", 2u);
        CHECK(s == "abcdefghij");
    }

    SECTION("reserve does not change content")
    {
        za::Utf8String s = "abc";
        s.reserve(64u);
        CHECK(s == "abc");
        CHECK(s.byteSize() == 3u);
    }

    SECTION("reserve allows incremental build without reallocation surprise")
    {
        za::Utf8String s;
        s.reserve(128u);
        for (int i = 0; i < 10; ++i)
            (void)s.appendCodepoint(U'カ'); // 3 bytes each
        CHECK(s.byteSize() == 30u);
        CHECK(s.codepointCount() == 10u);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - equality and conversion")
{
    SECTION("Equality on identical content")
    {
        CHECK(za::Utf8String{"hello"} == za::Utf8String{"hello"});
        CHECK(za::Utf8String{} == za::Utf8String{});
    }

    SECTION("Inequality on different content (synthesized != from C++23 ==)")
    {
        // `operator!=` is intentionally not defined; C++20+ rewrites
        // `a != b` as `!(a == b)`. These checks confirm that path.
        CHECK(za::Utf8String{"hello"} != za::Utf8String{"world"});
        CHECK(za::Utf8String{"a"} != za::Utf8String{"ab"});
    }

    SECTION("Equality with const char* (no temporary Utf8String)")
    {
        const za::Utf8String s = "hello";
        CHECK(s == "hello");
        CHECK(s != "world");

        // Operand-reversal also synthesized in C++20+.
        CHECK("hello" == s);
        CHECK("world" != s);
    }

    SECTION("Equality with za::StringView")
    {
        const za::Utf8String s    = "hello";
        const za::StringView view = "hello";
        CHECK(static_cast<bool>(s == view));
        CHECK(static_cast<bool>(view == s));

        const za::StringView other = "world";
        CHECK(static_cast<bool>(s != other));
    }

    SECTION("Implicit conversion to za::StringView")
    {
        const za::Utf8String s = "hello";
        const za::StringView v = s;
        CHECK(v.size() == 5u);
        CHECK(v.data() == s.data());
    }

    SECTION("cStr is null-terminated")
    {
        const za::Utf8String s = "abc";
        CHECK(s.cStr()[0] == 'a');
        CHECK(s.cStr()[1] == 'b');
        CHECK(s.cStr()[2] == 'c');
        CHECK(s.cStr()[3] == '\0');
    }

    SECTION("asBytes exposes the underlying za::String")
    {
        const za::Utf8String s = "abc";
        CHECK(s.asBytes().size() == 3u);
        CHECK(static_cast<bool>(s.asBytes() == za::StringView{"abc"}));
    }

    SECTION("asBytes() && moves the underlying bytes out (rvalue overload)")
    {
        // Use a string longer than the small-string-optimization threshold so
        // that the bytes live on the heap and the move-out can be observed via
        // pointer identity. `za::String::maxSsoSize` is 23 on a 64-bit build.
        za::Utf8String    s            = "this string is intentionally longer than SSO so it goes to the heap";
        const char* const originalData = s.data();

        za::String taken = static_cast<za::Utf8String&&>(s).asBytes();

        CHECK(taken.size() == 67u);
        CHECK(taken.data() == originalData); // heap pointer transferred, no copy
        CHECK(s.empty());                    // source is moved-from
    }

    SECTION("asBytes() && on a returned-by-value temporary skips the copy")
    {
        const auto makeUtf8 = [] { return za::Utf8String{"transient"}; };

        // `makeUtf8()` is an rvalue, so `.asBytes()` picks the && overload
        // and moves rather than returning a reference.
        za::String taken = makeUtf8().asBytes();

        CHECK(taken.size() == 9u);
        CHECK(static_cast<bool>(taken == za::StringView{"transient"}));
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - iterator byte pointer slicing")
{
    // "café" -- 5 bytes (c,a,f,0xC3,0xA9), 4 codepoints.
    // We want to slice off the trailing 'é'.
    const za::Utf8String s  = "caf\xC3\xA9";
    auto                 it = s.codepoints().begin();

    // Advance past the three ASCII codepoints.
    ++it;
    ++it;
    ++it;

    // Now `it` points at 'é'. The byte offset is 3.
    CHECK(it.bytePtr() == s.data() + 3u);

    // The substring [s.data(), it.bytePtr()) is the "caf" prefix.
    const za::StringView prefix{s.data(), static_cast<za::SizeT>(it.bytePtr() - s.data())};
    CHECK(static_cast<bool>(prefix == za::StringView{"caf"}));
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - iterator operations")
{
    SECTION("Postfix ++ returns the pre-increment state")
    {
        const za::Utf8String s        = "abc";
        auto                 it       = s.codepoints().begin();
        const auto           snapshot = it++;

        CHECK(*snapshot == U'a');
        CHECK(*it == U'b');
    }

    SECTION("operator== compares position (synthesized != follows)")
    {
        const za::Utf8String s     = "ab";
        const auto           range = s.codepoints();

        auto first = range.begin();
        auto same  = range.begin();
        CHECK(static_cast<bool>(first == same));

        ++same;
        CHECK(static_cast<bool>(first != same));

        ++first;
        CHECK(static_cast<bool>(first == same));

        // After two advances, both reach end.
        ++first;
        ++same;
        CHECK(static_cast<bool>(first == range.end()));
        CHECK(static_cast<bool>(same == range.end()));
    }

    SECTION("Standard typedefs are present")
    {
        // These should compile -- they're a static-properties check.
        using Iter = za::Utf8String::CodepointIter;
        static_assert(ZA_IS_SAME(Iter::value_type, char32_t));
        static_assert(ZA_IS_SAME(Iter::reference, char32_t));
        static_assert(ZA_IS_SAME(Iter::pointer, const char32_t*));
        static_assert(ZA_IS_SAME(Iter::difference_type, za::PtrDiffT));

        const za::Utf8String s  = "x";
        auto                 it = s.codepoints().begin();
        Iter::value_type     v  = *it;
        CHECK(v == U'x');
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - search/inspection")
{
    const za::Utf8String s = "snail: カタツムリ";

    SECTION("startsWith")
    {
        CHECK(s.startsWith(za::StringView{"snail"}));
        CHECK(!s.startsWith(za::StringView{"cat"}));
        CHECK(s.startsWith(za::StringView{""}));
    }

    SECTION("endsWith")
    {
        // 5 Japanese codepoints x 3 bytes = 15 bytes for "カタツムリ"
        CHECK(s.endsWith(za::StringView{"\xE3\x83\xAA"})); // 'リ' (3 bytes)
        CHECK(!s.endsWith(za::StringView{"snail"}));
    }

    SECTION("contains")
    {
        CHECK(s.contains(za::StringView{"ail:"}));
        CHECK(s.contains(za::StringView{"\xE3\x82\xAB"})); // 'カ'
        CHECK(!s.contains(za::StringView{"dog"}));
    }

    SECTION("find returns byte offset of first match")
    {
        // ':' is at byte index 5 in "snail: ..."
        CHECK(s.find(za::StringView{":"}) == 5u);
        // 'カ' starts at byte index 7 (after "snail: ")
        CHECK(s.find(za::StringView{"\xE3\x82\xAB"}) == 7u);
        // No match.
        CHECK(s.find(za::StringView{"zzz"}) == za::StringView::nPos);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - ordering")
{
    SECTION("ASCII lexicographic")
    {
        CHECK(za::Utf8String{"abc"} < za::Utf8String{"abd"});
        CHECK(za::Utf8String{"abc"} <= za::Utf8String{"abc"});
        CHECK(za::Utf8String{"abd"} > za::Utf8String{"abc"});
        CHECK(za::Utf8String{"abc"} >= za::Utf8String{"abc"});

        CHECK_FALSE(za::Utf8String{"abd"} < za::Utf8String{"abc"});
        CHECK_FALSE(za::Utf8String{"abc"} > za::Utf8String{"abc"});
    }

    SECTION("Prefix is less than longer string")
    {
        CHECK(za::Utf8String{"abc"} < za::Utf8String{"abcd"});
        CHECK_FALSE(za::Utf8String{"abcd"} < za::Utf8String{"abc"});
    }

    SECTION("Empty compares less than any non-empty")
    {
        CHECK(za::Utf8String{} < za::Utf8String{"a"});
        CHECK_FALSE(za::Utf8String{"a"} < za::Utf8String{});
        CHECK(za::Utf8String{} <= za::Utf8String{});
    }

    SECTION("Multibyte codepoints: byte-lex order matches codepoint order")
    {
        // 'A' (U+0041, 1 byte) < 'é' (U+00E9, 2 bytes) < 'カ' (U+30AB, 3 bytes) < '🐌' (U+1F40C, 4 bytes).
        // UTF-8 was designed so byte-lex ordering preserves codepoint ordering.
        CHECK(za::Utf8String{"A"} < za::Utf8String{"\xC3\xA9"});
        CHECK(za::Utf8String{"\xC3\xA9"} < za::Utf8String{"\xE3\x82\xAB"});
        CHECK(za::Utf8String{"\xE3\x82\xAB"} < za::Utf8String{"\xF0\x9F\x90\x8C"});
    }

    SECTION("Synthesized != via C++20 spaceship rewrites")
    {
        CHECK_FALSE(za::Utf8String{"abc"} == za::Utf8String{"abd"});
        CHECK(za::Utf8String{"abc"} != za::Utf8String{"abd"});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - replaceAllOccurrences")
{
    SECTION("ASCII substitution")
    {
        za::Utf8String s = "a b c";
        CHECK(s.replaceAllOccurrences(" ", "_") == 2u);
        CHECK(s == "a_b_c");
    }

    SECTION("Replacement longer than needle")
    {
        za::Utf8String s = "x,y,z";
        CHECK(s.replaceAllOccurrences(",", " -> ") == 2u);
        CHECK(s == "x -> y -> z");
    }

    SECTION("Replacement shorter than needle")
    {
        za::Utf8String s = "xxxx";
        CHECK(s.replaceAllOccurrences("xx", "y") == 2u);
        CHECK(s == "yy");
    }

    SECTION("Needle not found leaves string unchanged")
    {
        za::Utf8String s = "hello";
        CHECK(s.replaceAllOccurrences("Z", "!") == 0u);
        CHECK(s == "hello");
    }

    SECTION("Empty needle is a no-op")
    {
        za::Utf8String s = "abc";
        CHECK(s.replaceAllOccurrences("", "X") == 0u);
        CHECK(s == "abc");
    }

    SECTION("Multibyte needle is replaced byte-wise (UTF-8 is self-synchronizing)")
    {
        // "カタツ" with 'タ' replaced by 'X'
        za::Utf8String s = "\xE3\x82\xAB\xE3\x82\xBF\xE3\x83\x84";
        CHECK(s.replaceAllOccurrences("\xE3\x82\xBF", "X") == 1u); // 'タ' -> 'X'
        CHECK(s == "\xE3\x82\xABX\xE3\x83\x84");
    }

    SECTION("Replacement that contains the needle does not loop forever")
    {
        // Replacing "a" -> "aa" must not match the inserted 'a' again.
        za::Utf8String s = "aXa";
        CHECK(s.replaceAllOccurrences("a", "aa") == 2u);
        CHECK(s == "aaXaa");
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - replaceFirstOccurrence")
{
    SECTION("Replaces only the first match")
    {
        za::Utf8String s = "a b a c a";
        CHECK(s.replaceFirstOccurrence("a", "X"));
        CHECK(s == "X b a c a");
    }

    SECTION("Returns false when needle is not found")
    {
        za::Utf8String s = "hello";
        CHECK(!s.replaceFirstOccurrence("Z", "!"));
        CHECK(s == "hello");
    }

    SECTION("Empty needle is a no-op")
    {
        za::Utf8String s = "abc";
        CHECK(!s.replaceFirstOccurrence("", "X"));
        CHECK(s == "abc");
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - swap")
{
    SECTION("Member swap exchanges contents")
    {
        za::Utf8String a = "hello";
        za::Utf8String b = "world";
        a.swap(b);
        CHECK(a == "world");
        CHECK(b == "hello");
    }

    SECTION("ADL swap exchanges contents")
    {
        za::Utf8String a = "alpha";
        za::Utf8String b = "beta";
        swap(a, b);
        CHECK(a == "beta");
        CHECK(b == "alpha");
    }

    SECTION("za::genericSwap dispatches to the member swap")
    {
        za::Utf8String a = "one";
        za::Utf8String b = "two";
        za::genericSwap(a, b);
        CHECK(a == "two");
        CHECK(b == "one");
    }

    SECTION("Self-swap is a no-op")
    {
        za::Utf8String a = "self";
        za::genericSwap(a, a);
        CHECK(a == "self");
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - operator+ rvalue overload")
{
    SECTION("Rvalue lhs: chain reuses the lhs buffer")
    {
        // The compiler should pick the rvalue overload here; we can't
        // observe the buffer reuse directly without instrumentation,
        // but we can confirm the result is correct.
        const za::Utf8String result = za::Utf8String{"foo"} + za::Utf8String{"bar"};
        CHECK(result == "foobar");
    }

    SECTION("Long chain")
    {
        const za::Utf8String result = za::Utf8String{"a"} + za::Utf8String{"b"} + za::Utf8String{"c"} +
                                      za::Utf8String{"d"};
        CHECK(result == "abcd");
    }

    SECTION("Lvalue lhs: lvalue overload still works")
    {
        const za::Utf8String a      = "left";
        const za::Utf8String b      = "right";
        const za::Utf8String result = a + b;
        CHECK(result == "leftright");
        // Source operands unaffected.
        CHECK(a == "left");
        CHECK(b == "right");
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[System] za::Utf8String - FmtAppendMixin (appendFmt / appendArg)")
{
    SECTION("appendFmt on empty string")
    {
        za::Utf8String s;
        s.appendFmt("hello");
        CHECK(s == "hello");
    }

    SECTION("appendFmt with single placeholder")
    {
        za::Utf8String s;
        s.appendFmt("answer = {}", 42);
        CHECK(s == "answer = 42");
    }

    SECTION("appendFmt with multiple heterogeneous placeholders")
    {
        za::Utf8String s;
        s.appendFmt("{}-{}-{}", 1, 'x', za::StringView{"end"});
        CHECK(s == "1-x-end");
    }

    SECTION("appendFmt with width / precision / type-tag spec")
    {
        za::Utf8String s;
        s.appendFmt("[{:5}][{:x}][{:.3}]", 7, 255, 3.14159);
        CHECK(s == "[    7][ff][3.142]");
    }

    SECTION("appendFmt is genuinely appending, not replacing")
    {
        za::Utf8String s = "prefix:";
        s.appendFmt(" v={}", 7);
        CHECK(s == "prefix: v=7");

        s.appendFmt(",{}", 11);
        CHECK(s == "prefix: v=7,11");
    }

    SECTION("appendFmt preserves multibyte UTF-8 content already in the sink")
    {
        za::Utf8String s = "\xC3\xA9"; // U+00E9 'é'
        s.appendFmt(" n={}", 42);
        // Existing multibyte sequence must be untouched -- appendFmt is byte-level.
        CHECK(s == "\xC3\xA9 n=42");
    }

    SECTION("appendFmt round-trips a multibyte literal argument")
    {
        za::Utf8String s;
        // The format engine treats arguments as bytes; the multibyte
        // literal in the format string flows through unchanged.
        s.appendFmt("snail: \xF0\x9F\x90\x8C tag={}", 7);
        CHECK(s == "snail: \xF0\x9F\x90\x8C tag=7");
    }

    SECTION("appendArg appends a single value with no spec")
    {
        za::Utf8String s = "n=";
        s.appendArg(42);
        CHECK(s == "n=42");

        s.appendArg(',' /*char*/);
        CHECK(s == "n=42,");

        s.appendArg(za::StringView{"tail"});
        CHECK(s == "n=42,tail");
    }

    SECTION("appendArg with float renders default precision")
    {
        za::Utf8String s;
        s.appendArg(3.14159);
        CHECK(s == "3.141590");
    }
}

} // namespace Utf8StringTest
} // namespace
