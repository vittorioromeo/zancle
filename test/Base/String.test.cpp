#include "StringifyStringViewUtil.hpp" // IWYU pragma: keep
#include "StringifyZbStringUtil.hpp"   // IWYU pragma: keep
#include "Tst/Tst.hpp"

#include "Zancle/String/String.hpp"

#include "Zancle/Fmt/FmtAppendMixin.hpp"
#include "Zancle/Fmt/FmtNumeric.hpp" // IWYU pragma: keep -- enables int/float `fmtArg`

#include "Zancle/String/StringView.hpp"
#include "Zancle/String/StringViewSplits.hpp" // IWYU pragma: keep

#include "Zancle/Algorithm/Copy.hpp"

#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsMoveAssignable.hpp"
#include "Zancle/Trait/IsMoveConstructible.hpp"
#include "Zancle/Trait/IsTrivial.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"


namespace
{

za::String makeLongString(const za::StringView& base)
{
    za::String result;
    result.reserve(base.size() * 2 + 30);
    result.append(base);
    return result;
}

} // namespace


TEST_CASE("[Base] Base/String.hpp")
{
    // A string guaranteed to be longer than the SSO buffer
    const char*    longStringLiteral = "This is a long string that will definitely not fit in SSO.";
    constexpr auto maxSsoSize        = za::String::maxSsoSize;

    SECTION("Type traits")
    {
        // String has manual resource management, so it should not be trivial.
        STATIC_CHECK(!ZA_IS_TRIVIAL(za::String));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPYABLE(za::String));

        // But it should be fully copyable and movable.
        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::String));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::String));
        STATIC_CHECK(ZA_IS_MOVE_CONSTRUCTIBLE(za::String));
        STATIC_CHECK(ZA_IS_MOVE_ASSIGNABLE(za::String));

        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::String));
    }

    SECTION("Constexpr")
    {
        constexpr za::String emptyStr;
    }

    SECTION("Constructors")
    {
        SUBCASE("Default constructor")
        {
            constexpr za::String str;
            STATIC_CHECK(str.isSso());
            STATIC_CHECK(str.size() == 0);
            STATIC_CHECK(str.capacity() == maxSsoSize);
            STATIC_CHECK(str.empty());
            STATIC_CHECK(str[0] == '\0');

            // Default constructor is non-explicit: must work in copy-list-init contexts.
            constexpr za::String copyListInit = {};
            STATIC_CHECK(copyListInit.isSso());
            STATIC_CHECK(copyListInit.empty());

            const auto returnsEmpty = []() -> za::String { return {}; };
            CHECK(returnsEmpty().empty());
        }

        SUBCASE("From const char*")
        {
            za::String ssoStr("Short");
            CHECK(ssoStr.isSso());
            CHECK(ssoStr.size() == 5);
            CHECK(ssoStr.capacity() == maxSsoSize);
            CHECK(ssoStr == "Short");

            za::String heapStr(longStringLiteral);
            CHECK(!heapStr.isSso());
            CHECK(heapStr.size() == za::StringView(longStringLiteral).size());
            CHECK(heapStr.capacity() >= heapStr.size());
            CHECK(heapStr == longStringLiteral);
        }

        SUBCASE("From const char* and count")
        {
            za::String ssoStr("Short", 3);
            CHECK(ssoStr.isSso());
            CHECK(ssoStr.size() == 3);
            CHECK(ssoStr == "Sho");

            za::String heapStr(longStringLiteral, maxSsoSize + 5);
            CHECK(!heapStr.isSso());
            CHECK(heapStr.size() == maxSsoSize + 5);
            CHECK(za::StringView(heapStr) == za::StringView(longStringLiteral, maxSsoSize + 5));
        }

        SUBCASE("From StringView")
        {
            za::StringView ssoView("Short");
            za::String     ssoStr(ssoView);
            CHECK(ssoStr.size() == 5);
            CHECK(ssoStr == "Short");

            za::StringView heapView(longStringLiteral);
            za::String     heapStr(heapView);
            CHECK(heapStr.size() == heapView.size());
            CHECK(heapStr == longStringLiteral);
        }

        SUBCASE("Copy constructor")
        {
            za::String ssoOrig("Short");
            CHECK(ssoOrig.isSso());
            CHECK(ssoOrig.size() == 5);
            CHECK(ssoOrig == "Short");

            za::String ssoCopy(ssoOrig); // NOLINT(performance-unnecessary-copy-initialization)
            CHECK(ssoCopy.isSso());
            CHECK(ssoCopy.size() == 5);
            CHECK(ssoCopy == "Short");

            za::String heapOrig(longStringLiteral);
            CHECK(!heapOrig.isSso());
            CHECK(heapOrig == longStringLiteral);

            za::String heapCopy(heapOrig);
            CHECK(!heapCopy.isSso());
            CHECK(heapCopy == longStringLiteral);
            CHECK(heapCopy.size() == heapOrig.size());
            CHECK(heapCopy.data() != heapOrig.data()); // Must be a deep copy
        }

        SUBCASE("Move constructor")
        {
            za::String ssoOrig("Short");
            CHECK(ssoOrig.isSso());
            CHECK(ssoOrig.size() == 5);
            CHECK(ssoOrig == "Short");

            za::String ssoMoved(ZA_MOVE(ssoOrig));
            CHECK(ssoMoved.size() == 5);
            CHECK(ssoMoved == "Short");

            // Source must be sso to avoid double-free
            CHECK(ssoOrig.isSso());

            za::String heapOrig(longStringLiteral);
            CHECK(!heapOrig.isSso());
            CHECK(heapOrig == longStringLiteral);

            const char* origPtr = heapOrig.data();

            za::String heapMoved(ZA_MOVE(heapOrig));
            CHECK(!heapMoved.isSso());
            CHECK(heapMoved == longStringLiteral);
            CHECK(heapMoved.size() == za::StringView(longStringLiteral).size());
            CHECK(heapMoved == longStringLiteral);
            CHECK(heapMoved.data() == origPtr); // Pointer must be stolen

            // Source will be in empty SSO state
            CHECK(heapOrig.isSso());
            CHECK(heapOrig.empty());
        }
    }

    SECTION("Assignment")
    {
        SUBCASE("Copy assignment")
        {
            za::String ssoStr("SSO");
            CHECK(ssoStr.isSso());
            CHECK(ssoStr.size() == 3);
            CHECK(ssoStr == "SSO");

            za::String heapStr(longStringLiteral);
            CHECK(!heapStr.isSso());
            CHECK(heapStr == longStringLiteral);

            za::String dest;
            CHECK(dest.isSso());
            CHECK(dest.empty());

            dest = ssoStr; // SSO -> Empty
            CHECK(dest.isSso());
            CHECK(!dest.empty());
            CHECK(dest == "SSO");
            CHECK(!ssoStr.empty());
            CHECK(ssoStr == "SSO");

            dest = heapStr; // Heap -> SSO
            CHECK(!dest.isSso());
            CHECK(!dest.empty());
            CHECK(dest == longStringLiteral);
            CHECK(dest.data() != heapStr.data());
            CHECK(!heapStr.isSso());
            CHECK(heapStr == longStringLiteral);

            heapStr = dest; // Heap -> Heap
            CHECK(!heapStr.isSso());
            CHECK(heapStr == longStringLiteral);

            heapStr = ssoStr; // SSO -> Heap
            CHECK(!heapStr.isSso());
            CHECK(!heapStr.empty());
            CHECK(heapStr == "SSO");
        }

        SUBCASE("Move assignment")
        {
            za::String ssoStr("SSO");
            CHECK(ssoStr.isSso());
            CHECK(ssoStr.size() == 3);
            CHECK(ssoStr == "SSO");

            za::String heapStr(longStringLiteral);
            CHECK(!heapStr.isSso());
            CHECK(heapStr == longStringLiteral);

            auto* origPtr = heapStr.data();

            za::String dest;
            CHECK(dest.isSso());
            CHECK(dest.empty());

            dest = ZA_MOVE(ssoStr); // SSO -> empty
            CHECK(dest.isSso());
            CHECK(dest == "SSO");
            CHECK(ssoStr.isSso()); // Source must be sso to avoid double-free

            dest = ZA_MOVE(heapStr); // Heap -> SSO
            CHECK(dest == longStringLiteral);
            CHECK(heapStr.isSso());
            CHECK(heapStr.empty());

            CHECK(!dest.isSso());
            CHECK(dest == longStringLiteral);
            CHECK(dest.size() == za::StringView(longStringLiteral).size());
            CHECK(dest == longStringLiteral);
            CHECK(dest.data() == origPtr); // Pointer must be stolen
        }

        SUBCASE("C-string and StringView assignment")
        {
            za::String str;
            str = "From C-string";
            CHECK(str == "From C-string");

            str = "From StringView";
            CHECK(str == "From StringView");

            str = longStringLiteral;
            CHECK(str == longStringLiteral);
        }
    }

    SECTION("Modifiers")
    {
        SUBCASE("clear")
        {
            za::String ssoStr("sso");
            CHECK(ssoStr.isSso());
            CHECK(ssoStr.size() == 3);
            CHECK(ssoStr == "sso");

            ssoStr.clear();
            CHECK(ssoStr.isSso());
            CHECK(ssoStr.empty());
            CHECK(ssoStr.size() == 0);
            CHECK(ssoStr.capacity() == maxSsoSize); // Capacity is preserved

            za::String heapStr(longStringLiteral);
            CHECK(!heapStr.isSso());
            CHECK(heapStr == longStringLiteral);

            const auto origHeapCap = heapStr.capacity();

            heapStr.clear();
            CHECK(!heapStr.isSso());
            CHECK(heapStr.empty());
            CHECK(heapStr.size() == 0);
            CHECK(heapStr.capacity() == origHeapCap); // Capacity is preserved
        }

        SUBCASE("pushBack")
        {
            za::String str;
            CHECK(str.isSso());
            CHECK(str.empty());
            CHECK(str == "");

            str.pushBack('A');
            CHECK(str.isSso());
            CHECK(str.size() == 1);
            CHECK(str == "A");
            CHECK(za::StringView(str) == "A");

            // Fill up to SSO limit
            for (za::SizeT i = 1; i < maxSsoSize; ++i)
                str.pushBack('B');

            CHECK(str.isSso());
            CHECK(str.size() == maxSsoSize);
            CHECK(str.capacity() == maxSsoSize);

            // Trigger transition to heap
            str.pushBack('C');
            CHECK(!str.isSso());
            CHECK(str.size() == maxSsoSize + 1);
            CHECK(str.capacity() > maxSsoSize);
            CHECK(str[maxSsoSize] == 'C');

            // Append on heap
            str.pushBack('D');
            CHECK(!str.isSso());
            CHECK(str.size() == maxSsoSize + 2);
            CHECK(str[maxSsoSize + 1] == 'D');
        }

        SUBCASE("popBack")
        {
            // Pop on SSO string
            za::String ssoStr("abc");
            CHECK(ssoStr.isSso());
            CHECK(ssoStr.size() == 3);

            ssoStr.popBack();
            CHECK(ssoStr.isSso());
            CHECK(ssoStr.size() == 2);
            CHECK(ssoStr == "ab");
            CHECK(ssoStr.cStr()[2] == '\0'); // Null terminator written

            ssoStr.popBack();
            CHECK(ssoStr == "a");
            CHECK(ssoStr.cStr()[1] == '\0');

            ssoStr.popBack();
            CHECK(ssoStr.empty());
            CHECK(ssoStr.size() == 0);
            CHECK(ssoStr == "");
            CHECK(ssoStr.cStr()[0] == '\0');

            // Pop on heap string preserves capacity and heap-ness
            za::String heapStr(longStringLiteral);
            CHECK(!heapStr.isSso());

            const auto origCap  = heapStr.capacity();
            const auto origSize = heapStr.size();

            heapStr.popBack();
            CHECK(!heapStr.isSso());
            CHECK(heapStr.size() == origSize - 1);
            CHECK(heapStr.capacity() == origCap);
            CHECK(heapStr.cStr()[heapStr.size()] == '\0');
            CHECK(za::StringView(heapStr) == za::StringView(longStringLiteral, origSize - 1));

            // Pop down to one character on heap (stays heap, capacity unchanged)
            while (heapStr.size() > 1)
                heapStr.popBack();

            CHECK(!heapStr.isSso());
            CHECK(heapStr.size() == 1);
            CHECK(heapStr.capacity() == origCap);
            CHECK(heapStr[0] == longStringLiteral[0]);

            heapStr.popBack();
            CHECK(!heapStr.isSso()); // Still on heap, popBack does not deallocate
            CHECK(heapStr.empty());
            CHECK(heapStr.capacity() == origCap);

            // pushBack/popBack round-trip on a freshly-grown heap string
            za::String roundTrip;
            for (za::SizeT i = 0; i < maxSsoSize + 5; ++i)
                roundTrip.pushBack(static_cast<char>('a' + (i % 26)));

            CHECK(!roundTrip.isSso());
            const auto rtSize = roundTrip.size();

            roundTrip.pushBack('Z');
            CHECK(roundTrip.size() == rtSize + 1);
            CHECK(roundTrip.back() == 'Z');

            roundTrip.popBack();
            CHECK(roundTrip.size() == rtSize);
            CHECK(roundTrip.back() == static_cast<char>('a' + ((rtSize - 1) % 26)));
            CHECK(roundTrip.cStr()[roundTrip.size()] == '\0');
        }

        SUBCASE("append")
        {
            // Build a string via two appends that stays within SSO
            const za::String partA(longStringLiteral, maxSsoSize / 3);
            const za::String partB(longStringLiteral, maxSsoSize / 3);
            const za::String partC(longStringLiteral, maxSsoSize / 3);

            za::String str(partA);
            CHECK(str.isSso());

            str.append(za::StringView(partB));
            CHECK(str.isSso());

            str.append(za::StringView(partC));
            CHECK(str.isSso());

            const auto prefixSize = str.size();

            za::String finalPart(longStringLiteral);
            str.append(finalPart);
            CHECK(!str.isSso());
            CHECK(str.size() == prefixSize + finalPart.size());
            CHECK(za::StringView(str).substrByPosLen(prefixSize) == longStringLiteral);
        }

        SUBCASE("append2")
        {
            za::String str;
            CHECK(str.isSso());
            CHECK(str.empty());

            str += 'a';
            CHECK(str.isSso());
            CHECK(str == "a");

            str += "bc";
            CHECK(str.isSso());
            CHECK(str == "abc");

            str += longStringLiteral;
            CHECK(!str.isSso());
            CHECK(str.substrByPosLen(3u) == longStringLiteral);
        }
    }

    SECTION("Iterators")
    {
        za::String       str("abc");
        const za::String cStr("xyz");

        // Basic iteration
        za::String result;
        for (char c : str)
            result += c;

        CHECK(result == "abc");

        // Const iteration
        result.clear();
        for (char c : cStr)
            result += c;

        CHECK(result == "xyz");

        // Algorithm compatibility
        char buffer[4] = {};
        za::copy(str.begin(), str.end(), buffer);
        CHECK(za::StringView(buffer, 3) == "abc");
    }

    SECTION("Comparison")
    {
        za::String sso1("abc");
        za::String sso2("abc");
        za::String sso3("def");
        za::String heap1(longStringLiteral);
        za::String heap2(longStringLiteral);
        za::String heap3("A different long string for comparison");

        CHECK(sso1 == sso2);
        CHECK(sso1 != sso3);
        CHECK(heap1 == heap2);
        CHECK(heap1 != heap3);

        // Cross-type comparisons
        CHECK(sso1 == "abc");
        CHECK("abc" == sso1);
        CHECK(sso1 != "def");
        CHECK(heap1 == longStringLiteral);
        CHECK(za::StringView("abc") == sso1);
    }

    SECTION("Swap")
    {
        SUBCASE("SSO with SSO")
        {
            za::String a("A");
            za::String b("B");
            swap(a, b);
            CHECK(za::StringView(a) == "B");
            CHECK(za::StringView(b) == "A");
        }

        SUBCASE("SSO with Heap")
        {
            za::String  a("Short");
            za::String  b(longStringLiteral);
            const char* bPtr = b.data();

            swap(a, b);

            CHECK(za::StringView(a) == longStringLiteral);
            CHECK(a.data() == bPtr); // Pointer must be swapped
            CHECK(za::StringView(b) == "Short");
        }

        SUBCASE("Heap with Heap")
        {
            za::String  a(longStringLiteral);
            za::String  b("Another long string that is also on the heap");
            const char* aPtr = a.data();
            const char* bPtr = b.data();

            swap(a, b);

            CHECK(a.data() == bPtr);
            CHECK(b.data() == aPtr);
        }
    }

    SECTION("Move from heap string")
    {
        za::String heapStr(longStringLiteral);
        CHECK(!heapStr.isSso());
        CHECK(heapStr == longStringLiteral);

        za::String dest = ZA_MOVE(heapStr); // Heap -> empty
        CHECK(!dest.isSso());
        CHECK(dest == longStringLiteral);
        CHECK(heapStr.isSso());
    }

    SECTION("Self-append")
    {
        za::String str("Hello");

        str.append(str);
        CHECK(str == "HelloHello");

        str.append(str);
        CHECK(str == "HelloHelloHelloHello");

        str.append(str);
        CHECK(str == "HelloHelloHelloHelloHelloHelloHelloHello");

        str.append(str);
        CHECK(str == "HelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHello");
    }

    SECTION("SsoSelfAppend")
    {
        // Small string: should be SSO and appending itself should produce "abcabc"
        za::String s("abc");
        CHECK(s.size() == 3u);
        s.append(s); // append itself
        CHECK(s.size() == 6u);
        CHECK(s == "abcabc");
    }


    SECTION("InPlaceSelfAppend_NoRealloc_UsesMemmovePath")
    {
        const auto makePattern = [](const za::SizeT n)
        {
            za::String s;
            s.reserve(n);

            for (za::SizeT i = 0u; i < n; ++i)
                s.pushBack(char('a' + (i % 26)));

            return s;
        };

        // Prepare a reasonably large string so it uses heap storage
        const za::SizeT initialSize = 128;
        const auto      pattern     = makePattern(initialSize);

        // Construct our za::String on the heap
        za::String s(pattern.cStr(), static_cast<za::SizeT>(pattern.size()));
        CHECK(s.size() == static_cast<za::SizeT>(initialSize));

        // Ensure capacity is large enough to fit the result of appending itself
        const auto needed = static_cast<za::SizeT>(initialSize * 2);
        s.reserve(needed);

        // Sanity: capacity must be >= needed (so append won't reallocate)
        CHECK(s.capacity() == needed);

        // Capture data pointer before append -- if append reallocates, this will change.
        const char* dataBefore = s.data();

        // Perform self-append (source overlaps destination)
        s.append(s);

        // Data pointer should be unchanged (no reallocation) -> in-place path executed
        const char* dataAfter = s.data();
        CHECK(dataBefore == dataAfter);

        // Size doubled
        CHECK(s.size() == needed);

        // First half equals original
        CHECK(za::StringView{s.cStr(), initialSize} == za::StringView{pattern.cStr(), initialSize});

        // Second half equals original
        CHECK(za::StringView{s.cStr() + initialSize, initialSize} == za::StringView{pattern.cStr(), initialSize});
    }

    SECTION("Self-Assignment")
    {
        SECTION("s = s with SSO string")
        {
            za::String s = "hello sso";
            CHECK(s.isSso());
            const char* originalData = s.data();

            auto& self = s;
            s          = self; // Self-assignment (avoid warning)

            CHECK(s == "hello sso");
            CHECK(s.data() == originalData); // Should not have reallocated
        }

        SECTION("s = s with Heap string")
        {
            za::String s = makeLongString("this is a long string definitely on the heap");
            CHECK(!s.isSso());
            const char* originalData = s.data();
            const auto  originalCap  = s.capacity();

            auto& self = s;
            s          = self; // Self-assignment (avoid warning)

            CHECK(s == "this is a long string definitely on the heap");
            CHECK(s.data() == originalData); // Should not have reallocated
            CHECK(s.capacity() == originalCap);
        }

        SECTION("s = s.toStringView() with SSO string")
        {
            za::String  s            = "sso view";
            const char* originalData = s.data();

            s = s.toStringView();

            CHECK(s == "sso view");
            CHECK(s.data() == originalData);
        }

        SECTION("s = s.toStringView() with Heap string")
        {
            za::String  s            = makeLongString("heap view assignment");
            const char* originalData = s.data();

            s = s.toStringView();

            CHECK(s == "heap view assignment");
            CHECK(s.data() == originalData);
        }

        SECTION("Assigning an overlapping substring (tests memmove)")
        {
            // Case 1: Initial string is SSO
            za::String s1 = "abcdefgh";
            CHECK(s1.isSso());

            // Assign a substring of itself: s1 should become "cdef"
            s1 = za::StringView(s1.data() + 2, 4);

            CHECK(s1 == "cdef");

            // Case 2: Initial string is on the heap
            za::String s2 = makeLongString("This is a long string for testing overlap");
            CHECK(!s2.isSso());

            // Assign a substring of itself: s2 should become "long"
            s2 = za::StringView(s2.data() + 10, 4);
            CHECK(s2 == "long");
        }
    }

    SECTION("Self-Append")
    {
        SECTION("s += s with SSO string (transitioning to heap)")
        {
            // Choose a size that fits SSO but when doubled exceeds maxSsoSize.
            const auto len = (maxSsoSize / 2) + 1;
            za::String s(longStringLiteral, len);
            CHECK(s.size() == len);
            CHECK(s.isSso());

            const za::String expected = s + za::StringView(s);
            s += s; // Self-append, should trigger grow() and use-after-free bug

            CHECK(s.size() == len * 2);
            CHECK(s == expected);
            CHECK(!s.isSso());
        }

        SECTION("s += s with Heap string (forcing reallocation)")
        {
            // Create a heap string with little to no spare capacity
            za::String original = "a long string that will force a reallocation upon self-append";
            za::String s        = original;
            CHECK(!s.isSso());
            CHECK(s.capacity() - s.size() < s.size()); // Ensure it *must* reallocate

            s += s; // Self-append that reallocates heap buffer

            za::String expected = original;
            expected.append(original);

            CHECK(s == expected);
            CHECK(!s.isSso());
        }

        SECTION("s += s with Heap string (with enough capacity)")
        {
            za::String s = "pre-reserved";
            // Reserve enough space so no reallocation occurs
            s.reserve(s.size() * 2 + 10);
            CHECK(!s.isSso());
            const char* originalData = s.data();

            s += s; // Self-append in-place

            CHECK(s == "pre-reservedpre-reserved");
            CHECK(s.data() == originalData); // Must not reallocate
        }

        SECTION("Appending a substring of itself")
        {
            za::String s = "12345";
            CHECK(s.isSso());

            // Append a view of its own beginning
            s.append(za::StringView(s.data(), 3)); // Append "123"

            CHECK(s == "12345123");
        }
    }

    SECTION("Self-Insert")
    {
        SECTION("insert with overlapping source and no reallocation")
        {
            za::String s("abcdef");
            s.reserve(12);

            const char* const originalData = s.data();
            s.insert(2, s.cStr() + 1);

            CHECK(s.data() == originalData);
            CHECK(s == "abbcdefcdef");
        }

        SECTION("insert with overlapping source and reallocation")
        {
            // Build a string that fits SSO but after insert(3, cStr()+2)
            // produces a result of length (2*len - 2) that exceeds maxSsoSize.
            const auto len = (maxSsoSize / 2) + 2; // guarantees 2*len - 2 > maxSsoSize
            za::String s(longStringLiteral, len);
            CHECK(s.isSso());

            // Build the expected result: s[0..3] + s[2..] + s[3..]
            za::String expected;
            expected.append(za::StringView(s).substrByPosLen(0, 3));
            expected.append(za::StringView(s).substrByPosLen(2));
            expected.append(za::StringView(s).substrByPosLen(3));

            s.insert(3, s.cStr() + 2);

            CHECK(!s.isSso());
            CHECK(s == expected);
        }
    }

    SECTION("Resize")
    {
        za::String s = "Hello";
        CHECK(s.isSso());
        CHECK(s.size() == 5);

        s.resize(10, 'X');
        CHECK(s.size() == 10);
        CHECK(s == "HelloXXXXX");
        CHECK(s.isSso());

        s.resize(3);
        CHECK(s.size() == 3);
        CHECK(s == "Hel");
        CHECK(s.isSso());

        s.resize(50);
        CHECK(s.size() == 50);
        CHECK(s[0] == 'H');
        CHECK(s[1] == 'e');
        CHECK(s[2] == 'l');

        for (za::SizeT i = 3; i < 50; ++i)
            CHECK(s[i] == '\0');

        CHECK(!s.isSso());
    }

    SECTION("ResizeAndOverwrite")
    {
        SECTION("Grow from SSO into heap, op fills the whole buffer")
        {
            za::String s = "abc";
            CHECK(s.isSso());
            CHECK(s.size() == 3);

            constexpr za::SizeT requested = 100u; // > maxSsoSize
            s.resizeAndOverwrite(requested,
                                 [](char* buf, za::SizeT n) -> za::SizeT
            {
                for (za::SizeT i = 0; i < n; ++i)
                    buf[i] = static_cast<char>('a' + (i % 26));
                return n;
            });

            CHECK(s.size() == requested);
            CHECK(!s.isSso());
            CHECK(s[0] == 'a');
            CHECK(s[1] == 'b');
            CHECK(s[25] == 'z');
            CHECK(s[26] == 'a');
            // Must always be null-terminated for cStr() callers.
            CHECK(s.cStr()[requested] == '\0');
        }

        SECTION("Returning a smaller size truncates")
        {
            za::String s;
            s.resizeAndOverwrite(20u,
                                 [](char* buf, za::SizeT) -> za::SizeT
            {
                buf[0] = 'h';
                buf[1] = 'i';
                return 2u;
            });

            CHECK(s.size() == 2u);
            CHECK(s == "hi");
            CHECK(s.cStr()[2] == '\0');
        }

        SECTION("Returning zero clears the string")
        {
            za::String s = "previous";
            s.resizeAndOverwrite(50u, [](char*, za::SizeT) -> za::SizeT { return 0u; });

            CHECK(s.empty());
            CHECK(s.size() == 0u);
            CHECK(s.cStr()[0] == '\0');
        }

        SECTION("Existing prefix is preserved up to min(size(), newSize)")
        {
            // Matches std::string::resize_and_overwrite semantics: bytes up to
            // the lesser of old and new size keep their values when op runs.
            za::String s = "Hello";
            CHECK(s.isSso());

            s.resizeAndOverwrite(8u,
                                 [](char* buf, za::SizeT n) -> za::SizeT
            {
                // First 5 bytes are still "Hello"; only fill the new tail.
                CHECK(buf[0] == 'H');
                CHECK(buf[1] == 'e');
                CHECK(buf[2] == 'l');
                CHECK(buf[3] == 'l');
                CHECK(buf[4] == 'o');
                buf[5] = '!';
                buf[6] = '!';
                buf[7] = '!';
                return n;
            });

            CHECK(s == "Hello!!!");
        }

        SECTION("Shrinking via newSize < size() keeps prefix and truncates")
        {
            za::String s = "abcdefghij";
            s.resizeAndOverwrite(3u,
                                 [](char* buf, za::SizeT n) -> za::SizeT
            {
                CHECK(buf[0] == 'a');
                CHECK(buf[1] == 'b');
                CHECK(buf[2] == 'c');
                return n;
            });

            CHECK(s.size() == 3u);
            CHECK(s == "abc");
        }

        SECTION("newSize == 0 with empty string is a no-op")
        {
            za::String s;
            s.resizeAndOverwrite(0u, [](char*, za::SizeT n) -> za::SizeT { return n; });

            CHECK(s.empty());
            CHECK(s.cStr()[0] == '\0');
        }
    }

    SECTION("Replace")
    {
        SECTION("Same-length replacement (no shift)")
        {
            za::String s = "Hello, world!";
            s.replace(7, 5, "WORLD");
            CHECK(s == "Hello, WORLD!");
            CHECK(s.size() == 13u);
        }

        SECTION("Shorter replacement (tail shifts left)")
        {
            za::String s = "Hello, world!";
            s.replace(7, 5, "you");
            CHECK(s == "Hello, you!");
            CHECK(s.size() == 11u);
        }

        SECTION("Longer replacement (tail shifts right)")
        {
            za::String s = "Hello, world!";
            s.replace(7, 5, "everybody");
            CHECK(s == "Hello, everybody!");
            CHECK(s.size() == 17u);
        }

        SECTION("Empty replacement acts as erase")
        {
            za::String s = "Hello, world!";
            s.replace(5, 7, "");
            CHECK(s == "Hello!");
            CHECK(s.size() == 6u);
        }

        SECTION("Zero count acts as insert")
        {
            za::String s = "Hello!";
            s.replace(5, 0, ", world");
            CHECK(s == "Hello, world!");
            CHECK(s.size() == 13u);
        }

        SECTION("Replace at beginning")
        {
            za::String s = "abcdef";
            s.replace(0, 3, "XYZW");
            CHECK(s == "XYZWdef");
        }

        SECTION("Replace at end (pos == size)")
        {
            za::String s = "abc";
            s.replace(3, 0, "def");
            CHECK(s == "abcdef");
        }

        SECTION("count == nPos clamps to end")
        {
            za::String s = "Hello, world!";
            s.replace(7, za::String::nPos, "EVERYONE");
            CHECK(s == "Hello, EVERYONE");
        }

        SECTION("count past end clamps to end")
        {
            za::String s = "abcdef";
            s.replace(2, 100u, "XYZ");
            CHECK(s == "abXYZ");
        }

        SECTION("Replace whole string")
        {
            za::String s = "abc";
            s.replace(0, s.size(), "abcdefghij");
            CHECK(s == "abcdefghij");
        }

        SECTION("Replace then content fits in original capacity (no realloc)")
        {
            za::String s = "Hello, world!";
            s.reserve(64);
            const char* const originalData = s.data();

            s.replace(7, 5, "everybody");

            CHECK(s == "Hello, everybody!");
            CHECK(s.data() == originalData); // no reallocation
        }

        SECTION("Replace forces growth out of SSO")
        {
            za::String s = "abc";
            CHECK(s.isSso());

            s.replace(0, s.size(), longStringLiteral);

            CHECK(!s.isSso());
            CHECK(s == longStringLiteral);
        }

        SECTION("Self-aliasing replacement (substring of itself)")
        {
            // Replace [7..12) ("world") with the substring "Hello" already
            // sitting at [0..5). The implementation must copy the source first
            // before shifting the tail, otherwise the read would race the move.
            za::String s = "Hello, world!";
            s.replace(7, 5, za::StringView{s}.substrByPosLen(0, 5));

            CHECK(s == "Hello, Hello!");
        }

        SECTION("Self-aliasing replacement that overlaps the replaced range")
        {
            // Source overlaps the destination: replace [3..6) with the
            // substring at [4..7). Self-aliasing path must copy first.
            za::String s = "abcdefghij";
            s.replace(3, 3, za::StringView{s}.substrByPosLen(4, 3));

            CHECK(s == "abcefgghij");
        }

        SECTION("Heap string self-aliasing forcing reallocation")
        {
            // Long string that's already on the heap; replace with a tail of
            // itself that, after substitution, exceeds current capacity.
            za::String s(longStringLiteral);
            CHECK(!s.isSso());

            const auto       tailView = za::StringView{s}.substrByPosLen(s.size() / 2);
            const za::String expected = za::String{longStringLiteral} + za::String{tailView};

            // Replace zero chars at end with our own tail -> doubles the data.
            s.replace(s.size(), 0, tailView);

            CHECK(s == expected);
        }

        SECTION("Replace with empty replacement on empty string is a no-op")
        {
            za::String s;
            s.replace(0, 0, "");
            CHECK(s.empty());
        }

        SECTION("Replace preserves null terminator")
        {
            za::String s = "Hello, world!";
            s.replace(7, 5, "you");
            // `cStr()` must still produce a valid C-string at the new size.
            CHECK(s.cStr()[s.size()] == '\0');
        }

        SECTION("Replace accepts String, StringView, const char* via implicit conversions")
        {
            za::String s1 = "abc";
            za::String s2 = "abc";
            za::String s3 = "abc";

            const za::String     replString = "XX";
            const za::StringView replView   = "YY";

            s1.replace(1, 1, replString);
            s2.replace(1, 1, replView);
            s3.replace(1, 1, "ZZ");

            CHECK(s1 == "aXXc");
            CHECK(s2 == "aYYc");
            CHECK(s3 == "aZZc");
        }
    }

    SECTION("ReplaceFirstOccurrence")
    {
        SECTION("Found")
        {
            za::String s = "Hello, world!";
            CHECK(s.replaceFirstOccurrence("world", "everybody"));
            CHECK(s == "Hello, everybody!");
        }

        SECTION("Not found leaves string unchanged")
        {
            za::String s = "Hello, world!";
            CHECK_FALSE(s.replaceFirstOccurrence("xyz", "ABC"));
            CHECK(s == "Hello, world!");
        }

        SECTION("Only first match is replaced")
        {
            za::String s = "abc abc abc";
            CHECK(s.replaceFirstOccurrence("abc", "XYZ"));
            CHECK(s == "XYZ abc abc");
        }

        SECTION("Empty target returns false and leaves string unchanged")
        {
            za::String s = "Hello";
            CHECK_FALSE(s.replaceFirstOccurrence("", "X"));
            CHECK(s == "Hello");
        }

        SECTION("Empty replacement acts as erase")
        {
            za::String s = "Hello, world!";
            CHECK(s.replaceFirstOccurrence(", world", ""));
            CHECK(s == "Hello!");
        }

        SECTION("Replace at very start")
        {
            za::String s = "abcabc";
            CHECK(s.replaceFirstOccurrence("abc", "XX"));
            CHECK(s == "XXabc");
        }

        SECTION("Replace at very end")
        {
            za::String s = "abcabc";
            CHECK(s.replaceFirstOccurrence("bc", "XYZ"));
            CHECK(s == "aXYZabc"); // first "bc" at index 1
        }

        SECTION("Empty target on empty string returns false")
        {
            za::String s;
            CHECK_FALSE(s.replaceFirstOccurrence("", "X"));
            CHECK(s.empty());
        }

        SECTION("Self-aliasing target view (substring of self)")
        {
            za::String s   = "abcdef";
            const auto sub = za::StringView{s}.substrByPosLen(2, 2); // "cd"
            CHECK(s.replaceFirstOccurrence(sub, "XYZ"));
            CHECK(s == "abXYZef");
        }
    }

    SECTION("ReplaceAllOccurrences")
    {
        SECTION("Multiple matches")
        {
            za::String s = "abc abc abc";
            CHECK(s.replaceAllOccurrences("abc", "X") == 3u);
            CHECK(s == "X X X");
        }

        SECTION("No matches leaves string unchanged and returns 0")
        {
            za::String s = "Hello, world!";
            CHECK(s.replaceAllOccurrences("xyz", "ABC") == 0u);
            CHECK(s == "Hello, world!");
        }

        SECTION("Single match returns 1")
        {
            za::String s = "Hello, world!";
            CHECK(s.replaceAllOccurrences("world", "everybody") == 1u);
            CHECK(s == "Hello, everybody!");
        }

        SECTION("Empty target returns 0 and leaves string unchanged")
        {
            za::String s = "Hello";
            CHECK(s.replaceAllOccurrences("", "X") == 0u);
            CHECK(s == "Hello");
        }

        SECTION("Empty replacement removes all occurrences")
        {
            za::String s = "abXYZabXYZab";
            CHECK(s.replaceAllOccurrences("XYZ", "") == 2u);
            CHECK(s == "ababab");
        }

        SECTION("Adjacent matches all replaced")
        {
            za::String s = "aaaa";
            // "aa" "aa" matches at 0 and 2; both get replaced.
            CHECK(s.replaceAllOccurrences("aa", "X") == 2u);
            CHECK(s == "XX");
        }

        SECTION("Replacement contains target -- does not infinite-loop")
        {
            za::String s = "abc";
            // "a" -> "aa": after replace, advance past it so we don't re-match.
            CHECK(s.replaceAllOccurrences("a", "aa") == 1u);
            CHECK(s == "aabc");
        }

        SECTION("Replacement equals target is a no-op (count is reported)")
        {
            za::String s = "abcabc";
            CHECK(s.replaceAllOccurrences("abc", "abc") == 2u);
            CHECK(s == "abcabc");
        }

        SECTION("Overlapping target avoids re-matching its own output")
        {
            // Replacement "a" at every 2-char "aa": "aaaa" -> "aaa" -> "aa".
            // We use the non-overlapping convention: jump past each replacement,
            // so positions are 0 and 2 (after first replace -> "aaa"), then 1
            // (after second replace -> "aa"). Two replacements total.
            za::String s = "aaaa";
            CHECK(s.replaceAllOccurrences("aa", "a") == 2u);
            CHECK(s == "aa");
        }

        SECTION("Replacement causes growth out of SSO")
        {
            za::String s = "aXa";
            CHECK(s.isSso());

            // Replace each 'X' with the long literal -> result definitely heap.
            const auto count = s.replaceAllOccurrences("X", longStringLiteral);
            CHECK(count == 1u);
            CHECK(!s.isSso());

            za::String expected = "a";
            expected.append(longStringLiteral);
            expected.append("a");
            CHECK(s == expected);
        }

        SECTION("Self-aliasing target view (substring of self)")
        {
            za::String s = "abcabcabc";
            // Take a view of the first "abc" and replace all occurrences of
            // it with "X". The aliasing-detection path must copy the target
            // before the buffer reallocates.
            const auto needle = za::StringView{s}.substrByPosLen(0, 3);
            CHECK(s.replaceAllOccurrences(needle, "X") == 3u);
            CHECK(s == "XXX");
        }

        SECTION("Self-aliasing replacement view (substring of self)")
        {
            za::String s = "ab_cd";
            // Replace "_" with everything from index 0 to 2 ("ab"). View is
            // taken before mutation; aliasing-detection path must copy it.
            const auto repl = za::StringView{s}.substrByPosLen(0, 2);
            CHECK(s.replaceAllOccurrences("_", repl) == 1u);
            CHECK(s ==
                  "abab"
                  "cd"); // "ab" + "ab" + "cd"
        }

        SECTION("Heap string with many matches forces multiple shifts")
        {
            // Build a long string of repeated tokens, then replace them all.
            za::String s;
            for (int i = 0; i < 100; ++i)
                s.append("foo,");

            const auto count = s.replaceAllOccurrences("foo", "bar");
            CHECK(count == 100u);

            za::String expected;
            for (int i = 0; i < 100; ++i)
                expected.append("bar,");
            CHECK(s == expected);
        }

        SECTION("Empty replacement on empty string is a no-op")
        {
            za::String s;
            CHECK(s.replaceAllOccurrences("x", "") == 0u);
            CHECK(s.empty());
        }
    }

    SECTION("StringView bridge methods")
    {
        SECTION("forSplits is bridged from StringView (B)")
        {
            za::String s = "alpha,beta,gamma";

            za::SizeT count = 0u;
            s.forSplits(',',
                        [&](za::StringView seg)
            {
                ++count;
                if (count == 1u)
                    CHECK(seg == "alpha");
                if (count == 2u)
                    CHECK(seg == "beta");
                if (count == 3u)
                    CHECK(seg == "gamma");
            });
            CHECK(count == 3u);
        }

        SECTION("forSplits with StringView splitter is bridged from StringView (B)")
        {
            za::String s = "foo::bar::baz";

            za::SizeT count = 0u;
            s.forSplits("::",
                        [&](za::StringView seg)
            {
                ++count;
                if (count == 1u)
                    CHECK(seg == "foo");
                if (count == 2u)
                    CHECK(seg == "bar");
                if (count == 3u)
                    CHECK(seg == "baz");
            });
            CHECK(count == 3u);
        }

        SECTION("forLines side-effects are not elided (no gnu::pure on bridge) (C)")
        {
            // The bridge macro previously had `gnu::pure`, which would have
            // allowed the optimizer to drop calls whose return value isn't used.
            // `forLines` returns void and runs side-effecting user code, so
            // any elision would be observable as a missed counter increment.
            za::String s    = "a\nb\nc\nd";
            za::SizeT  hits = 0u;
            s.forLines([&](za::StringView) { ++hits; });
            CHECK(hits == 4u);
        }
    }

    SECTION("erase at end-of-string is a no-op (E)")
    {
        // `std::string::erase(size())` is allowed and is a no-op (count
        // clamps to 0). Previously this asserted; loosened to `<=`.
        za::String s = "Hello";

        s.erase(s.size());
        CHECK(s == "Hello");

        s.erase(s.size(), 0u);
        CHECK(s == "Hello");

        s.erase(s.size(), za::String::nPos);
        CHECK(s == "Hello");

        // Erasing the entire string still works at index 0.
        s.erase(0u);
        CHECK(s.empty());

        // Erasing at the (now zero) size of an empty string is also valid.
        s.erase(0u);
        CHECK(s.empty());
    }

    SECTION("operator== distinguishes embedded NUL bytes (A)")
    {
        // String comparison goes through StringView's `==` (memcmp), so it
        // must distinguish strings that differ AFTER an embedded NUL.
        const char raw1[] = {'a', '\0', 'b'};
        const char raw2[] = {'a', '\0', 'c'};

        const za::String s1{raw1, 3};
        const za::String s2{raw2, 3};

        CHECK_FALSE(s1 == s2);
        CHECK(s1 != s2);
        CHECK(s1 == s1);
    }

    SECTION("operator+ lvalue overloads")
    {
        const za::String a = "foo";
        const za::String b = "bar";

        CHECK(a + b == "foobar");
        CHECK(a + 'x' == "foox");
        CHECK('x' + a == "xfoo");
        CHECK(a + "bar" == "foobar");
        CHECK("bar" + a == "barfoo");
        CHECK(a + za::StringView{"bar"} == "foobar");
        CHECK(za::StringView{"bar"} + a == "barfoo");

        // Source operands unaffected.
        CHECK(a == "foo");
        CHECK(b == "bar");
    }

    SECTION("operator+ rvalue-lhs reuses the buffer (no realloc)")
    {
        // Force the lhs onto the heap with capacity to spare so the
        // append does not reallocate. The result should share the same
        // heap pointer, proving that lhs was moved into the result and
        // appended in-place rather than the impl allocating a fresh buffer.
        za::String lhs;
        lhs.reserve(64u);
        lhs = "abcdefghijklmnopqrstuvwxyz"; // > SSO max, on heap
        REQUIRE(lhs.capacity() >= 28u);

        const char* const lhsPtrBefore = lhs.data();

        const za::String result = static_cast<za::String&&>(lhs) + za::String{"!!"};
        CHECK(result == "abcdefghijklmnopqrstuvwxyz!!");
        CHECK(result.data() == lhsPtrBefore);
    }

    SECTION("operator+ rvalue-lhs overloads -- all argument types")
    {
        auto makeHeap = []
        {
            za::String s;
            s.reserve(64u);
            s = "abcdefghijklmnopqrstuvwxyz";
            return s;
        };

        CHECK(makeHeap() + za::String{"!"} == "abcdefghijklmnopqrstuvwxyz!");
        CHECK(makeHeap() + '!' == "abcdefghijklmnopqrstuvwxyz!");
        CHECK(makeHeap() + "!!" == "abcdefghijklmnopqrstuvwxyz!!");
        CHECK(makeHeap() + za::StringView{"!!!"} == "abcdefghijklmnopqrstuvwxyz!!!");
    }

    SECTION("operator+ chained expressions feed the rvalue overload")
    {
        // `a + b + c + d` -- after the first `+`, subsequent operands see
        // an rvalue lhs and reuse the buffer instead of reallocating.
        // The buffer-reuse property is verified in the prior section;
        // here we only check the result.
        const za::String a = "Hello, ";
        const za::String b = "beautiful ";
        const za::String c = "world";
        const za::String d = "!";

        CHECK(a + b + c + d == "Hello, beautiful world!");
    }

    SECTION("appendFmt (FmtAppendMixin) on empty string")
    {
        za::String s;
        s.appendFmt("hello");
        CHECK(s == "hello");
    }

    SECTION("appendFmt with single placeholder")
    {
        za::String s;
        s.appendFmt("answer = {}", 42);
        CHECK(s == "answer = 42");
    }

    SECTION("appendFmt with multiple heterogeneous placeholders")
    {
        za::String s;
        s.appendFmt("{}-{}-{}", 1, 'x', za::StringView{"end"});
        CHECK(s == "1-x-end");
    }

    SECTION("appendFmt with width / precision / type-tag spec")
    {
        za::String s;
        s.appendFmt("[{:5}][{:x}][{:.3}]", 7, 255, 3.14159);
        CHECK(s == "[    7][ff][3.142]");
    }

    SECTION("appendFmt is genuinely appending (not replacing)")
    {
        za::String s = "prefix:";
        s.appendFmt(" v={}", 7);
        CHECK(s == "prefix: v=7");

        // Second appendFmt continues onto the existing buffer.
        s.appendFmt(",{}", 11);
        CHECK(s == "prefix: v=7,11");
    }

    SECTION("appendFmt grows an SSO buffer onto the heap when needed")
    {
        za::String s = "abc";
        // Long format expansion forces growth beyond SSO.
        s.appendFmt("-{}-{}-{}-{}-{}-{}-{}-{}", "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta");
        CHECK(s == "abc-alpha-beta-gamma-delta-epsilon-zeta-eta-theta");
    }

    SECTION("appendArg appends a single value with no spec")
    {
        za::String s = "n=";
        s.appendArg(42);
        CHECK(s == "n=42");

        s.appendArg(',' /*char*/);
        CHECK(s == "n=42,");

        s.appendArg(za::StringView{"tail"});
        CHECK(s == "n=42,tail");
    }

    SECTION("appendArg with float renders default precision")
    {
        za::String s;
        s.appendArg(3.14159);
        // `defaultFloatPrecision == 6` ({:.6f}).
        CHECK(s == "3.141590");
    }
}
