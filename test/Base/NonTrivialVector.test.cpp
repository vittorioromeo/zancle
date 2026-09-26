#include "Tst/Tst.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsMoveAssignable.hpp"
#include "Zancle/Trait/IsMoveConstructible.hpp"
#include "Zancle/Trait/IsTrivial.hpp"
#include "Zancle/Trait/IsTriviallyCopyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyConstructible.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyMoveAssignable.hpp"
#include "Zancle/Trait/IsTriviallyMoveConstructible.hpp"


namespace
{
namespace NonTrivialVectorTest // for unity builds
{
int defaultCtorCount = 0;
int intCtorCount     = 0;
int copyCtorCount    = 0;
int moveCtorCount    = 0;
int dtorCount        = 0;
int copyAssignCount  = 0;
int moveAssignCount  = 0;

void resetCounters()
{
    defaultCtorCount = 0;
    intCtorCount     = 0;
    copyCtorCount    = 0;
    moveCtorCount    = 0;
    dtorCount        = 0;
    copyAssignCount  = 0;
    moveAssignCount  = 0;
}

struct Obj
{
    int value = 0;

    Obj()
    {
        ++defaultCtorCount;
    }

    Obj(int x) : value(x)
    {
        ++intCtorCount;
    }

    Obj(const Obj& rhs) : value(rhs.value)
    {
        ++copyCtorCount;
    }

    Obj(Obj&& rhs) noexcept : value(rhs.value)
    {
        ++moveCtorCount;

        rhs.value = 0;
    }

    ~Obj()
    {
        ++dtorCount;
    }

    Obj& operator=(const Obj& rhs)
    {
        if (this == &rhs)
            return *this;

        value = rhs.value;

        ++copyAssignCount;
        return *this;
    }

