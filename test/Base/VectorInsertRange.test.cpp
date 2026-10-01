#include "StringifyZbStringUtil.hpp" // IWYU pragma: keep
#include "Tst/Tst.hpp"

#include "Zancle/String/String.hpp"

#include "Zancle/Container/Vector.hpp"


TEST_CASE("[Base] Container/Vector.hpp - range insert and assign")
{
    SECTION("insert a range at the beginning, in the middle, and at the end")
    {
        za::Vector<int> vector{1, 2, 3};
        const int       extra[]{7, 8};

        int* it = vector.insert(vector.begin() + 1, extra, extra + 2);
        CHECK(it == vector.begin() + 1);
        CHECK((vector == za::Vector<int>{1, 7, 8, 2, 3}));

        it = vector.insert(vector.begin(), {0});
        CHECK(it == vector.begin());
        CHECK((vector == za::Vector<int>{0, 1, 7, 8, 2, 3}));

        it = vector.insert(vector.end(), {9, 10});
        CHECK(it == vector.end() - 2);
        CHECK((vector == za::Vector<int>{0, 1, 7, 8, 2, 3, 9, 10}));

        // Empty range
        it = vector.insert(vector.begin() + 2, extra, extra);
        CHECK(it == vector.begin() + 2);
        CHECK(vector.size() == 8u);
    }

    SECTION("insert a subrange of the vector itself")
    {
        za::Vector<int> withRoom{1, 2, 3, 4};
        withRoom.reserve(16u); // no growth
        withRoom.insert(withRoom.begin(), withRoom.begin() + 1, withRoom.begin() + 3);
        CHECK((withRoom == za::Vector<int>{2, 3, 1, 2, 3, 4}));

        za::Vector<int> full{1, 2, 3, 4};
        full.shrinkToFit(); // growth: the source is moved while being copied
        full.insert(full.begin() + 2, full.begin(), full.end());
        CHECK((full == za::Vector<int>{1, 2, 1, 2, 3, 4, 3, 4}));
    }

    SECTION("non-trivial elements")
    {
        za::Vector<za::String> vector{"a", "b"};
        const za::String       extra[]{"a string long enough to be on the heap", "y"};

        vector.insert(vector.begin() + 1, extra, extra + 2);
        CHECK((vector == za::Vector<za::String>{"a", "a string long enough to be on the heap", "y", "b"}));

        vector.insert(vector.begin(), {za::String{"first"}, za::String{"second"}});
        CHECK(vector.size() == 6u);
        CHECK(vector[0] == "first");
        CHECK(vector[1] == "second");
        CHECK(vector[5] == "b");
    }

    SECTION("assign")
    {
        za::Vector<int> vector{1, 2, 3};

        vector.assign(5u, 9);
        CHECK((vector == za::Vector<int>{9, 9, 9, 9, 9}));

        vector[0] = 4;
        vector.assign(2u, vector[0]); // an element of the vector itself
        CHECK((vector == za::Vector<int>{4, 4}));

        vector.assign(0u, 1);
        CHECK(vector.empty());

        za::Vector<za::String> strings{"a string long enough to be on the heap"};
        strings.assign(3u, strings[0]);
        CHECK(strings.size() == 3u);
        CHECK(strings[2] == "a string long enough to be on the heap");
    }
}
