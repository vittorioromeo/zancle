#include "StringifyStdStringUtil.hpp" // IWYU pragma: keep
#include "StringifyZbStringUtil.hpp"  // IWYU pragma: keep
#include "Tst/Tst.hpp"

#include "Zancle/String/StringStreamOp.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/StringView.hpp"
#include "Zancle/String/StringViewStreamOp.hpp"

#include "Zancle/Container/Vector.hpp"

#include <sstream>


namespace
{
namespace StringStreamOpTest // for unity builds
{
////////////////////////////////////////////////////////////
[[nodiscard]] za::Vector<za::String> extractWords(const char* text)
{
    std::istringstream     in{text};
    za::Vector<za::String> words;

    for (za::String word; in >> word;)
        words.pushBack(word);

    return words;
}

} // namespace StringStreamOpTest
} // namespace


TEST_CASE("[Base] za::String stream extraction")
{
    using StringStreamOpTest::extractWords;

    SECTION("Words separated by any whitespace")
    {
        const auto words = extractWords("  alpha\tbeta\ngamma\r\n");
        REQUIRE(words.size() == 3u);
        CHECK(words[0] == "alpha");
        CHECK(words[1] == "beta");
        CHECK(words[2] == "gamma");
    }

    SECTION("No extra empty word after trailing whitespace")
    {
        CHECK(extractWords("a b ").size() == 2u); // used to also yield ""
        CHECK(extractWords("a b").size() == 2u);
    }

    SECTION("Empty and whitespace-only input yield no words")
    {
        CHECK(extractWords("").empty());
        CHECK(extractWords("   \n\t ").empty());
    }

    SECTION("A failed extraction leaves the string empty and fails the stream")
    {
        std::istringstream in{"  "};
        za::String         word{"previous"};

        CHECK(!(in >> word));
        CHECK(word.empty());
        CHECK(in.fail());
    }
}


TEST_CASE("[Base] za::String and za::StringView stream insertion")
{
    std::ostringstream out;
    out << za::String{"hello"} << ' ' << za::StringView{"world"};
    CHECK(out.str() == "hello world");
}
