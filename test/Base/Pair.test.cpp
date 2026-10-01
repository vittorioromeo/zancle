#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/Pair.hpp"

#include "Zancle/String/String.hpp"

#include "Zancle/Algorithm/LowerBound.hpp"
#include "Zancle/Algorithm/Sort.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"


TEST_CASE("[Base] Vocabulary/Pair.hpp")
{
    SECTION("Aggregate, deduction, and makePair")
    {
        STATIC_CHECK(ZA_IS_AGGREGATE(za::Pair<int, float>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Pair<int, float>));

        constexpr za::Pair p{1, 2.f};
        STATIC_CHECK(za::isSame<decltype(p), const za::Pair<int, float>>);
        STATIC_CHECK(p.first == 1);
        STATIC_CHECK(p.second == 2.f);

        const char text[] = "abc";
        const int  i      = 5;
        const auto made   = za::makePair(i, text); // decays: `const int` to `int`, array to pointer
        STATIC_CHECK(za::isSame<decltype(made), const za::Pair<int, const char*>>);
        CHECK(made.first == 5);
        CHECK(made.second == text);

        const auto [first, second] = za::makePair(za::String{"key"}, 7);
        CHECK(first == "key");
        CHECK(second == 7);

        STATIC_CHECK(za::Pair<int, int>{} == za::Pair{0, 0});
    }

    SECTION("Comparisons: by `first`, then by `second`")
    {
        STATIC_CHECK(za::Pair{1, 2} == za::Pair{1, 2});
        STATIC_CHECK(za::Pair{1, 2} != za::Pair{1, 3});

        STATIC_CHECK(za::Pair{1, 9} < za::Pair{2, 0});
        STATIC_CHECK(za::Pair{1, 2} < za::Pair{1, 3});
        STATIC_CHECK(!(za::Pair{1, 3} < za::Pair{1, 3}));
        STATIC_CHECK(!(za::Pair{2, 0} < za::Pair{1, 9}));

        STATIC_CHECK(za::Pair{2, 0} > za::Pair{1, 9});
        STATIC_CHECK(za::Pair{1, 3} <= za::Pair{1, 3});
        STATIC_CHECK(za::Pair{1, 2} <= za::Pair{1, 3});
        STATIC_CHECK(za::Pair{1, 3} >= za::Pair{1, 3});
        STATIC_CHECK(!(za::Pair{1, 2} >= za::Pair{1, 3}));
    }

    SECTION("Sorted and binary searched as (key, index) pairs")
    {
        za::Vector<za::Pair<int, int>> entries{{3, 0}, {1, 1}, {3, 2}, {2, 3}, {1, 4}};
        za::quickSort(entries.begin(), entries.end());

        CHECK(entries == za::Vector<za::Pair<int, int>>{{1, 1}, {1, 4}, {2, 3}, {3, 0}, {3, 2}});

        const auto* it = za::lowerBound(entries.begin(), entries.end(), za::makePair(3, -1));
        CHECK(it == entries.begin() + 3);
    }
}
