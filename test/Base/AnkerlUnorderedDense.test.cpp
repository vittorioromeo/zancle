#include "Tst/Tst.hpp"

#include "Zancle/Container/AnkerlUnorderedDense.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/StringView.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsTriviallyRelocatable.hpp"


namespace
{
namespace AnkerlUnorderedDenseTest // unity build
{
////////////////////////////////////////////////////////////
using IntMap = ankerl::unordered_dense::map<int, int>;
using IntSet = ankerl::unordered_dense::set<int>;


////////////////////////////////////////////////////////////
struct Base
{
};

struct Derived : Base
{
};


////////////////////////////////////////////////////////////
struct TwoArgs
{
    TwoArgs(const int a, const int b) : sum{a + b}
    {
    }

    int sum;
};


////////////////////////////////////////////////////////////
struct NonRelocatable
{
    NonRelocatable() = default;

    NonRelocatable(NonRelocatable&&) noexcept
    {
    }

    NonRelocatable& operator=(NonRelocatable&&) noexcept
    {
        return *this;
    }
};


////////////////////////////////////////////////////////////
struct Padded
{
    char c;
    int  i;
};


////////////////////////////////////////////////////////////
template <typename Map, typename Arg>
concept CanTryEmplace = requires(Map& m, Arg&& arg) { m.try_emplace(1, static_cast<Arg&&>(arg)); };

template <typename Map, typename Arg>
concept CanInsertOrAssign = requires(Map& m, Arg&& arg) { m.insert_or_assign(1, static_cast<Arg&&>(arg)); };


////////////////////////////////////////////////////////////
template <typename T>
constexpr bool byteHashable = ankerl::unordered_dense::detail::byte_hashable_range<T>;


////////////////////////////////////////////////////////////
void checkUsable(IntMap& m)
{
    const IntMap defaultConstructed;

    CHECK(m.empty());
    CHECK(m.size() == 0u);
    CHECK(m.bucket_count() == defaultConstructed.bucket_count());
    CHECK(m.find(5) == m.end());
    CHECK(!m.contains(5));
    CHECK(m.erase(5) == 0u);

    m[5] = 50;
    CHECK(m.try_emplace(6, 60).second);
    CHECK(m.size() == 2u);
    CHECK(m.find(5)->second == 50);
    CHECK(m.at(6) == 60);

    // Grow past the initial bucket count.
    for (int i = 100; i < 200; ++i)
        m[i] = i;

    CHECK(m.size() == 102u);
    CHECK(m.erase(5) == 1u);
    CHECK(!m.contains(5));

    const IntMap copy = m; // NOLINT(performance-unnecessary-copy-initialization)
    CHECK(static_cast<bool>(copy == m));
    CHECK(copy.size() == 101u);
    CHECK(copy.at(150) == 150);
}

} // namespace AnkerlUnorderedDenseTest
} // namespace


TEST_CASE("[Base] za::ankerl::map moved-from state")
{
    using namespace AnkerlUnorderedDenseTest;

    SECTION("Move constructor source is usable")
    {
        IntMap a;
        for (int i = 0; i < 10; ++i)
            a[i] = i * 10;

        IntMap b = ZA_MOVE(a);
        CHECK(b.size() == 10u);
        CHECK(b.at(3) == 30);

        checkUsable(a); // NOLINT(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
    }

    SECTION("Move assignment source is usable")
    {
        IntMap a;
        for (int i = 0; i < 10; ++i)
            a[i] = i * 10;

        IntMap b;
        b[1000] = 1;
        b       = ZA_MOVE(a);
        CHECK(b.size() == 10u);
        CHECK(b.at(3) == 30);
        CHECK(!b.contains(1000));

        checkUsable(a); // NOLINT(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
    }

    SECTION("Moved-from source can be moved again and copy-assigned")
    {
        IntMap a;
        a[1] = 1;

        IntMap b = ZA_MOVE(a);
        IntMap c = ZA_MOVE(a); // NOLINT(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
        CHECK(c.empty());

        a = b;
        CHECK(a.size() == 1u);
        CHECK(a.at(1) == 1);
    }

    SECTION("Moved-from set is usable")
    {
        IntSet a{1, 2, 3};
        IntSet b = ZA_MOVE(a);
        CHECK(b.size() == 3u);

        CHECK(a.empty()); // NOLINT(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
        CHECK(a.insert(4).second);
        CHECK(a.contains(4));
        CHECK(!a.contains(1));
    }
}


