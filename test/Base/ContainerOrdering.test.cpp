#include "Tst/Tst.hpp"

#include "Zancle/Algorithm/Sort.hpp"

#include "Zancle/Container/Array.hpp"
#include "Zancle/Container/InPlaceVector.hpp"
#include "Zancle/Container/SmallVector.hpp"
#include "Zancle/Container/Vector.hpp"


namespace
{
namespace ContainerOrderingTest // for unity builds
{
////////////////////////////////////////////////////////////
// Only has `<` (and `==`): the ordering operators must not need anything else
struct OnlyLess
{
    int value;

    [[nodiscard]] constexpr bool operator==(const OnlyLess&) const = default;

    [[nodiscard]] constexpr bool operator<(const OnlyLess& rhs) const
    {
        return value < rhs.value;
    }
};


////////////////////////////////////////////////////////////
static_assert(za::Array{1, 2, 3} < za::Array{1, 2, 4});
static_assert(za::Array{1, 2, 3} < za::Array{2, 0, 0});
static_assert(!(za::Array{1, 2, 3} < za::Array{1, 2, 3}));
static_assert(za::Array{1, 2, 3} <= za::Array{1, 2, 3});
static_assert(za::Array{1, 2, 3} >= za::Array{1, 2, 3});
static_assert(za::Array{1, 3, 0} > za::Array{1, 2, 9});
static_assert(za::Array{OnlyLess{1}, OnlyLess{2}} < za::Array{OnlyLess{1}, OnlyLess{3}});


////////////////////////////////////////////////////////////
template <typename V>
void checkVectorOrdering()
{
    CHECK(V{1, 2, 3} < V{1, 2, 4});
    CHECK(V{1, 2, 3} < V{2});
    CHECK(V{1, 2} < V{1, 2, 0}); // a prefix is less
    CHECK(V{} < V{0});
    CHECK(!(V{} < V{}));
    CHECK(!(V{1, 2, 3} < V{1, 2, 3}));
    CHECK(!(V{1, 2, 0} < V{1, 2}));

    CHECK(V{1, 2, 4} > V{1, 2, 3});
    CHECK(V{1, 2} <= V{1, 2});
    CHECK(V{1, 2} <= V{1, 3});
    CHECK(!(V{1, 3} <= V{1, 2}));
    CHECK(V{1, 2} >= V{1, 2});
    CHECK(V{1, 2, 0} >= V{1, 2});
    CHECK(!(V{1, 2} >= V{1, 2, 0}));
}

} // namespace ContainerOrderingTest
} // namespace


TEST_CASE("[Base] Container ordering operators")
{
    SECTION("Array")
    {
        const za::Array a{3, 1, 2};
        const za::Array b{3, 1, 5};

        CHECK(a < b);
        CHECK(b > a);
        CHECK(a <= b);
        CHECK(b >= a);
        CHECK(!(b < a));
    }

    SECTION("Vector, SmallVector, InPlaceVector")
    {
        ContainerOrderingTest::checkVectorOrdering<za::Vector<int>>();
        ContainerOrderingTest::checkVectorOrdering<za::SmallVector<int, 2>>(); // inline and heap storage
        ContainerOrderingTest::checkVectorOrdering<za::InPlaceVector<int, 4>>();

        CHECK(za::Vector<ContainerOrderingTest::OnlyLess>{{1}, {2}} <
              za::Vector<ContainerOrderingTest::OnlyLess>{{1}, {3}});
    }

    SECTION("A vector of vectors sorts lexicographically")
    {
        za::Vector<za::Vector<int>> rows{{2, 1}, {1, 9, 9}, {2}, {}, {1, 9}};
        za::quickSort(rows.begin(), rows.end());

        CHECK(rows == za::Vector<za::Vector<int>>{{}, {1, 9}, {1, 9, 9}, {2}, {2, 1}});
    }
}
