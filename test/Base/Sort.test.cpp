#include "StringifyVectorUtil.hpp"   // IWYU pragma: keep
#include "StringifyZbStringUtil.hpp" // IWYU pragma: keep
#include "Tst/Tst.hpp"

#include "Zancle/Algorithm/Sort.hpp"

#include "Zancle/String/String.hpp"

#include "Zancle/Algorithm/IsSorted.hpp"
#include "Zancle/Algorithm/LowerBound.hpp"
#include "Zancle/Algorithm/NthElement.hpp"
#include "Zancle/Algorithm/StableSort.hpp"
#include "Zancle/Algorithm/UpperBound.hpp"

#include "Zancle/Container/Vector.hpp"


namespace
{
const auto lessCmp    = [](const auto& a, const auto& b) { return a < b; };
const auto greaterCmp = [](const auto& a, const auto& b) { return a > b; };
} // namespace


TEST_CASE("[Base] Base/Sort.hpp")
{
    SECTION("Insertion Sort")
    {
        int values[]{3, 2, 1, 0};

        za::insertionSort(values, values + 4, lessCmp);

        CHECK(values[0] == 0);
        CHECK(values[1] == 1);
        CHECK(values[2] == 2);
        CHECK(values[3] == 3);
    }

    SECTION("Quick Sort")
    {
        int values[]{3, 2, 1, 0};

        za::quickSort(values, values + 4, lessCmp);

        CHECK(values[0] == 0);
        CHECK(values[1] == 1);
        CHECK(values[2] == 2);
        CHECK(values[3] == 3);
    }
}

struct Person
{
    za::String name;
    int        age;

    bool operator<(const Person& other) const
    {
        return name < other.name;
    }
};

TEST_CASE("QuickSort (Default Comparator): Core functionality")
{
    SUBCASE("Sorting a general vector of integers")
    {
        za::Vector<int> vec      = {9, 0, 2, 7, 5, 3, 8, 1, 6, 4};
        za::Vector<int> expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        za::quickSort(vec.begin(), vec.end(), lessCmp);
        CHECK(vec == expected);
    }

    SUBCASE("Sorting a vector with duplicate elements")
    {
        za::Vector<int> vec      = {5, 2, 8, 2, 9, 5, 8, 1, 5};
        za::Vector<int> expected = {1, 2, 2, 5, 5, 5, 8, 8, 9};
        za::quickSort(vec.begin(), vec.end(), lessCmp);
        CHECK(vec == expected);
    }
}

TEST_CASE("QuickSort (Default Comparator): Edge cases")
{
    SUBCASE("Sorting an empty vector")
    {
        za::Vector<int> vec;
        za::quickSort(vec.begin(), vec.end());
        CHECK(vec.empty());
    }

    SUBCASE("Sorting a single-element vector")
    {
        za::Vector<int> vec = {42};
        za::quickSort(vec.begin(), vec.end(), lessCmp);
        CHECK(vec == za::Vector<int>{42});
    }

    SUBCASE("Sorting an already sorted vector")
    {
        za::Vector<int> vec = {1, 2, 3, 4, 5};
        za::quickSort(vec.begin(), vec.end(), lessCmp);
        CHECK(vec == za::Vector<int>{1, 2, 3, 4, 5});
    }

    SUBCASE("Sorting a reverse-sorted vector")
    {
        za::Vector<int> vec = {10, 9, 8, 7, 6};
        za::quickSort(vec.begin(), vec.end(), lessCmp);
        CHECK(vec == za::Vector<int>{6, 7, 8, 9, 10});
    }

    SUBCASE("Sorting a vector with all elements identical")
    {
        za::Vector<int> vec = {7, 7, 7, 7, 7};
        za::quickSort(vec.begin(), vec.end(), lessCmp);
        CHECK(vec == za::Vector<int>{7, 7, 7, 7, 7});
    }
}

TEST_CASE("QuickSort (Custom Comparator): Functionality")
{
    SUBCASE("Sorting integers in descending order using std::greater")
    {
        za::Vector<int> vec      = {3, 1, 4, 1, 5, 9};
        za::Vector<int> expected = {9, 5, 4, 3, 1, 1};
        za::quickSort(vec.begin(), vec.end(), greaterCmp);
        CHECK(vec == expected);
    }

    SUBCASE("Sorting a custom struct by a member using a lambda")
    {
        za::Vector<Person> people = {{"Charlie", 35}, {"Alice", 30}, {"Bob", 25}};

        auto compareByAge = [](const Person& a, const Person& b) { return a.age < b.age; };

        za::quickSort(people.begin(), people.end(), compareByAge);
        CHECK(za::isSorted(people.begin(), people.end(), compareByAge));

        CHECK(people[0].name == "Bob");
        CHECK(people[1].name == "Alice");
        CHECK(people[2].name == "Charlie");
    }

    SUBCASE("Sorting a custom struct using its default operator<")
    {
        za::Vector<Person> people = {{"Charlie", 35}, {"Alice", 30}, {"Bob", 25}};

        za::quickSort(people.begin(), people.end(), lessCmp);

        CHECK(people[0].name == "Alice");
        CHECK(people[1].name == "Bob");
        CHECK(people[2].name == "Charlie");
    }
}