    Obj& operator=(Obj&& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        value     = rhs.value;
        rhs.value = 0;

        ++moveAssignCount;
        return *this;
    }
};

TEST_CASE("[Base] Base/Vector.hpp")
{
    const auto asConst = [](auto& x) -> const auto& { return x; };

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_TRIVIAL(Obj));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPYABLE(Obj));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(Obj));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(Obj));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(Obj));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(Obj));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(Obj));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Vector<Obj>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPYABLE(za::Vector<Obj>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Vector<Obj>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Vector<Obj>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Vector<Obj>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Vector<Obj>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Vector<Obj>));

        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::Vector<Obj>));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::Vector<Obj>));
        STATIC_CHECK(ZA_IS_MOVE_CONSTRUCTIBLE(za::Vector<Obj>));
        STATIC_CHECK(ZA_IS_MOVE_ASSIGNABLE(za::Vector<Obj>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::Vector<Obj>));
    }

    SECTION("Empty")
    {
#define DO_EMPTY_CHECKS(tv)                  \
    CHECK((tv).begin() == nullptr);          \
    CHECK((tv).end() == nullptr);            \
    CHECK((tv).data() == nullptr);           \
                                             \
    CHECK(asConst((tv)).begin() == nullptr); \
    CHECK(asConst((tv)).end() == nullptr);   \
    CHECK(asConst((tv)).data() == nullptr);  \
                                             \
    CHECK((tv).size() == 0u);                \
    CHECK((tv).empty());

        resetCounters();

        za::Vector<Obj> tv;
        DO_EMPTY_CHECKS(tv);
        CHECK(tv.capacity() == 0u);

        tv.clear();
        DO_EMPTY_CHECKS(tv);
        CHECK(tv.capacity() == 0u);

        za::Vector<Obj> tv2 = tv;
        DO_EMPTY_CHECKS(tv2);
        CHECK(tv2.capacity() == 0u);

        za::Vector<Obj> tv3 = ZA_MOVE(tv);
        DO_EMPTY_CHECKS(tv3);
        CHECK(tv3.capacity() == 0u);

        za::Vector<Obj> tv4;
        tv4 = tv;
        DO_EMPTY_CHECKS(tv4);
        CHECK(tv4.capacity() == 0u);

        za::Vector<Obj> tv5;
        tv5 = ZA_MOVE(tv4);
        DO_EMPTY_CHECKS(tv5);
        CHECK(tv5.capacity() == 0u);

        CHECK(defaultCtorCount == 0);
        CHECK(intCtorCount == 0);
        CHECK(copyCtorCount == 0);
        CHECK(moveCtorCount == 0);
        CHECK(dtorCount == 0);
        CHECK(copyAssignCount == 0);
        CHECK(moveAssignCount == 0);
    }

    SECTION("Non-empty")
    {
        resetCounters();

        {
            za::Vector<Obj> tv;
            DO_EMPTY_CHECKS(tv);
            CHECK(tv.capacity() == 0u);

            CHECK(defaultCtorCount == 0);
            CHECK(intCtorCount == 0);
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 0);
            CHECK(dtorCount == 0);
            CHECK(copyAssignCount == 0);
            CHECK(moveAssignCount == 0);

            tv.reserve(1);
            CHECK(tv.data() != nullptr);
            CHECK(tv.begin() == tv.data());
            CHECK(tv.end() == tv.data() + tv.size());
            CHECK(tv.size() == 0u);
            CHECK(tv.capacity() == 1u);

            CHECK(defaultCtorCount == 0);
            CHECK(intCtorCount == 0);
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 0);
            CHECK(dtorCount == 0);
            CHECK(copyAssignCount == 0);
            CHECK(moveAssignCount == 0);

            tv.unsafeEmplaceBack(42);
            CHECK(tv.data() != nullptr);
            CHECK(tv.begin() == tv.data());
            CHECK(tv.end() == tv.data() + tv.size());
            CHECK(tv.size() == 1u);
            CHECK(tv.capacity() == 1u);
            CHECK(tv[0].value == 42);

            CHECK(defaultCtorCount == 0);
            CHECK(intCtorCount == 1);
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 0);
            CHECK(dtorCount == 0);
            CHECK(copyAssignCount == 0);
            CHECK(moveAssignCount == 0);

            tv.reserveMore(10);

            CHECK(tv.data() != nullptr);
            CHECK(tv.begin() == tv.data());
            CHECK(tv.end() == tv.data() + tv.size());
            CHECK(tv.size() == 1u);
            CHECK(tv.capacity() >= 11u);
            CHECK(tv[0].value == 42);

            CHECK(defaultCtorCount == 0);
            CHECK(intCtorCount == 1);
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 1);
            CHECK(dtorCount == 1);
            CHECK(copyAssignCount == 0);
            CHECK(moveAssignCount == 0);

            tv.resize(100);

            CHECK(defaultCtorCount == 99);
            CHECK(intCtorCount == 1);
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 2);
            CHECK(dtorCount == 2);
            CHECK(copyAssignCount == 0);
            CHECK(moveAssignCount == 0);

            CHECK(tv.data() != nullptr);
            CHECK(tv.begin() == tv.data());
            CHECK(tv.end() == tv.data() + tv.size());
            CHECK(tv.size() == 100u);
            CHECK(tv.capacity() >= 100u);
            CHECK(tv[0].value == 42);

            for (za::SizeT i = 1; i < 100; ++i)
                CHECK(tv[i].value == 0);
        }

        CHECK(defaultCtorCount == 99);
        CHECK(intCtorCount == 1);
        CHECK(copyCtorCount == 0);
        CHECK(moveCtorCount == 2);
        CHECK(dtorCount == 102);
        CHECK(copyAssignCount == 0);
        CHECK(moveAssignCount == 0);
    }

    SECTION("Shrink to fit")
    {
        resetCounters();

        {
            za::Vector<Obj> tv;
            DO_EMPTY_CHECKS(tv);
            CHECK(tv.capacity() == 0u);

            CHECK(defaultCtorCount == 0);
            CHECK(intCtorCount == 0);
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 0);
            CHECK(dtorCount == 0);
            CHECK(copyAssignCount == 0);
            CHECK(moveAssignCount == 0);

            tv.reserve(255);

            CHECK(defaultCtorCount == 0);
            CHECK(intCtorCount == 0);
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 0);
            CHECK(dtorCount == 0);
            CHECK(copyAssignCount == 0);
            CHECK(moveAssignCount == 0);

            for (int i = 0; i < 100; ++i)
                tv.unsafeEmplaceBack(5);

            CHECK(defaultCtorCount == 0);
            CHECK(intCtorCount == 100);
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 0);
            CHECK(dtorCount == 0);
            CHECK(copyAssignCount == 0);
            CHECK(moveAssignCount == 0);

            CHECK(tv.size() == 100);
            CHECK(tv.capacity() > 100);

            tv.shrinkToFit();

            CHECK(defaultCtorCount == 0);
            CHECK(intCtorCount == 100);
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 100);
            CHECK(dtorCount == 100);
            CHECK(copyAssignCount == 0);
            CHECK(moveAssignCount == 0);

            CHECK(tv.size() == 100);
            CHECK(tv.capacity() == 100);
        }

        CHECK(defaultCtorCount == 0);
        CHECK(intCtorCount == 100);
        CHECK(copyCtorCount == 0);
        CHECK(moveCtorCount == 100);
        CHECK(dtorCount == 200);
        CHECK(copyAssignCount == 0);
        CHECK(moveAssignCount == 0);
    }


    SECTION("erase")
    {
        resetCounters();
        za::Vector<Obj> tv;

        // Populate the vector: 10, 20, 30, 40, 50
        tv.emplaceBack(10);
        tv.emplaceBack(20);
        tv.emplaceBack(30);
        tv.emplaceBack(40);
        tv.emplaceBack(50);
        REQUIRE(tv.size() == 5);

        // We expect 5 int constructions from emplaceBack creating temporaries,
        // and 5 move constructions into the vector (assuming some reallocations might occur).
        // Let's reset counters here to focus purely on erase operations.
        resetCounters();

        // --- Erase from the middle (element 30 at index 2) ---
        Obj* itRet = tv.erase(tv.begin() + 2);
        CHECK(tv.size() == 4);
        // Check return value points to the element after the erased one (40)
        REQUIRE(itRet != tv.end());
        CHECK(itRet->value == 40);
        CHECK(itRet == tv.begin() + 2); // 40 is now at index 2
        // Check element values: 10, 20, 40, 50
        CHECK(tv[0].value == 10);
        CHECK(tv[1].value == 20);
        CHECK(tv[2].value == 40);
        CHECK(tv[3].value == 50);
        // Check counters: Expect 2 move assignments (40->30's spot, 50->40's spot)
        // Expect 1 destructor call (for the element at the new end, originally 50)
        CHECK(moveAssignCount == 2);
        CHECK(dtorCount == 1);
        CHECK(defaultCtorCount == 0);
        CHECK(intCtorCount == 0);
        CHECK(copyCtorCount == 0);
        CHECK(moveCtorCount == 0);
        CHECK(copyAssignCount == 0);

        // --- Erase the first element (element 10 at index 0) ---
        resetCounters();
        itRet = tv.erase(tv.begin());
        CHECK(tv.size() == 3);
        // Check return value points to the element after the erased one (20)
        REQUIRE(itRet != tv.end());
        CHECK(itRet->value == 20);
        CHECK(itRet == tv.begin()); // 20 is now at index 0
        // Check element values: 20, 40, 50
        CHECK(tv[0].value == 20);
        CHECK(tv[1].value == 40);
        CHECK(tv[2].value == 50);
        // Check counters: Expect 3 move assignments (20->10, 40->20, 50->40)
        // Expect 1 destructor call (for the element at the new end, originally 50)
        CHECK(moveAssignCount == 3);
        CHECK(dtorCount == 1);
        CHECK(defaultCtorCount == 0);
        CHECK(intCtorCount == 0);
        CHECK(copyCtorCount == 0);
        CHECK(moveCtorCount == 0);
        CHECK(copyAssignCount == 0);


        // --- Erase the last element (element 50 at index 2) ---
        resetCounters();
        itRet = tv.erase(tv.end() - 1); // Equivalent to tv.begin() + 2
        CHECK(tv.size() == 2);
        // Check return value points to end() since last element was erased
        CHECK(itRet == tv.end());
        // Check element values: 20, 40
        CHECK(tv[0].value == 20);
        CHECK(tv[1].value == 40);
        // Check counters: Expect 0 move assignments (no shifting)
        // Expect 1 destructor call (for the erased element 50)
        CHECK(moveAssignCount == 0);
        CHECK(dtorCount == 1);
        CHECK(defaultCtorCount == 0);
        CHECK(intCtorCount == 0);
        CHECK(copyCtorCount == 0);
        CHECK(moveCtorCount == 0);
        CHECK(copyAssignCount == 0);


        // --- Erase remaining elements ---
        // Erase last (40)
        resetCounters();
        itRet = tv.erase(tv.end() - 1);
        CHECK(tv.size() == 1);
        CHECK(itRet == tv.end());
        CHECK(tv[0].value == 20);
        CHECK(moveAssignCount == 0);
        CHECK(dtorCount == 1);

        // Erase first/last (20)
        resetCounters();
        itRet = tv.erase(tv.begin());
        CHECK(tv.size() == 0);
        CHECK(itRet == tv.end()); // Erasing last element returns end()
        CHECK(tv.empty());
        CHECK(moveAssignCount == 0);
        CHECK(dtorCount == 1);


        // --- Edge case: Erase from single-element vector ---
        resetCounters();
        za::Vector<Obj> tvSingle;
        tvSingle.emplaceBack(100); // int:1, move:0 (initially no alloc)
        REQUIRE(tvSingle.size() == 1);
        resetCounters();

        itRet = tvSingle.erase(tvSingle.begin());
        CHECK(tvSingle.empty());
        CHECK(itRet == tvSingle.end());
        CHECK(moveAssignCount == 0);
        CHECK(dtorCount == 1);
    }


    SECTION("emplace")
    {
        SUBCASE("Emplace into empty vector")
        {
            resetCounters();
            za::Vector<Obj> tv;
            tv.reserve(5);
            resetCounters();

            Obj* itRet = tv.emplace(tv.begin(), 42);

            CHECK(tv.size() == 1);
            CHECK(tv[0].value == 42);
            CHECK(itRet == tv.begin());
            // Emplace at end: direct construction, no temporary needed.
            CHECK(intCtorCount == 1);
            CHECK(moveCtorCount == 0);
            CHECK(moveAssignCount == 0);
            CHECK(dtorCount == 0);
        }

        SUBCASE("Emplace at the end with capacity")
        {
            resetCounters();
            za::Vector<Obj> tv;
            tv.emplaceBack(10);
            tv.emplaceBack(20);
            tv.reserve(5);
            resetCounters();

            Obj* itRet = tv.emplace(tv.end(), 30);

            CHECK(tv.size() == 3);
            CHECK(tv[0].value == 10);
            CHECK(tv[1].value == 20);
            CHECK(tv[2].value == 30);
            CHECK(itRet == tv.begin() + 2);

            // Emplace at end: direct construction, no temporary needed.
            CHECK(intCtorCount == 1);
            CHECK(moveCtorCount == 0);
            CHECK(moveAssignCount == 0);
            CHECK(dtorCount == 0);
        }

        SUBCASE("Emplace in the middle with capacity")
        {
            resetCounters();
            za::Vector<Obj> tv;
            tv.emplaceBack(10);
            tv.emplaceBack(30);
            tv.emplaceBack(40);
            tv.reserve(5);
            REQUIRE(tv.size() == 3);
            resetCounters();

            Obj* itRet = tv.emplace(tv.begin() + 1, 20);

            CHECK(tv.size() == 4);
            CHECK(tv[0].value == 10);
            CHECK(tv[1].value == 20); // new element
            CHECK(tv[2].value == 30);
            CHECK(tv[3].value == 40);
            CHECK(itRet == tv.begin() + 1);

            // Analysis: Shift 2 elements (30, 40) + temporary for new element
            // 1. Construct temporary from int arg: 1 int-ctor.
            // 2. Move-construct '40' to new end: 1 move-ctor.
            // 3. Move-assign '30' over old '40': 1 move-assign.
            // 4. Destroy moved-from at insertion point: 1 dtor.
            // 5. Move-construct from temporary into position: 1 move-ctor.
            // 6. Destroy temporary: 1 dtor.
            CHECK(intCtorCount == 1);
            CHECK(moveCtorCount == 2);
            CHECK(moveAssignCount == 1);
            CHECK(dtorCount == 2);
        }

        SUBCASE("Emplace at the beginning with capacity")
        {
            resetCounters();
            za::Vector<Obj> tv;
            tv.emplaceBack(20);
            tv.emplaceBack(30);
            tv.reserve(5);
            REQUIRE(tv.size() == 2);
            resetCounters();

            Obj* itRet = tv.emplace(tv.begin(), 10);

            CHECK(tv.size() == 3);
            CHECK(tv[0].value == 10);
            CHECK(tv[1].value == 20);
            CHECK(tv[2].value == 30);
            CHECK(itRet == tv.begin());

            // Analysis: Shift 2 elements (20, 30) + temporary for new element
            // 1. Construct temporary from int arg: 1 int-ctor.
            // 2. Move-construct '30' to new end: 1 move-ctor.
            // 3. Move-assign '20' over old '30': 1 move-assign.
            // 4. Destroy moved-from at insertion point: 1 dtor.
            // 5. Move-construct from temporary into position: 1 move-ctor.
            // 6. Destroy temporary: 1 dtor.
            CHECK(intCtorCount == 1);
            CHECK(moveCtorCount == 2);
            CHECK(moveAssignCount == 1);
            CHECK(dtorCount == 2);
        }

        SUBCASE("Emplace in the middle with reallocation")
        {
            resetCounters();
            za::Vector<Obj> tv;
            tv.emplaceBack(10);
            tv.emplaceBack(30);
            tv.shrinkToFit(); // size == 2, capacity == 2
            REQUIRE(tv.size() == 2);
            REQUIRE(tv.capacity() == 2);
            resetCounters();

            Obj* itRet = tv.emplace(tv.begin() + 1, 20);

            CHECK(tv.size() == 3);
            CHECK(tv.capacity() >= 3);
            CHECK(tv[0].value == 10);
            CHECK(tv[1].value == 20);
            CHECK(tv[2].value == 30);
            CHECK(itRet == tv.begin() + 1);

            // Analysis: N=2 elements, growAndEmplace path
            // 1. Construct '20' directly in new buffer: 1 int-constructor.
            // 2. Relocate '10' (before insert pos): 1 move-construct, 1 destructor.
            // 3. Relocate '30' (after insert pos): 1 move-construct, 1 destructor.
            CHECK(intCtorCount == 1);
            CHECK(moveCtorCount == 2);
            CHECK(moveAssignCount == 0);
            CHECK(dtorCount == 2);
        }

        SUBCASE("Emplace at the end with reallocation")
        {
            resetCounters();
            za::Vector<Obj> tv;
            tv.emplaceBack(10);
            tv.emplaceBack(20);
            tv.shrinkToFit(); // size == 2, capacity == 2
            REQUIRE(tv.size() == 2);
            REQUIRE(tv.capacity() == 2);
            resetCounters();

            Obj* itRet = tv.emplace(tv.end(), 30);

            CHECK(tv.size() == 3);
            CHECK(tv.capacity() >= 3);
            CHECK(tv[0].value == 10);
            CHECK(tv[1].value == 20);
            CHECK(tv[2].value == 30);
            CHECK(itRet == tv.begin() + 2);

            // Analysis: N=2 elements
            // 1. Reallocation: 2 move-constructs, 2 destructors.
            // 2. No shifts needed for emplace at end.
            // 3. In-place construct '30': 1 int-constructor.
            CHECK(intCtorCount == 1);
            CHECK(moveCtorCount == 2);
            CHECK(moveAssignCount == 0);
            CHECK(dtorCount == 2);
        }
    }

    SECTION("Self-aliasing: pushBack from own element without reallocation")
    {
        za::Vector<Obj> v;
        v.reserve(10);
        v.emplaceBack(10);
        v.emplaceBack(20);
        v.emplaceBack(30);

        v.pushBack(v[0]);

        CHECK(v.size() == 4);
        CHECK(v[0].value == 10);
        CHECK(v[1].value == 20);
        CHECK(v[2].value == 30);
        CHECK(v[3].value == 10);
    }

    SECTION("Self-aliasing: pushBack from own element with reallocation")
    {
        za::Vector<Obj> v;
        v.emplaceBack(10);
        v.emplaceBack(20);
        v.emplaceBack(30);
        v.shrinkToFit();
        REQUIRE(v.capacity() == v.size()); // next pushBack must reallocate

        v.pushBack(v[0]);

        CHECK(v.size() == 4);
        CHECK(v[0].value == 10);
        CHECK(v[1].value == 20);
        CHECK(v[2].value == 30);
        CHECK(v[3].value == 10);
    }

    SECTION("Self-aliasing: insert at begin from last element")
    {
        za::Vector<Obj> v;
        v.reserve(10);
        v.emplaceBack(10);
        v.emplaceBack(20);
        v.emplaceBack(30);

        v.insert(v.begin(), v[2]);

        CHECK(v.size() == 4);
        CHECK(v[0].value == 30);
        CHECK(v[1].value == 10);
        CHECK(v[2].value == 20);
        CHECK(v[3].value == 30);
    }

    SECTION("Self-aliasing: insert at middle from element that gets shifted")
    {
        za::Vector<Obj> v;
        v.reserve(10);
        v.emplaceBack(10);
        v.emplaceBack(20);
        v.emplaceBack(30);

        v.insert(v.begin() + 1, v[2]);

        CHECK(v.size() == 4);
        CHECK(v[0].value == 10);
        CHECK(v[1].value == 30);
        CHECK(v[2].value == 20);
        CHECK(v[3].value == 30);
    }

    SECTION("Self-aliasing: emplace at begin from back()")
    {
        za::Vector<Obj> v;
        v.reserve(10);
        v.emplaceBack(10);
        v.emplaceBack(20);
        v.emplaceBack(30);

        const Obj& backRef = v.back();
        v.emplace(v.begin(), backRef);

        CHECK(v.size() == 4);
        CHECK(v[0].value == 30);
        CHECK(v[1].value == 10);
        CHECK(v[2].value == 20);
        CHECK(v[3].value == 30);
    }

    SECTION("Self-aliasing: emplaceBack from own element with reallocation")
    {
        za::Vector<Obj> v;
        v.emplaceBack(10);
        v.emplaceBack(20);
        v.emplaceBack(30);
        v.shrinkToFit();
        REQUIRE(v.capacity() == v.size());

        const Obj& ref = v[1];
        v.emplaceBack(ref);

        CHECK(v.size() == 4);
        CHECK(v[0].value == 10);
        CHECK(v[1].value == 20);
        CHECK(v[2].value == 30);
        CHECK(v[3].value == 20);
    }
}

} // namespace NonTrivialVectorTest
} // namespace
