#include "Tst/Tst.hpp"

#include "Zancle/String/String.hpp"

#include "Zancle/Algorithm/AdjacentFind.hpp"
#include "Zancle/Algorithm/AllOf.hpp"
#include "Zancle/Algorithm/AnyOf.hpp"
#include "Zancle/Algorithm/Copy.hpp"
#include "Zancle/Algorithm/Count.hpp"
#include "Zancle/Algorithm/Erase.hpp"
#include "Zancle/Algorithm/Fill.hpp"
#include "Zancle/Algorithm/Find.hpp"
#include "Zancle/Algorithm/Iota.hpp"
#include "Zancle/Algorithm/IsSorted.hpp"
#include "Zancle/Algorithm/LowerBound.hpp"
#include "Zancle/Algorithm/MaxElement.hpp"
#include "Zancle/Algorithm/Remove.hpp"
#include "Zancle/Algorithm/Replace.hpp"
#include "Zancle/Algorithm/Rotate.hpp"
#include "Zancle/Algorithm/Shuffle.hpp"
#include "Zancle/Algorithm/StablePartition.hpp"
#include "Zancle/Algorithm/SwapAndPop.hpp"
#include "Zancle/Algorithm/Unique.hpp"

#include "Zancle/Container/Vector.hpp"


namespace
{
namespace AlgorithmTest // for unity builds
{
////////////////////////////////////////////////////////////
struct NoDefaultCtor
{
    int value;

    explicit NoDefaultCtor(const int v) : value{v}
    {
    }
};


////////////////////////////////////////////////////////////
[[nodiscard]] constexpr int copyAtCompileTime()
{
    const int values[]{1, 2, 3};
    int       target[3]{};

    za::copy(values, values + 3, target);
    return target[0] + target[1] * 10 + target[2] * 100;
}

////////////////////////////////////////////////////////////
// Key plus original position, to make the stability of partitions visible
struct Tagged
{
    int key;
    int tag;

    [[nodiscard]] constexpr bool operator==(const Tagged&) const = default;
};


////////////////////////////////////////////////////////////
struct MoveOnly
{
    int value;

    explicit MoveOnly(const int v) : value{v}
    {
    }

    MoveOnly(const MoveOnly&)            = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    MoveOnly(MoveOnly&&) noexcept            = default;
    MoveOnly& operator=(MoveOnly&&) noexcept = default;
};


////////////////////////////////////////////////////////////
template <typename T, za::SizeT N>
[[nodiscard]] constexpr bool rangeEquals(const T (&actual)[N], const T (&expected)[N])
{
    for (za::SizeT i = 0u; i < N; ++i)
        if (!(actual[i] == expected[i]))
            return false;

    return true;
}


////////////////////////////////////////////////////////////
[[nodiscard]] constexpr bool stablePartitionAtCompileTime()
{
    int values[]{1, 2, 3, 4, 5, 6};
    return za::stablePartition(values, values + 6, [](const int x) { return x % 2 == 0; }) == values + 3 &&
           rangeEquals(values, {2, 4, 6, 1, 3, 5});
}


////////////////////////////////////////////////////////////
[[nodiscard]] constexpr bool otherAlgorithmsAtCompileTime()
{
    constexpr int sorted[]{1, 3, 3, 7};

    int values[5]{};
    za::fill(values, values + 5, 4);
    za::iota(values + 1, values + 4, 10);
    za::replace(values, values + 5, 4, 0);

    return za::lowerBound(sorted, sorted + 4, 3) == sorted + 1 && rangeEquals(values, {0, 10, 11, 12, 0});
}

} // namespace AlgorithmTest
} // namespace