TEST_CASE("InsertionSort (with Comparator): Functionality")
{
    SUBCASE("Sorting a small, unsorted array ascending")
    {
        za::Vector<int> vec      = {5, 1, 4, 2, 8};
        za::Vector<int> expected = {1, 2, 4, 5, 8};
        za::insertionSort(vec.begin(), vec.end(), lessCmp);
        CHECK(vec == expected);
    }

    SUBCASE("Sorting a small, unsorted array descending")
    {
        za::Vector<int> vec      = {5, 1, 4, 2, 8};
        za::Vector<int> expected = {8, 5, 4, 2, 1};
        za::insertionSort(vec.begin(), vec.end(), greaterCmp);
        CHECK(vec == expected);
    }

    SUBCASE("Sorting an empty range")
    {
        za::Vector<int> vec;
        za::insertionSort(vec.begin(), vec.end(), lessCmp);
        CHECK(vec.empty());
    }
}


namespace
{
namespace SortTest // for unity builds
{
////////////////////////////////////////////////////////////
// Small deterministic PRNG, so that failures are reproducible
struct Lcg
{
    unsigned int state;

    unsigned int next()
    {
        state = state * 1'664'525u + 1'013'904'223u;
        return state >> 8u;
    }
};


////////////////////////////////////////////////////////////
// McIlroy's "killer adversary" (A Killer Adversary for Quicksort, 1999): assigns values to
// elements lazily, during the comparisons, so that every pivot ends up near the minimum.
// Drives any plain quicksort to quadratic behavior.
struct Adversary
{
    za::Vector<int> values; // `gas` means "not decided yet"
    int             gas;
    int             nSolid    = 0;
    int             candidate = 0;
    long long       nComparisons{0};

    explicit Adversary(const int n) : values(static_cast<za::SizeT>(n), n), gas{n}
    {
    }

    bool less(const int x, const int y)
    {
        ++nComparisons;

        int& vx = values[static_cast<za::SizeT>(x)];
        int& vy = values[static_cast<za::SizeT>(y)];

        if (vx == gas && vy == gas)
            (x == candidate ? vx : vy) = nSolid++;

        if (vx == gas)
            candidate = x;
        else if (vy == gas)
            candidate = y;

        return vx < vy;
    }
};


////////////////////////////////////////////////////////////
struct MoveCounted
{
    static inline int moves = 0;

    int value;

    explicit MoveCounted(const int v) : value{v}
    {
    }

    MoveCounted(MoveCounted&& rhs) noexcept : value{rhs.value}
    {
        ++moves;
    }

    MoveCounted& operator=(MoveCounted&& rhs) noexcept
    {
        value = rhs.value;
        ++moves;
        return *this;
    }

    MoveCounted(const MoveCounted&)            = delete;
    MoveCounted& operator=(const MoveCounted&) = delete;

    bool operator<(const MoveCounted& rhs) const
    {
        return value < rhs.value;
    }
};

} // namespace SortTest
} // namespace