TEST_CASE("[Base] za::ankerl::map of maps")
{
    using Inner = ankerl::unordered_dense::map<int, za::String>;
    using Outer = ankerl::unordered_dense::map<int, Inner>;

    STATIC_CHECK(za::isTriviallyRelocatable<Inner>);
    STATIC_CHECK(za::isTriviallyRelocatable<Outer>);

    SECTION("Outer map growth keeps inner maps intact")
    {
        Outer outer;
        for (int i = 0; i < 200; ++i)
            for (int j = 0; j < 5; ++j)
                outer[i][j] = "value";

        REQUIRE(outer.size() == 200u);
        for (int i = 0; i < 200; ++i)
        {
            const auto* const it = outer.find(i);
            REQUIRE(it != outer.end());
            REQUIRE(it->second.size() == 5u);
            CHECK(it->second.at(4) == "value");
        }

        // Inner maps stay usable after being moved around by erase (swap-with-back).
        for (int i = 0; i < 200; i += 2)
            CHECK(outer.erase(i) == 1u);

        for (int i = 1; i < 200; i += 2)
        {
            outer[i][100] = "new";
            CHECK(outer[i].size() == 6u);
            CHECK(outer[i].at(100) == "new");
        }
    }

    SECTION("Vector of maps growth")
    {
        za::Vector<Inner> maps;
        for (int i = 0; i < 100; ++i)
            maps.emplaceBack()[i] = "x";

        for (int i = 0; i < 100; ++i)
        {
            REQUIRE(maps[static_cast<za::SizeT>(i)].size() == 1u);
            CHECK(maps[static_cast<za::SizeT>(i)].at(i) == "x");
        }
    }
}


TEST_CASE("[Base] za::ankerl::map value construction")
{
    using namespace AnkerlUnorderedDenseTest;

    SECTION("Cast-only conversions are rejected")
    {
        using PtrMap     = ankerl::unordered_dense::map<int, int*>;
        using DerivedMap = ankerl::unordered_dense::map<int, Derived*>;

        STATIC_CHECK(!CanTryEmplace<PtrMap, long long>);
        STATIC_CHECK(!CanInsertOrAssign<PtrMap, long long>);
        STATIC_CHECK(!CanTryEmplace<DerivedMap, Base*>);
        STATIC_CHECK(!CanInsertOrAssign<DerivedMap, Base*>);
        STATIC_CHECK(!CanTryEmplace<IntMap, const char*>);
    }

    SECTION("Regular conversions are accepted")
    {
        using PtrMap  = ankerl::unordered_dense::map<int, int*>;
        using BaseMap = ankerl::unordered_dense::map<int, Base*>;

        STATIC_CHECK(CanTryEmplace<PtrMap, int*>);
        STATIC_CHECK(CanTryEmplace<PtrMap, decltype(nullptr)>);
        STATIC_CHECK(CanTryEmplace<BaseMap, Derived*>);
        STATIC_CHECK(CanInsertOrAssign<BaseMap, Derived*>);
        STATIC_CHECK(CanTryEmplace<IntMap, long long>);
        STATIC_CHECK(CanTryEmplace<ankerl::unordered_dense::map<int, za::String>, const char*>);

        Derived d;
        BaseMap m;
        CHECK(m.try_emplace(1, &d).second);
        CHECK(m.at(1) == &d);
        CHECK(!m.insert_or_assign(1, nullptr).second);
        CHECK(m.at(1) == nullptr);
    }

    SECTION("No arguments value-initializes")
    {
        IntMap m;
        CHECK(m.try_emplace(1).first->second == 0);
        CHECK(m[2] == 0);

        ankerl::unordered_dense::map<int, int*> pm;
        CHECK(pm[1] == nullptr);
    }

    SECTION("Multiple arguments construct in place")
    {
        ankerl::unordered_dense::map<int, TwoArgs> m;
        CHECK(m.try_emplace(1, 2, 3).first->second.sum == 5);

        ankerl::unordered_dense::map<int, za::String> sm;
        CHECK(sm.try_emplace(1, "abcdef", za::SizeT{3}).first->second == "abc");
    }
}