TEST_CASE("[Base] Base/Algorithm/*.hpp")
{
    SECTION("Copy")
    {
        const int values[]{0, 1, 2, 3};
        int       target[4];

        CHECK(za::copy(values, values + 4, target) == target + 4);

        CHECK(target[0] == 0);
        CHECK(target[1] == 1);
        CHECK(target[2] == 2);
        CHECK(target[3] == 3);
    }

    SECTION("Copy: memmove path")
    {
        // Empty ranges, including null ones
        int* const nullPtr = nullptr;
        CHECK(za::copy(nullPtr, nullPtr, nullPtr) == nullptr);

        // Overlapping, with the destination before the source (allowed, like `std::copy`)
        int values[]{0, 1, 2, 3, 4, 5};
        CHECK(za::copy(values + 2, values + 6, values) == values + 4);
        CHECK(values[0] == 2);
        CHECK(values[1] == 3);
        CHECK(values[2] == 4);
        CHECK(values[3] == 5);

        // Usable in constant expressions
        STATIC_CHECK(AlgorithmTest::copyAtCompileTime() == 321);
    }

    SECTION("Copy: non-trivial elements")
    {
        const za::String values[]{"a", "long string that is not stored inline, hopefully"};
        za::String       target[2];

        CHECK(za::copy(values, values + 2, target) == target + 2);
        CHECK(target[0] == values[0]);
        CHECK(target[1] == values[1]);
    }

    SECTION("CountTruthy")
    {
        const int values[]{0, 1, 0, 2, 3, 0};

        CHECK(za::countTruthy(values, values + 6) == 3u);
        CHECK(za::countTruthy(values, values) == 0u);
        CHECK(za::countTruthy(values, values + 1) == 0u);

        const bool flags[]{true, true, false};
        CHECK(za::countTruthy(flags, flags + 3) == 2u);
    }

    SECTION("MaxElement")
    {
        const int values[]{3, 7, 1, 7, 2};

        CHECK(za::maxElement(values, values) == values); // empty: `last`
        CHECK(za::maxElement(values, values + 1) == values);
        CHECK(za::maxElement(values, values + 5) == values + 1); // first of the ties

        const auto greater = [](const int a, const int b) { return a > b; };
        CHECK(za::maxElement(values, values + 5, greater) == values + 2); // i.e. the minimum
    }

    SECTION("Unique")
    {
        int values[]{1, 1, 2, 2, 2, 3, 1, 1, 4};

        int* const newEnd = za::unique(values, values + 9);

        REQUIRE(newEnd == values + 5);
        CHECK(values[0] == 1);
        CHECK(values[1] == 2);
        CHECK(values[2] == 3);
        CHECK(values[3] == 1); // only consecutive duplicates are removed
        CHECK(values[4] == 4);

        CHECK(za::unique(values, values) == values);
        CHECK(za::unique(values, values + 1) == values + 1);

        int allSame[]{5, 5, 5};
        CHECK(za::unique(allSame, allSame + 3) == allSame + 1);

        za::Vector<za::String> strings{"a", "a", "b", "c", "c"};
        strings.erase(za::unique(strings.begin(), strings.end()), strings.end());
        CHECK((strings == za::Vector<za::String>{"a", "b", "c"}));
    }

    SECTION("Shuffle")
    {
        int values[]{0, 1, 2, 3, 4, 5, 6, 7};

        // Always picking the lowest index is a valid (if unlucky) RNG: it rotates left by one
        za::SizeT nCalls = 0u;
        za::shuffle(values,
                    values + 8,
                    [&](const za::SizeT min, const za::SizeT max)
        {
            CHECK(min == 0u);
            CHECK(max == 7u - nCalls); // Fisher-Yates bounds shrink by one each step
            ++nCalls;
            return min;
        });

        CHECK(nCalls == 7u);
        CHECK(values[0] == 1);
        CHECK(values[1] == 2);
        CHECK(values[7] == 0);

        // Picking the highest index leaves the range untouched
        za::shuffle(values, values + 8, [](za::SizeT, const za::SizeT max) { return max; });
        CHECK(values[0] == 1);
        CHECK(values[1] == 2);
        CHECK(values[7] == 0);

        // A pseudo-random RNG produces a permutation
        unsigned int state = 42u;
        za::shuffle(values,
                    values + 8,
                    [&](const za::SizeT min, const za::SizeT max)
        {
            state = state * 1'664'525u + 1'013'904'223u;
            return min + (state >> 8u) % (max - min + 1u);
        });

        bool seen[8]{};
        for (const int v : values)
            seen[v] = true;

        for (const bool s : seen)
            CHECK(s);

        // Empty and single-element ranges never call the RNG
        za::shuffle(values,
                    values,
                    [](za::SizeT, za::SizeT) -> za::SizeT
        {
            FAIL_CHECK("unexpected call");
            return 0u;
        });
        za::shuffle(values,
                    values + 1,
                    [](za::SizeT, za::SizeT) -> za::SizeT
        {
            FAIL_CHECK("unexpected call");
            return 0u;
        });
    }

    SECTION("Find/FindIf/AnyOf")
    {
        const int values[]{0, 1, 2, 3, 4, 5, 6, 7};

        CHECK(za::find(values, values + 8, 4) == &values[4]);
        CHECK(za::find(values, values + 8, 5) == &values[5]);
        CHECK(za::find(values, values + 8, 400) == values + 8);

        CHECK(za::findIf(values, values + 8, [](const int x) { return x == 4; }) == &values[4]);
        CHECK(za::findIf(values, values + 8, [](const int x) { return x == 5; }) == &values[5]);
        CHECK(za::findIf(values, values + 8, [](const int x) { return x == 400; }) == values + 8);

        CHECK(za::anyOf(values, values + 8, [](const int x) { return x == 4; }));
        CHECK(za::anyOf(values, values + 8, [](const int x) { return x == 5; }));
        CHECK(!za::anyOf(values, values + 8, [](const int x) { return x == 400; }));
    }

    SECTION("Count")
    {
        const bool bools[]{true, false, true, true, false};
        CHECK(za::count(bools, bools + 5, true) == 3);

        const int ints[]{1, 0, 5, 0, -1, 0, 7};
        CHECK(za::count(ints, ints + 7, 0) == 3);
    }

    SECTION("CountIf")
    {
        const int values[]{0, 1, 2, 3, 4, 5, 6, 7};

        const auto isEven      = [](int x) { return x % 2 == 0; };
        const auto isGt5       = [](int x) { return x > 5; };
        const auto alwaysFalse = [](int) { return false; };

        CHECK(za::countIf(values, values + 8, isEven) == 4);
        CHECK(za::countIf(values, values + 8, isGt5) == 2); // 6, 7
        CHECK(za::countIf(values, values + 8, alwaysFalse) == 0);

        CHECK(za::countIf(values, values + 0, isEven) == 0);
    }

    SECTION("AllOf")
    {
        const int values[]{0, 1, 2, 3, 4, 5, 6, 7};

        const auto isLt10     = [](int x) { return x < 10; };
        const auto isEven     = [](int x) { return x % 2 == 0; };
        const auto alwaysTrue = [](int) { return true; };

        CHECK(za::allOf(values, values + 8, isLt10));
        CHECK(!za::allOf(values, values + 8, isEven));

        const int evenValues[]{2, 4, 6, 8};
        CHECK(za::allOf(evenValues, evenValues + 4, isEven));

        CHECK(za::allOf(evenValues, evenValues + 0, isEven));     // Vacuously true
        CHECK(za::allOf(evenValues, evenValues + 0, alwaysTrue)); // Vacuously true
    }


    SECTION("RemoveIf")
    {
        za::Vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};

        const auto isEven = [](int x) { return x % 2 == 0; };

        auto* newEnd = za::removeIf(v.begin(), v.end(), isEven);

        // Check the returned iterator position
        CHECK((newEnd == v.begin() + 4));

        // Check the elements that should remain are at the beginning
        CHECK(v[0] == 1);
        CHECK(v[1] == 3);
        CHECK(v[2] == 5);
        CHECK(v[3] == 7);
        // Elements from v[4] onwards are moved-from/unspecified but valid

        // Check with no elements to remove
        za::Vector<int> vecOdd{1, 3, 5, 7};
        auto*           newEndOdd = za::removeIf(vecOdd.begin(), vecOdd.end(), isEven);
        CHECK((newEndOdd == vecOdd.end()));
        CHECK((vecOdd == za::Vector<int>{1, 3, 5, 7}));

        // Check with all elements to remove
        za::Vector<int> vecEven{2, 4, 6, 8};
        auto*           newEndEven = za::removeIf(vecEven.begin(), vecEven.end(), isEven);
        CHECK((newEndEven == vecEven.begin()));
    }

    SECTION("VectorEraseIf")
    {
        za::Vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};
        auto            isEven = [](int x) { return x % 2 == 0; };

        za::SizeT removedCount = za::vectorEraseIf(v, isEven);

        CHECK(removedCount == 4);
        CHECK(v.size() == 4);
        CHECK((v == za::Vector<int>{1, 3, 5, 7}));

        // Check with no elements removed
        removedCount = za::vectorEraseIf(v, isEven);
        CHECK(removedCount == 0);
        CHECK(v.size() == 4);
        CHECK((v == za::Vector<int>{1, 3, 5, 7}));

        // Check removing all elements
        auto isOdd   = [](int x) { return x % 2 != 0; };
        removedCount = za::vectorEraseIf(v, isOdd);
        CHECK(removedCount == 4);
        CHECK(v.empty());

        // Check empty vector
        removedCount = za::vectorEraseIf(v, isOdd);
        CHECK(removedCount == 0);
        CHECK(v.empty());
    }

    SECTION("IsSorted")
    {
        const auto less    = [](int a, int b) { return a < b; };
        const auto greater = [](int a, int b) { return a > b; };

        const int sorted[] = {1, 2, 2, 3, 4, 5};
        CHECK(za::isSorted(sorted, sorted + 6, less));

        const int unsorted[] = {1, 3, 2, 4, 5};
        CHECK(!za::isSorted(unsorted, unsorted + 5, less));

        const int reverseSorted[] = {5, 4, 3, 2, 1};
        CHECK(!za::isSorted(reverseSorted, reverseSorted + 5, less));
        CHECK(za::isSorted(reverseSorted, reverseSorted + 5, greater)); // Check with different comparer

        const int single[] = {10};
        CHECK(za::isSorted(single, single + 1, less)); // Single element is sorted

        CHECK(za::isSorted(unsorted, unsorted + 0, less)); // Empty range is sorted
    }

    SECTION("VectorSwapAndPopIf")
    {
        za::Vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};
        auto            isEven = [](int x) { return x % 2 == 0; };

        za::SizeT removedCount = za::vectorSwapAndPopIf(v, isEven);

        CHECK(removedCount == 4);
        CHECK(v.size() == 4);
        CHECK((v == za::Vector<int>{1, 5, 3, 7}));

        // Check with no elements removed
        removedCount = za::vectorSwapAndPopIf(v, isEven);
        CHECK(removedCount == 0);
        CHECK(v.size() == 4);
        CHECK((v == za::Vector<int>{1, 5, 3, 7}));

        // Check removing all elements
        auto isOdd   = [](int x) { return x % 2 != 0; };
        removedCount = za::vectorSwapAndPopIf(v, isOdd);
        CHECK(removedCount == 4);
        CHECK(v.empty());

        // Check empty vector
        removedCount = za::vectorSwapAndPopIf(v, isOdd);
        CHECK(removedCount == 0);
        CHECK(v.empty());
    }

    SECTION("VectorSwapAndPopIf: elements without a default constructor")
    {
        za::Vector<AlgorithmTest::NoDefaultCtor> v;
        for (int i = 0; i < 6; ++i)
            v.emplaceBack(i);

        CHECK(za::vectorSwapAndPopIf(v, [](const auto& x) { return x.value % 3 == 0; }) == 2u);
        REQUIRE(v.size() == 4u);

        for (const auto& x : v)
            CHECK(x.value % 3 != 0);
    }


    SECTION("AdjacentFind (Default): Core functionality")
    {
        SUBCASE("Pair found at the beginning")
        {
            int   vec[] = {5, 5, 2, 3, 4};
            auto* it    = za::adjacentFind(vec, vec + 5);
            REQUIRE(it == vec);
            CHECK(*it == 5);
        }

        SUBCASE("Pair found in the middle")
        {
            int   vec[] = {1, 2, 8, 8, 3};
            auto* it    = za::adjacentFind(vec, vec + 5);
            REQUIRE(it == vec + 2);
            CHECK(*it == 8);
        }

        SUBCASE("Pair found at the end")
        {
            int   vec[] = {1, 2, 3, 9, 9};
            auto* it    = za::adjacentFind(vec, vec + 5);
            REQUIRE(it == vec + 3);
            CHECK(*it == 9);
        }

        SUBCASE("Multiple pairs exist, finds the first one")
        {
            int   vec[] = {1, 2, 2, 3, 4, 4};
            auto* it    = za::adjacentFind(vec, vec + 6);
            REQUIRE(it == vec + 1);
            CHECK(*it == 2);
        }
    }

    SECTION("AdjacentFind (Default): Edge cases")
    {
        SUBCASE("No adjacent pair found")
        {
            int   vec[] = {1, 2, 3, 4, 5, 4, 3, 2, 1};
            auto* it    = za::adjacentFind(vec, vec + 9);
            CHECK(it == vec + 9);
        }

        SUBCASE("Empty range")
        {
            int   vec[1]{};
            auto* it = za::adjacentFind(vec, vec + 0);
            CHECK(it == vec + 0);
        }

        SUBCASE("Single element range")
        {
            int   vec[] = {100};
            auto* it    = za::adjacentFind(vec, vec + 1);
            CHECK(it == vec + 1);
        }
    }

    SECTION("Adjacent find")
    {
        SUBCASE("Find where second element is greater than first")
        {
            int  vec[5]     = {5, 2, 3, 1, 8};
            auto greaterCmp = [](int a, int b) { return b > a; };

            auto* it = za::adjacentFind(vec, vec + 5, greaterCmp);

            // The first pair where b > a is (2, 3)
            REQUIRE(it == vec + 1);
            CHECK(*it == 2);
            CHECK(*(it + 1) == 3);
        }

        SUBCASE("Predicate is never satisfied")
        {
            int  vec[5]     = {10, 8, 6, 4, 2};
            auto greaterCmp = [](int a, int b) { return b > a; };

            auto* it = za::adjacentFind(vec, vec + 5, greaterCmp);
            CHECK(it == vec + 5);
        }
    }

    SECTION("Rotate")
    {
        SUBCASE("Rotate by 3 in middle of range")
        {
            int values[] = {1, 2, 3, 4, 5, 6, 7};

            auto* it = za::rotate(values, values + 3, values + 7);

            CHECK(it == values + 4);
            CHECK(values[0] == 4);
            CHECK(values[1] == 5);
            CHECK(values[2] == 6);
            CHECK(values[3] == 7);
            CHECK(values[4] == 1);
            CHECK(values[5] == 2);
            CHECK(values[6] == 3);
        }

        SUBCASE("Rotate by 1")
        {
            int values[] = {1, 2, 3, 4};

            auto* it = za::rotate(values, values + 1, values + 4);

            CHECK(it == values + 3);
            CHECK(values[0] == 2);
            CHECK(values[1] == 3);
            CHECK(values[2] == 4);
            CHECK(values[3] == 1);
        }

        SUBCASE("Rotate by size - 1 (single element to the back)")
        {
            int values[] = {1, 2, 3, 4};

            auto* it = za::rotate(values, values + 3, values + 4);

            CHECK(it == values + 1);
            CHECK(values[0] == 4);
            CHECK(values[1] == 1);
            CHECK(values[2] == 2);
            CHECK(values[3] == 3);
        }

        SUBCASE("Rotate with middle == first is a no-op")
        {
            int values[] = {1, 2, 3, 4};

            auto* it = za::rotate(values, values, values + 4);

            CHECK(it == values + 4);
            CHECK(values[0] == 1);
            CHECK(values[1] == 2);
            CHECK(values[2] == 3);
            CHECK(values[3] == 4);
        }

        SUBCASE("Rotate with middle == last is a no-op")
        {
            int values[] = {1, 2, 3, 4};

            auto* it = za::rotate(values, values + 4, values + 4);

            CHECK(it == values);
            CHECK(values[0] == 1);
            CHECK(values[1] == 2);
            CHECK(values[2] == 3);
            CHECK(values[3] == 4);
        }

        SUBCASE("Rotate at exact midpoint of even-sized range")
        {
            int values[] = {1, 2, 3, 4, 5, 6};

            auto* it = za::rotate(values, values + 3, values + 6);

            CHECK(it == values + 3);
            CHECK(values[0] == 4);
            CHECK(values[1] == 5);
            CHECK(values[2] == 6);
            CHECK(values[3] == 1);
            CHECK(values[4] == 2);
            CHECK(values[5] == 3);
        }

        SUBCASE("Rotate with second segment larger than first (uneven)")
        {
            int values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

            auto* it = za::rotate(values, values + 2, values + 9);

            CHECK(it == values + 7);
            CHECK(values[0] == 3);
            CHECK(values[1] == 4);
            CHECK(values[2] == 5);
            CHECK(values[3] == 6);
            CHECK(values[4] == 7);
            CHECK(values[5] == 8);
            CHECK(values[6] == 9);
            CHECK(values[7] == 1);
            CHECK(values[8] == 2);
        }
    }
}