TEST_CASE("HeapSort: Functionality")
{
    SUBCASE("Edge cases")
    {
        za::Vector<int> vec;
        za::heapSort(vec.begin(), vec.end());
        CHECK(vec.empty());

        vec = {42};
        za::heapSort(vec.begin(), vec.end());
        CHECK(vec == za::Vector<int>{42});

        vec = {2, 1};
        za::heapSort(vec.begin(), vec.end());
        CHECK(vec == za::Vector<int>{1, 2});
    }

    SUBCASE("General, duplicate, sorted, reversed, and identical elements")
    {
        za::Vector<int> vec = {9, 0, 2, 7, 5, 3, 8, 1, 6, 4};
        za::heapSort(vec.begin(), vec.end());
        CHECK(vec == za::Vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

        vec = {5, 2, 8, 2, 9, 5, 8, 1, 5};
        za::heapSort(vec.begin(), vec.end(), lessCmp);
        CHECK(vec == za::Vector<int>{1, 2, 2, 5, 5, 5, 8, 8, 9});

        vec = {1, 2, 3, 4, 5};
        za::heapSort(vec.begin(), vec.end());
        CHECK(vec == za::Vector<int>{1, 2, 3, 4, 5});

        vec = {10, 9, 8, 7, 6};
        za::heapSort(vec.begin(), vec.end());
        CHECK(vec == za::Vector<int>{6, 7, 8, 9, 10});

        vec = {7, 7, 7, 7, 7};
        za::heapSort(vec.begin(), vec.end());
        CHECK(vec == za::Vector<int>{7, 7, 7, 7, 7});
    }

    SUBCASE("Custom comparator")
    {
        za::Vector<int> vec = {3, 1, 4, 1, 5, 9};
        za::heapSort(vec.begin(), vec.end(), greaterCmp);
        CHECK(vec == za::Vector<int>{9, 5, 4, 3, 1, 1});
    }

    SUBCASE("Non-trivial elements")
    {
        za::Vector<Person> people = {{"Charlie", 35}, {"Alice", 30}, {"Bob", 25}, {"Dave", 40}};
        za::heapSort(people.begin(), people.end());

        CHECK(people[0].name == "Alice");
        CHECK(people[1].name == "Bob");
        CHECK(people[2].name == "Charlie");
        CHECK(people[3].name == "Dave");
    }

    SUBCASE("Move-only elements")
    {
        using SortTest::MoveCounted;

        za::Vector<MoveCounted> vec;
        for (const int x : {4, 2, 5, 1, 3})
            vec.emplaceBack(x);

        za::heapSort(vec.begin(), vec.end());

        for (int i = 0; i < 5; ++i)
            CHECK(vec[static_cast<za::SizeT>(i)].value == i + 1);
    }
}


TEST_CASE("Sorting algorithms agree on many inputs")
{
    SortTest::Lcg rng{12'345u};

    const auto check = [&](const za::Vector<int>& input)
    {
        za::Vector<int> byQuick = input;
        za::Vector<int> byHeap  = input;

        za::quickSort(byQuick.begin(), byQuick.end());
        za::heapSort(byHeap.begin(), byHeap.end());

        CHECK(za::isSorted(byQuick.begin(), byQuick.end(), lessCmp));
        CHECK(byQuick == byHeap);

        if (input.size() <= 64u)
        {
            za::Vector<int> byInsertion = input;
            za::insertionSort(byInsertion.begin(), byInsertion.end());
            CHECK(byQuick == byInsertion);
        }
    };

    for (za::SizeT n = 0u; n < 300u; n += (n < 40u ? 1u : 13u))
    {
        za::Vector<int> random, fewDistinct, sorted, reversed, organPipe;

        for (za::SizeT i = 0u; i < n; ++i)
        {
            random.pushBack(static_cast<int>(rng.next() % 1000u));
            fewDistinct.pushBack(static_cast<int>(rng.next() % 3u));
            sorted.pushBack(static_cast<int>(i));
            reversed.pushBack(static_cast<int>(n - i));
            organPipe.pushBack(static_cast<int>(i < n / 2u ? i : n - i));
        }

        check(random);
        check(fewDistinct);
        check(sorted);
        check(reversed);
        check(organPipe);
    }
}


TEST_CASE("QuickSort: O(n log n) comparisons against a killer adversary")
{
    constexpr int n = 4096;

    SortTest::Adversary adversary{n};

    za::Vector<int> items;
    for (int i = 0; i < n; ++i)
        items.pushBack(i);

    za::quickSort(items.begin(), items.end(), [&](const int a, const int b) { return adversary.less(a, b); });

    // The adversary only commits to values it compared, so check against the final values
    for (za::SizeT i = 1u; i < items.size(); ++i)
        CHECK(adversary.values[static_cast<za::SizeT>(items[i - 1])] <= adversary.values[static_cast<za::SizeT>(items[i])]);

    // Plain median-of-three quicksort needs millions of comparisons here (~n^2/4)
    constexpr long long log2n = 12;
    CHECK(adversary.nComparisons < 8 * n * log2n);
}


TEST_CASE("InsertionSort: elements already in place are not moved")
{
    using SortTest::MoveCounted;

    za::Vector<MoveCounted> vec;
    for (int i = 0; i < 10; ++i)
        vec.emplaceBack(i);

    MoveCounted::moves = 0;
    za::insertionSort(vec.begin(), vec.end());
    CHECK(MoveCounted::moves == 0);

    za::quickSort(vec.begin(), vec.end()); // small range: insertion sort only
    CHECK(MoveCounted::moves == 0);

    // One element out of place: moved out, shifted into, and moved back
    za::genericSwap(vec[3], vec[4]);
    MoveCounted::moves = 0;
    za::insertionSort(vec.begin(), vec.end());
    CHECK(MoveCounted::moves == 3);
    CHECK(vec[3].value == 3);
    CHECK(vec[4].value == 4);
}


TEST_CASE("[Base] Algorithm/Sort.hpp - insertionSort is stable")
{
    struct Entry
    {
        int key;
        int order; // position before sorting
    };

    Entry entries[64];
    for (int i = 0; i < 64; ++i)
        entries[i] = {(i * 7) % 5, i}; // many equivalent keys, interleaved

    za::insertionSort(entries, entries + 64, [](const Entry& a, const Entry& b) { return a.key < b.key; });

    bool stable = true;
    for (int i = 1; i < 64; ++i)
    {
        stable &= entries[i - 1].key <= entries[i].key;
        if (entries[i - 1].key == entries[i].key)
            stable &= entries[i - 1].order < entries[i].order;
    }

    CHECK(stable);
}


namespace
{
namespace SortTest // for unity builds
{
////////////////////////////////////////////////////////////
struct Keyed
{
    int key;
    int order; // position before sorting

    [[nodiscard]] bool operator==(const Keyed&) const = default;
};


////////////////////////////////////////////////////////////
const auto byKey = [](const Keyed& a, const Keyed& b) { return a.key < b.key; };


////////////////////////////////////////////////////////////
[[nodiscard]] constexpr bool sortsInConstantExpressions()
{
    int stable[]{5, 3, 9, 1, 7, 2, 8, 0, 6, 4, 5, 3, 9, 1, 7, 2, 8, 0, 6, 4};
    za::stableSort(stable, stable + 20);

    int nth[]{5, 3, 9, 1, 7, 2, 8, 0, 6, 4, 5, 3, 9, 1, 7, 2, 8, 0, 6, 4};
    za::nthElement(nth, nth + 10, nth + 20);

    const int sorted[]{0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9};

    for (int i = 0; i < 20; ++i)
        if (stable[i] != sorted[i])
            return false;

    return nth[10] == 5 && za::upperBound(sorted, sorted + 20, 5) == sorted + 12;
}

} // namespace SortTest
} // namespace


TEST_CASE("[Base] Algorithm/UpperBound.hpp")
{
    const int values[]{1, 2, 2, 2, 5, 8};

    CHECK(za::upperBound(values, values + 6, 0) == values);
    CHECK(za::upperBound(values, values + 6, 1) == values + 1);
    CHECK(za::upperBound(values, values + 6, 2) == values + 4);
    CHECK(za::upperBound(values, values + 6, 3) == values + 4);
    CHECK(za::upperBound(values, values + 6, 8) == values + 6);
    CHECK(za::upperBound(values, values + 6, 9) == values + 6);
    CHECK(za::upperBound(values, values, 2) == values);

    // `lowerBound` and `upperBound` delimit the equivalent elements
    CHECK(za::upperBound(values, values + 6, 2) - za::lowerBound(values, values + 6, 2) == 3);

    // Custom comparator (descending range), called with the value first
    const int descending[]{9, 7, 7, 3};
    CHECK(za::upperBound(descending, descending + 4, 7, greaterCmp) == descending + 3);
}


TEST_CASE("[Base] Algorithm/Sort.hpp - stableSort and nthElement")
{
    STATIC_CHECK(SortTest::sortsInConstantExpressions());

    SortTest::Lcg rng{777u};

    SECTION("stableSort matches insertionSort (both stable), equivalent elements included")
    {
        for (za::SizeT n = 0u; n < 600u; n += (n < 70u ? 1u : 37u))
        {
            for (const unsigned int distinctKeys : {2u, 10u, 1000u})
            {
                za::Vector<SortTest::Keyed> input;
                for (za::SizeT i = 0u; i < n; ++i)
                    input.pushBack({static_cast<int>(rng.next() % distinctKeys), static_cast<int>(i)});

                za::Vector<SortTest::Keyed> byStable    = input;
                za::Vector<SortTest::Keyed> byInsertion = input;

                za::stableSort(byStable.begin(), byStable.end(), SortTest::byKey);
                za::insertionSort(byInsertion.begin(), byInsertion.end(), SortTest::byKey);

                CHECK(byStable == byInsertion);
            }
        }
    }

    SECTION("stableSort on large ranges: sorted, and stable")
    {
        za::Vector<SortTest::Keyed> entries;
        for (int i = 0; i < 20'000; ++i)
            entries.pushBack({static_cast<int>(rng.next() % 100u), i});

        za::stableSort(entries.begin(), entries.end(), SortTest::byKey);

        bool stable = true;
        for (za::SizeT i = 1u; i < entries.size(); ++i)
        {
            stable &= entries[i - 1u].key <= entries[i].key;
            if (entries[i - 1u].key == entries[i].key)
                stable &= entries[i - 1u].order < entries[i].order;
        }

        CHECK(stable);
    }

    SECTION("stableSort: edge cases, custom comparator, non-trivial and move-only elements")
    {
        za::Vector<int> vec;
        za::stableSort(vec.begin(), vec.end());
        CHECK(vec.empty());

        vec = {42};
        za::stableSort(vec.begin(), vec.end());
        CHECK(vec == za::Vector<int>{42});

        vec = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7, 9, 3, 2, 3, 8, 4, 6, 2, 6, 4, 3};
        za::stableSort(vec.begin(), vec.end(), greaterCmp);
        CHECK(za::isSorted(vec.begin(), vec.end(), greaterCmp));

        za::Vector<Person> people;
        for (int i = 0; i < 40; ++i)
            people.pushBack({za::String{i % 2 == 0 ? "Even" : "Odd"}, i});

        za::stableSort(people.begin(), people.end());

        for (int i = 0; i < 40; ++i)
        {
            CHECK(people[static_cast<za::SizeT>(i)].name == (i < 20 ? "Even" : "Odd"));
            CHECK(people[static_cast<za::SizeT>(i)].age == (i < 20 ? 2 * i : 2 * (i - 20) + 1));
        }

        using SortTest::MoveCounted;

        za::Vector<MoveCounted> moveOnly;
        for (int i = 0; i < 100; ++i)
            moveOnly.emplaceBack((i * 37) % 100);

        za::stableSort(moveOnly.begin(), moveOnly.end());

        for (int i = 0; i < 100; ++i)
            CHECK(moveOnly[static_cast<za::SizeT>(i)].value == i);

        // Presorted input: only compared, never moved
        MoveCounted::moves = 0;
        za::stableSort(moveOnly.begin(), moveOnly.end());
        CHECK(MoveCounted::moves == 0);
    }

    SECTION("nthElement puts the right element at `nth`, and partitions around it")
    {
        for (za::SizeT n = 1u; n < 300u; n += (n < 50u ? 1u : 23u))
        {
            for (const unsigned int distinctValues : {3u, 1000u})
            {
                za::Vector<int> input;
                for (za::SizeT i = 0u; i < n; ++i)
                    input.pushBack(static_cast<int>(rng.next() % distinctValues));

                za::Vector<int> sorted = input;
                za::quickSort(sorted.begin(), sorted.end());

                for (za::SizeT k = 0u; k < n; k += (n < 50u ? 1u : 7u))
                {
                    za::Vector<int> partitioned = input;
                    za::nthElement(partitioned.begin(), partitioned.begin() + k, partitioned.end());

                    const int nth = partitioned[k];
                    CHECK(nth == sorted[k]);

                    bool ok = true;
                    for (za::SizeT i = 0u; i < k; ++i)
                        ok &= !(nth < partitioned[i]);
                    for (za::SizeT i = k + 1u; i < n; ++i)
                        ok &= !(partitioned[i] < nth);

                    CHECK(ok);
                }
            }
        }
    }

    SECTION("nthElement: edge cases and custom comparator")
    {
        za::Vector<int> vec;
        za::nthElement(vec.begin(), vec.end(), vec.end());
        CHECK(vec.empty());

        vec = {3, 1, 2};
        za::nthElement(vec.begin(), vec.end(), vec.end()); // `nth == last`: no effect
        CHECK(vec == za::Vector<int>{3, 1, 2});

        vec = {5, 9, 1, 7, 3, 8, 2, 6, 4, 0, 15, 19, 11, 17, 13, 18, 12, 16, 14, 10};
        za::nthElement(vec.begin(), vec.begin() + 2, vec.end(), greaterCmp); // third largest
        CHECK(vec[2] == 17);
    }
}


TEST_CASE("NthElement: O(n log n) comparisons against a killer adversary")
{
    constexpr int n = 4096;

    SortTest::Adversary adversary{n};

    za::Vector<int> items;
    for (int i = 0; i < n; ++i)
        items.pushBack(i);

    za::nthElement(items.begin(), items.begin() + n / 2, items.end(), [&](const int a, const int b) {
        return adversary.less(a, b);
    });

    constexpr long long log2n = 12;
    CHECK(adversary.nComparisons < 8 * n * log2n);
}
