#include "Tst/Tst.hpp"

#include "Zancle/Base/ReverseIterator.hpp"

#include "Zancle/Container/Array.hpp"
#include "Zancle/Container/Vector.hpp"

#include "Zancle/Vocabulary/Span.hpp"

#include <algorithm>


namespace
{
namespace ReverseIteratorTest // for unity builds
{
////////////////////////////////////////////////////////////
template <typename Range>
concept CanReverse = requires(Range&& range) { za::reversed(static_cast<Range&&>(range)); };


////////////////////////////////////////////////////////////
[[nodiscard]] constexpr int reversedDigits()
{
    const za::Array<int, 4> array{1, 2, 3, 4};

    int result = 0;
    for (const int digit : za::reversed(array))
        result = result * 10 + digit;

    return result;
}

} // namespace ReverseIteratorTest
} // namespace


TEST_CASE("[Base] Base/ReverseIterator.hpp")
{
    SECTION("reversed")
    {
        za::Vector<int> vector{1, 2, 3, 4};

        za::Vector<int> seen;
        for (const int x : za::reversed(vector))
            seen.pushBack(x);

        CHECK((seen == za::Vector<int>{4, 3, 2, 1}));

        // Mutable elements, and C-style arrays
        int array[]{1, 2, 3};
        int factor = 1;
        for (int& x : za::reversed(array))
        {
            x *= factor;
            factor *= 10;
        }

        CHECK(array[0] == 100);
        CHECK(array[1] == 20);
        CHECK(array[2] == 3);

        // Constant expressions
        STATIC_CHECK(ReverseIteratorTest::reversedDigits() == 4321);

        // Only lvalues: a temporary container would be destroyed before the loop runs
        STATIC_CHECK(ReverseIteratorTest::CanReverse<za::Vector<int>&>);
        STATIC_CHECK(ReverseIteratorTest::CanReverse<const za::Vector<int>&>);
        STATIC_CHECK(!ReverseIteratorTest::CanReverse<za::Vector<int>>);
    }

    SECTION("Random access")
    {
        za::Vector<int> vector{1, 2, 3, 4};

        auto first = za::rbegin(vector);
        auto last  = za::rend(vector);

        CHECK(*first == 4);
        CHECK(first[1] == 3);
        CHECK(*(first + 2) == 2);
        CHECK(*(2 + first) == 2);
        CHECK(*(last - 1) == 1);
        CHECK(last - first == 4);
        CHECK(first < last);
        CHECK(last > first);
        CHECK(first.base() == vector.end());
        CHECK(last.base() == vector.begin());

        ++first;
        CHECK(*first == 3);
        --first;
        CHECK(*first == 4);

        za::Vector<int> empty;
        CHECK(za::rbegin(empty) == za::rend(empty));

        // Usable by standard algorithms: sorting the reversed range sorts the vector in descending order
        za::Vector<int> numbers{3, 1, 4, 1, 5, 9, 2, 6};
        std::sort(za::rbegin(numbers), za::rend(numbers));
        CHECK((numbers == za::Vector<int>{9, 6, 5, 4, 3, 2, 1, 1}));
    }

    SECTION("Spans and constant containers")
    {
        int values[]{1, 2, 3};

        const za::Span<int> span{values};
        CHECK(*za::rbegin(span) == 3);

        const za::Vector<int> vector{5, 6};
        int                   sum = 0;
        for (const int x : za::reversed(vector))
            sum = sum * 10 + x;

        CHECK(sum == 65);
    }
}