TEST_CASE("[Base] za::ankerl::hash byte-hashable ranges")
{
    using namespace AnkerlUnorderedDenseTest;

    SECTION("Constraint")
    {
        STATIC_CHECK(byteHashable<za::String>);
        STATIC_CHECK(byteHashable<za::StringView>);
        STATIC_CHECK(byteHashable<za::Vector<char>>);
        STATIC_CHECK(byteHashable<za::Vector<int>>);
        STATIC_CHECK(byteHashable<za::Vector<za::U64>>);

        STATIC_CHECK(!byteHashable<za::Vector<za::String>>);
        STATIC_CHECK(!byteHashable<za::Vector<float>>);
        STATIC_CHECK(!byteHashable<za::Vector<double>>);
        STATIC_CHECK(!byteHashable<za::Vector<Padded>>);
    }

    SECTION("String keys")
    {
        ankerl::unordered_dense::map<za::String, int> m;
        m["hello"] = 1;
        m["world"] = 2;

        CHECK(m.at(za::String{"hello"}) == 1);
        CHECK(m.find(za::String{"world"})->second == 2);
        CHECK(!m.contains(za::String{"other"}));
    }

    SECTION("StringView keys")
    {
        ankerl::unordered_dense::map<za::StringView, int> m;
        m[za::StringView{"hello"}] = 1;

        const char buffer[] = "hello";
        CHECK(m.at(za::StringView{buffer}) == 1);
    }

    SECTION("Vector keys")
    {
        ankerl::unordered_dense::map<za::Vector<char>, int> cm;
        cm[za::Vector<char>{'a', 'b'}] = 1;
        CHECK(cm.at(za::Vector<char>{'a', 'b'}) == 1);
        CHECK(!cm.contains(za::Vector<char>{'a'}));

        ankerl::unordered_dense::map<za::Vector<int>, int> im;
        im[za::Vector<int>{1, 2, 3}] = 1;
        CHECK(im.at(za::Vector<int>{1, 2, 3}) == 1);
        CHECK(!im.contains(za::Vector<int>{1, 2}));
    }
}


TEST_CASE("[Base] za::ankerl trivial relocation")
{
    using namespace AnkerlUnorderedDenseTest;
    using ankerl::unordered_dense::detail::pair;

    STATIC_CHECK(za::isTriviallyRelocatable<pair<int, int>>);
    STATIC_CHECK(za::isTriviallyRelocatable<pair<za::String, za::Vector<int>>>);
    STATIC_CHECK(za::isTriviallyRelocatable<pair<int, za::String>>);
    STATIC_CHECK(!za::isTriviallyRelocatable<pair<int, NonRelocatable>>);
    STATIC_CHECK(!za::isTriviallyRelocatable<pair<NonRelocatable, int>>);

    STATIC_CHECK(za::isTriviallyRelocatable<IntMap>);
    STATIC_CHECK(za::isTriviallyRelocatable<IntSet>);
    STATIC_CHECK(za::isTriviallyRelocatable<ankerl::unordered_dense::map<za::String, za::Vector<int>>>);

    SECTION("Map with relocatable values grows correctly")
    {
        ankerl::unordered_dense::map<int, za::String> m;
        for (int i = 0; i < 500; ++i)
            m[i] = "some string long enough to not fit in the small buffer";

        REQUIRE(m.size() == 500u);
        for (int i = 0; i < 500; ++i)
            CHECK(m.at(i) == "some string long enough to not fit in the small buffer");
    }
}


TEST_CASE("[Base] za::ankerl erase_if")
{
    using namespace AnkerlUnorderedDenseTest;

    SECTION("Map")
    {
        IntMap m;
        for (int i = 0; i < 100; ++i)
            m[i] = i;

        const za::SizeT erased = erase_if(m, [](const auto& kv) { return kv.second % 2 == 0; }); // found via ADL
        CHECK(erased == 50u);
        CHECK(m.size() == 50u);

        for (int i = 0; i < 100; ++i)
            CHECK(m.contains(i) == (i % 2 != 0));
    }

    SECTION("Set")
    {
        IntSet s{1, 2, 3, 4, 5};
        CHECK(erase_if(s, [](const int x) { return x > 3; }) == 2u);
        CHECK(s.size() == 3u);
        CHECK(!s.contains(4));
        CHECK(s.contains(3));
    }
}


TEST_CASE("[Base] za::ankerl::map copy and clear")
{
    using namespace AnkerlUnorderedDenseTest;

    IntMap a;
    for (int i = 0; i < 50; ++i)
        a[i] = i;

    IntMap b;
    b[-1] = -1;
    b     = a;
    CHECK(static_cast<bool>(b == a));
    CHECK(!b.contains(-1));

    IntMap small;
    small[1] = 1;
    b        = small; // copy-assign a smaller map over a larger one
    CHECK(static_cast<bool>(b == small));
    CHECK(!b.contains(40));

    b.clear();
    CHECK(b.empty());
    CHECK(b.find(1) == b.end());
    b[7] = 7;
    CHECK(b.at(7) == 7);

    b.reserve(1000);
    CHECK(b.at(7) == 7);
    b.rehash(0);
    CHECK(b.at(7) == 7);
}
