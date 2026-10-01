#include "Tst/Tst.hpp"

#include "Zancle/Base/Assert.hpp"

#include "Zancle/String/StringView.hpp"

#include <csetjmp>


#ifdef ZA_DEBUG

namespace
{
namespace AssertTest // for unity builds
{
////////////////////////////////////////////////////////////
// The handler cannot return normally (that would abort): it jumps back into the test instead
std::jmp_buf jumpBuffer;

const char* capturedCode = nullptr;
const char* capturedFile = nullptr;
int         capturedLine = 0;
int         handlerCalls = 0;


////////////////////////////////////////////////////////////
void recordingHandler(const char* code, const char* file, const int line)
{
    capturedCode = code;
    capturedFile = file;
    capturedLine = line;
    ++handlerCalls;

    std::longjmp(jumpBuffer, 1);
}


////////////////////////////////////////////////////////////
// Called through a function, so that `ZA_ASSERT`'s line is known and nothing with a destructor is jumped over
[[gnu::noinline]] void failAssertion(const int value)
{
    ZA_ASSERT(value == 42);
}

} // namespace AssertTest
} // namespace


TEST_CASE("[Base] Assert.hpp")
{
    SECTION("A user handler receives failed assertions")
    {
        AssertTest::handlerCalls = 0;

        const za::AssertHandler previous = za::setAssertHandler(&AssertTest::recordingHandler);

        if (setjmp(AssertTest::jumpBuffer) == 0)
        {
            AssertTest::failAssertion(0);
            FAIL("unreachable: the failed assertion must call the handler");
        }

        CHECK(za::setAssertHandler(previous) == &AssertTest::recordingHandler);

        CHECK(AssertTest::handlerCalls == 1);
        CHECK(za::StringView{AssertTest::capturedCode} == za::StringView{"value == 42"});
        CHECK(za::StringView{AssertTest::capturedFile}.find("Assert.test.cpp") != za::StringView::nPos);
        CHECK(AssertTest::capturedLine > 0);
    }

    SECTION("Passing assertions do not call the handler")
    {
        AssertTest::handlerCalls = 0;

        const za::AssertHandler previous = za::setAssertHandler(&AssertTest::recordingHandler);

        if (setjmp(AssertTest::jumpBuffer) == 0)
            AssertTest::failAssertion(42);

        CHECK(za::setAssertHandler(previous) == &AssertTest::recordingHandler);
        CHECK(AssertTest::handlerCalls == 0);
    }
}

#else

TEST_CASE("[Base] Assert.hpp")
{
    // Without `ZA_DEBUG`, there are no assertions, but the handler can still be installed
    const za::AssertHandler previous = za::setAssertHandler(nullptr);
    CHECK(za::setAssertHandler(previous) == nullptr);
}

#endif
