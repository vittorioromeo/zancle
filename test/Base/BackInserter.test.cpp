#include "Tst/Tst.hpp"

#include "Zancle/Container/BackInserter.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Algorithm/Copy.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Base/PtrDiffT.hpp"

#include "Zancle/Trait/DeclVal.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsSame.hpp"


namespace
{
namespace BackInserterTest // for unity builds
{
////////////////////////////////////////////////////////////
/// Counts conversions from `const char*` and moves/copies.
////////////////////////////////////////////////////////////
struct Tracked
{
    static inline int conversions{};
    static inline int copies{};
    static inline int moves{};

    const char* str;

    /* implicit */ Tracked(const char* s) : str(s)
    {
        ++conversions;
    }

    Tracked(const Tracked& rhs) : str(rhs.str)
    {
        ++copies;
    }

    Tracked(Tracked&& rhs) noexcept : str(rhs.str)
    {
        ++moves;
    }

    Tracked& operator=(const Tracked&) = default;
    Tracked& operator=(Tracked&&)      = default;
    ~Tracked()                         = default;

    static void resetCounters()
    {
        conversions = copies = moves = 0;
    }
};


////////////////////////////////////////////////////////////
/// Minimal sink without a `value_type` typedef.
////////////////////////////////////////////////////////////
struct CharSink
{
    char buf[8]{};
    int  count{};

    void pushBack(const char c)
    {
        buf[count++] = c;
    }
};


////////////////////////////////////////////////////////////
using VecIntInserter = za::BackInserter<za::Vector<int>>;

static_assert(za::isSame<VecIntInserter::container_type, za::Vector<int>>);
static_assert(za::isSame<VecIntInserter::value_type, void>);
static_assert(za::isSame<VecIntInserter::difference_type, za::PtrDiffT>);

// Copy-assignment of the inserter itself is not hijacked by the forwarding `operator=`.
static_assert(ZA_IS_COPY_ASSIGNABLE(VecIntInserter));
static_assert(za::isSame<decltype(za::declVal<VecIntInserter&>() = za::declVal<const VecIntInserter&>()), VecIntInserter&>);

// Works with sinks that do not expose `value_type`.
static_assert(za::isSame<za::BackInserter<za::Utf8String>::container_type, za::Utf8String>);
static_assert(za::isSame<za::BackInserter<CharSink>::value_type, void>);

} // namespace BackInserterTest
} // namespace


TEST_CASE("[Base] Container/BackInserter.hpp")
{
    using namespace BackInserterTest;

    SECTION("Back Inserter")
    {
        const int       values[]{0, 1, 2, 3};
        za::Vector<int> target{-1};

        za::copy(values, values + 4, za::BackInserter{target});

        REQUIRE(target.size() == 5u);
        CHECK(target[0] == -1);
        CHECK(target[1] == 0);
        CHECK(target[2] == 1);
        CHECK(target[3] == 2);
        CHECK(target[4] == 3);
    }

    SECTION("Iterator operations return the inserter itself")
    {
        za::Vector<int>  target;
        za::BackInserter it{target};

        za::BackInserter<za::Vector<int>>& deref  = *it;
        za::BackInserter<za::Vector<int>>& preInc = ++it;

        CHECK(&deref == &it);
        CHECK(&preInc == &it);

        *it++ = 1;
        *it++ = 2;

        REQUIRE(target.size() == 2u);
        CHECK(target[0] == 1);
        CHECK(target[1] == 2);
    }

    SECTION("Copy-assigning the inserter rebinds it")
    {
        za::Vector<int> a;
        za::Vector<int> b;

        za::BackInserter itA{a};
        za::BackInserter itB{b};

        itA  = itB; // must not call `a.pushBack(itB)`
        *itA = 42;

        CHECK(a.empty());
        REQUIRE(b.size() == 1u);
        CHECK(b[0] == 42);
    }

    SECTION("Rvalue of a move-only type")
    {
        za::Vector<za::UniquePtr<int>> target;
        za::BackInserter               it{target};

        auto p = za::makeUnique<int>(10);
        *it    = static_cast<za::UniquePtr<int>&&>(p);
        *it    = za::makeUnique<int>(20);

        CHECK(p == nullptr);
        REQUIRE(target.size() == 2u);
        CHECK(*target[0] == 10);
        CHECK(*target[1] == 20);
    }

    SECTION("Lvalues are copied")
    {
        za::Vector<za::String> target;
        za::BackInserter       it{target};

        const za::String s{"hello"};
        *it = s;

        CHECK(s == "hello");
        REQUIRE(target.size() == 1u);
        CHECK(target[0] == "hello");
    }

    SECTION("Convertible type into `Vector<String>`")
    {
        za::Vector<za::String> target;
        za::BackInserter       it{target};

        const char* cStr = "abc";
        *it              = cStr;
        *it              = "def";

        REQUIRE(target.size() == 2u);
        CHECK(target[0] == "abc");
        CHECK(target[1] == "def");
    }

    SECTION("Convertible type is forwarded without an intermediate temporary")
    {
        za::Vector<Tracked> target;
        target.reserve(4u); // avoid reallocation moves

        Tracked::resetCounters();

        za::BackInserter it{target};
        *it = "abc";

        REQUIRE(target.size() == 1u);
        CHECK(target[0].str[0] == 'a');
        CHECK(Tracked::conversions == 1);
        CHECK(Tracked::copies == 0);
        CHECK(Tracked::moves == 0);
    }

    SECTION("`Utf8String` sink")
    {
        za::Utf8String   target;
        za::BackInserter it{target};

        *it++ = 'a';
        *it++ = 'b';
        *it++ = 'c';

        CHECK(target == "abc");

        const char more[]{'d', 'e'};
        za::copy(more, more + 2, za::BackInserter{target});

        CHECK(target == "abcde");
    }

    SECTION("Custom sink without `value_type`")
    {
        CharSink         sink;
        za::BackInserter it{sink};

        *it = 'x';
        *it = 'y';

        REQUIRE(sink.count == 2);
        CHECK(sink.buf[0] == 'x');
        CHECK(sink.buf[1] == 'y');
    }
}