TEST_CASE("[Base] Algorithm/StablePartition.hpp")
{
    using AlgorithmTest::rangeEquals;
    using AlgorithmTest::Tagged;

    const auto isEven = [](const int x) { return x % 2 == 0; };

    SECTION("Partitions while preserving the relative order of each group")
    {
        int values[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        CHECK(za::stablePartition(values, values + 10, isEven) == values + 5);
        CHECK(rangeEquals(values, {2, 4, 6, 8, 10, 1, 3, 5, 7, 9}));
    }

    SECTION("Equal keys keep their order")
    {
        Tagged values[]{{3, 0}, {1, 1}, {4, 2}, {1, 3}, {5, 4}, {9, 5}, {2, 6}, {6, 7}, {5, 8}, {3, 9}};

        CHECK(za::stablePartition(values, values + 10, [](const Tagged& t) { return t.key < 4; }) == values + 5);
        CHECK(rangeEquals(values, {{3, 0}, {1, 1}, {1, 3}, {2, 6}, {3, 9}, {4, 2}, {5, 4}, {9, 5}, {6, 7}, {5, 8}}));
    }

    SECTION("Edge cases")
    {
        int values[]{1, 3, 5};
        int empty[1]{};

        CHECK(za::stablePartition(empty, empty, isEven) == empty);
        CHECK(za::stablePartition(values, values + 1, isEven) == values);                       // one `false` element
        CHECK(za::stablePartition(values, values + 3, isEven) == values);                       // all `false`
        CHECK(za::stablePartition(values, values + 3, [](int) { return true; }) == values + 3); // all `true`
        CHECK(rangeEquals(values, {1, 3, 5}));

        int one[]{2};
        CHECK(za::stablePartition(one, one + 1, isEven) == one + 1); // one `true` element
    }

    SECTION("Calls the predicate exactly once per element, left to right")
    {
        int values[100];
        for (int i = 0; i < 100; ++i)
            values[i] = i;

        int  calls       = 0;
        bool inOrder     = true;
        int  previousArg = -1;

        za::stablePartition(values,
                            values + 100,
                            [&](const int x)
        {
            ++calls;
            inOrder &= x > previousArg;
            previousArg = x;
            return x % 3 == 0;
        });

        CHECK(calls == 100);
        CHECK(inOrder);

        // The 34 multiples of 3, then all the others, both ascending
        bool correct = true;
        int  i       = 0;

        for (int expected = 0; expected < 100; expected += 3)
            correct &= values[i++] == expected;

        for (int expected = 1; expected < 100; ++expected)
            if (expected % 3 != 0)
                correct &= values[i++] == expected;

        CHECK(i == 100);
        CHECK(correct);
    }

    SECTION("Move-only elements")
    {
        za::Vector<AlgorithmTest::MoveOnly> values;
        for (const int x : {5, 2, 8, 1, 4})
            values.emplaceBack(x);

        auto* const boundary = za::stablePartition(values.begin(), values.end(), [](const auto& m) {
            return m.value > 3;
        });

        CHECK(boundary == values.begin() + 3);
        CHECK(values[0].value == 5);
        CHECK(values[1].value == 8);
        CHECK(values[2].value == 4);
        CHECK(values[3].value == 2);
        CHECK(values[4].value == 1);
    }

    SECTION("Usable in constant expressions")
    {
        STATIC_CHECK(AlgorithmTest::stablePartitionAtCompileTime());
    }
}


TEST_CASE("[Base] Algorithm/LowerBound.hpp")
{
    const int values[]{1, 2, 2, 2, 3, 5, 8};

    SECTION("Default comparer")
    {
        CHECK(za::lowerBound(values, values + 7, 2) == values + 1); // first of the equal elements
        CHECK(za::lowerBound(values, values + 7, 0) == values + 0);
        CHECK(za::lowerBound(values, values + 7, 1) == values + 0);
        CHECK(za::lowerBound(values, values + 7, 4) == values + 5); // between elements
        CHECK(za::lowerBound(values, values + 7, 8) == values + 6);
        CHECK(za::lowerBound(values, values + 7, 9) == values + 7);   // past the end
        CHECK(za::lowerBound(values, values, 2) == values);           // empty range
        CHECK(za::lowerBound(values, values + 7, 2.5) == values + 4); // heterogeneous value
    }

    SECTION("Custom comparer, called with the element first")
    {
        const int  descending[]{9, 7, 7, 4, 1};
        const auto greater = [](const int element, const int value) { return element > value; };

        CHECK(za::lowerBound(descending, descending + 5, 7, greater) == descending + 1);
        CHECK(za::lowerBound(descending, descending + 5, 5, greater) == descending + 3);
        CHECK(za::lowerBound(descending, descending + 5, 0, greater) == descending + 5);
        CHECK(za::lowerBound(descending, descending + 5, 10, greater) == descending + 0);
    }

    SECTION("Logarithmic number of comparisons")
    {
        za::Vector<int> big;
        for (int i = 0; i < 1000; ++i)
            big.pushBack(i);

        int comparisons = 0;
        CHECK(za::lowerBound(big.begin(),
                             big.end(),
                             777,
                             [&](const int a, const int b)
        {
            ++comparisons;
            return a < b;
        }) == big.begin() + 777);

        CHECK(comparisons <= 10); // ceil(log2(1000))
    }
}


TEST_CASE("[Base] Algorithm/Fill.hpp, Iota.hpp, Replace.hpp")
{
    using AlgorithmTest::rangeEquals;

    SECTION("fill")
    {
        int values[]{1, 2, 3, 4, 5};

        za::fill(values + 1, values + 4, 0);
        CHECK(rangeEquals(values, {1, 0, 0, 0, 5}));

        za::fill(values, values, 9); // empty range
        CHECK(rangeEquals(values, {1, 0, 0, 0, 5}));

        za::String strings[3];
        za::fill(strings, strings + 3, za::String{"a fairly long string, likely allocated on the heap"});
        CHECK(strings[0] == strings[2]);
        CHECK(strings[1] == "a fairly long string, likely allocated on the heap");

        double doubles[2]{};
        za::fill(doubles, doubles + 2, 3); // `int` value, `double` elements
        CHECK(doubles[1] == 3.0);
    }

    SECTION("iota")
    {
        int values[5]{};
        za::iota(values, values + 5, -2);
        CHECK(rangeEquals(values, {-2, -1, 0, 1, 2}));

        char letters[4]{};
        za::iota(letters, letters + 4, 'a');
        CHECK(rangeEquals(letters, {'a', 'b', 'c', 'd'}));

        double doubles[3]{};
        za::iota(doubles, doubles + 3, 0.5);
        CHECK(rangeEquals(doubles, {0.5, 1.5, 2.5}));
    }

    SECTION("replace")
    {
        int values[]{1, 2, 3, 2, 1};

        za::replace(values, values + 5, 2, 7);
        CHECK(rangeEquals(values, {1, 7, 3, 7, 1}));

        za::replace(values, values + 5, 4, 0); // no match
        CHECK(rangeEquals(values, {1, 7, 3, 7, 1}));
    }

    SECTION("replace with `oldValue` referring to an element of the range")
    {
        // As with `std::replace`, the values are taken by reference: once `values[0]` is
        // replaced, later elements are compared against its new value (`1`)
        int values[]{2, 1, 2, 1};

        za::replace(values, values + 4, values[0], 1);
        CHECK(rangeEquals(values, {1, 1, 2, 1}));
    }

    SECTION("Usable in constant expressions")
    {
        STATIC_CHECK(AlgorithmTest::otherAlgorithmsAtCompileTime());
    }
}
