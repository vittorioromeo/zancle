#include "Tst/Tst.hpp"

#include "Zancle/Container/InPlaceVector.hpp"

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
namespace NonTrivialInPlaceVectorTest // for unity builds
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

    // Added for operator== tests (operator!= is synthesized by C++20 rewriting)
    bool operator==(const Obj& rhs) const
    {
        return value == rhs.value;
    }
};

constexpr za::SizeT defaultCapacity = 10; // Define a capacity for InPlaceVector

TEST_CASE("[Base] Base/InPlaceVector.hpp")
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

        // InPlaceVector itself is not trivial due to custom destructor and copy/move ops
        STATIC_CHECK(!ZA_IS_TRIVIAL(za::InPlaceVector<Obj, defaultCapacity>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPYABLE(za::InPlaceVector<Obj, defaultCapacity>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::InPlaceVector<Obj, defaultCapacity>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::InPlaceVector<Obj, defaultCapacity>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::InPlaceVector<Obj, defaultCapacity>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::InPlaceVector<Obj, defaultCapacity>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::InPlaceVector<Obj, defaultCapacity>));

        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::InPlaceVector<Obj, defaultCapacity>));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::InPlaceVector<Obj, defaultCapacity>));
        STATIC_CHECK(ZA_IS_MOVE_CONSTRUCTIBLE(za::InPlaceVector<Obj, defaultCapacity>));
        STATIC_CHECK(ZA_IS_MOVE_ASSIGNABLE(za::InPlaceVector<Obj, defaultCapacity>));

        STATIC_CHECK(!ZA_IS_TRIVIALLY_RELOCATABLE(za::InPlaceVector<Obj, defaultCapacity>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::InPlaceVector<int, defaultCapacity>));
    }

    SECTION("Empty")
    {
#define DO_EMPTY_CHECKS_IPV(tv)                                      \
    CHECK((tv).begin() == (tv).data());                              \
    CHECK((tv).end() == (tv).data());                                \
    CHECK((tv).data() != nullptr); /* Storage is always allocated */ \
                                                                     \
    CHECK(asConst((tv)).begin() == asConst((tv)).data());            \
    CHECK(asConst((tv)).end() == asConst((tv)).data());              \
    CHECK(asConst((tv)).data() != nullptr);                          \
                                                                     \
    CHECK((tv).size() == 0u);                                        \
    CHECK((tv).empty());

        resetCounters();

        za::InPlaceVector<Obj, defaultCapacity> tv;
        DO_EMPTY_CHECKS_IPV(tv);
        CHECK(tv.capacity() == defaultCapacity);

        tv.clear();
        DO_EMPTY_CHECKS_IPV(tv);
        CHECK(tv.capacity() == defaultCapacity);

        za::InPlaceVector<Obj, defaultCapacity> tv2 = tv;
        DO_EMPTY_CHECKS_IPV(tv2);
        CHECK(tv2.capacity() == defaultCapacity);
        CHECK(copyCtorCount == 0); // No elements to copy

        za::InPlaceVector<Obj, defaultCapacity> tv3 = ZA_MOVE(tv);
        DO_EMPTY_CHECKS_IPV(tv3);
        CHECK(tv3.capacity() == defaultCapacity);
        CHECK(moveCtorCount == 0); // No elements to move

        za::InPlaceVector<Obj, defaultCapacity> tv4;
        tv4 = tv;
        DO_EMPTY_CHECKS_IPV(tv4);
        CHECK(tv4.capacity() == defaultCapacity);
        CHECK(copyAssignCount == 0); // No elements to copy assign

        za::InPlaceVector<Obj, defaultCapacity> tv5;
        tv5 = ZA_MOVE(tv4);
        DO_EMPTY_CHECKS_IPV(tv5);
        CHECK(tv5.capacity() == defaultCapacity);
        CHECK(moveAssignCount == 0); // No elements to move assign

        CHECK(defaultCtorCount == 0);
        CHECK(intCtorCount == 0);
        // copy/move ctor counts already checked above
        CHECK(dtorCount == 0);
        // copy/move assign counts already checked above
    }

    SECTION("Constructors")
    {
        resetCounters();
        {
            za::InPlaceVector<Obj, 5> tv(3); // Default construct 3 Objs
            CHECK(tv.size() == 3);
            CHECK(defaultCtorCount == 3);
            for (za::SizeT i = 0; i < 3; ++i)
                CHECK(tv[i].value == 0);
        }
        CHECK(dtorCount == 3);

        resetCounters();
        {
            za::InPlaceVector<Obj, 5> tv(2, Obj(77)); // Construct 2 Objs with value 77
            CHECK(tv.size() == 2);
            CHECK(intCtorCount == 1);  // For temporary Obj(77)
            CHECK(copyCtorCount == 2); // Copying Obj(77) into vector
            CHECK(dtorCount == 1);     // For temporary Obj(77)
            for (za::SizeT i = 0; i < 2; ++i)
                CHECK(tv[i].value == 77);
        }
        CHECK(dtorCount == 1 + 2); // 1 temp + 2 in vector

        resetCounters();
        {
            Obj source[] = {Obj(1), Obj(2), Obj(3)};
            CHECK(intCtorCount == 3);
            resetCounters(); // Focus on vector construction

            za::InPlaceVector<Obj, 5> tv(source, source + 3);
            CHECK(tv.size() == 3);
            CHECK(copyCtorCount == 3); // Copying from source array
            CHECK(tv[0].value == 1);
            CHECK(tv[1].value == 2);
            CHECK(tv[2].value == 3);
        }
        CHECK(dtorCount == 6); // For 3 elements in vector

        resetCounters();
        {
            za::InPlaceVector<Obj, 5> tv{Obj(10), Obj(20)};
            CHECK(tv.size() == 2);
            CHECK(intCtorCount == 2);  // For temporaries in initializer list
            CHECK(copyCtorCount == 2); // For copying from initializer list (if not elided)
                                       // OR moveCtorCount == 2 if elements are moved from list proxy
                                       // Current impl uses copyRange, so copyCtorCount is expected.
            CHECK(dtorCount == 2);     // For temporaries in initializer list
            CHECK(tv[0].value == 10);
            CHECK(tv[1].value == 20);
        }
        CHECK(dtorCount == 2 + 2); // 2 temps + 2 in vector
    }


    SECTION("Non-empty, operations within capacity")
    {
        resetCounters();
        {
            za::InPlaceVector<Obj, defaultCapacity> tv;
            DO_EMPTY_CHECKS_IPV(tv);
            CHECK(tv.capacity() == defaultCapacity);

            tv.reserve(defaultCapacity); // Should be no-op
            CHECK(tv.capacity() == defaultCapacity);
            CHECK(tv.size() == 0u);
            CHECK(defaultCtorCount == 0);
            CHECK(intCtorCount == 0);
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 0);
            CHECK(dtorCount == 0);

            tv.unsafeEmplaceBack(42);
            CHECK(tv.data() != nullptr);
            CHECK(tv.begin() == tv.data());
            CHECK(tv.end() == tv.data() + tv.size());
            CHECK(tv.size() == 1u);
            CHECK(tv.capacity() == defaultCapacity);
            CHECK(tv[0].value == 42);
            CHECK(intCtorCount == 1);

            tv.reserveMore(defaultCapacity - tv.size()); // Should be no-op if enough capacity
            CHECK(tv.capacity() == defaultCapacity);
            CHECK(tv.size() == 1u);
            CHECK(tv[0].value == 42);
            // No reallocations, so no moves/copies
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 0);
            CHECK(dtorCount == 0);

            tv.resize(5); // Default construct 4 more
            CHECK(tv.size() == 5u);
            CHECK(tv.capacity() == defaultCapacity);
            CHECK(tv[0].value == 42);
            for (za::SizeT i = 1; i < 5; ++i)
                CHECK(tv[i].value == 0);
            CHECK(defaultCtorCount == 4);

            tv.resize(2); // Shrink, destroy 3
            CHECK(tv.size() == 2u);
            CHECK(tv.capacity() == defaultCapacity);
            CHECK(tv[0].value == 42);
            CHECK(tv[1].value == 0); // The second element remains
            CHECK(dtorCount == 3);
        }
        // At scope end: 1 Obj(42) + 1 Obj() destroyed
        CHECK(dtorCount == 3 + 2);
        CHECK(defaultCtorCount == 4);
        CHECK(intCtorCount == 1);
        CHECK(copyCtorCount == 0);
        CHECK(moveCtorCount == 0);
    }

    SECTION("Shrink to fit (no-op)")
    {
        resetCounters();
        {
            za::InPlaceVector<Obj, defaultCapacity> tv;
            for (za::SizeT i = 0; i < defaultCapacity / 2; ++i)
                tv.unsafeEmplaceBack(5);

            CHECK(intCtorCount == defaultCapacity / 2);
            CHECK(tv.size() == defaultCapacity / 2);
            CHECK(tv.capacity() == defaultCapacity);

            tv.shrinkToFit(); // Should be a no-op for InPlaceVector

            CHECK(intCtorCount == defaultCapacity / 2); // No change
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 0);
            CHECK(dtorCount == 0);
            CHECK(tv.size() == defaultCapacity / 2);
            CHECK(tv.capacity() == defaultCapacity); // Capacity unchanged
        }
        CHECK(dtorCount == defaultCapacity / 2); // Elements destroyed at scope end
    }

    SECTION("erase")
    {
        za::InPlaceVector<Obj, 10> tv; // Capacity of 10

        // Populate the vector: 10, 20, 30, 40, 50
        tv.emplaceBack(10);
        tv.emplaceBack(20);
        tv.emplaceBack(30);
        tv.emplaceBack(40);
        tv.emplaceBack(50);
        REQUIRE(tv.size() == 5);
        // Initial emplaceBacks: 5 intCtorCount, 0 moveCtorCount (direct construction)
        resetCounters();

        // --- Erase from the middle (element 30 at index 2) ---
        Obj* itRet = tv.erase(tv.begin() + 2);
        CHECK(tv.size() == 4);
        REQUIRE(itRet != tv.end());
        CHECK(itRet->value == 40);
        CHECK(itRet == tv.begin() + 2);
        CHECK(tv[0].value == 10);
        CHECK(tv[1].value == 20);
        CHECK(tv[2].value == 40);
        CHECK(tv[3].value == 50);
        // Check counters: Expect 2 move assignments (40->30's spot, 50->40's spot)
        // Expect 1 destructor call (for the element at the old m_size, which was 50's original content)
        CHECK(moveAssignCount == 2); // 40 assigned to tv[2], 50 assigned to tv[3]
        CHECK(dtorCount == 1);       // Element at tv.data() + new_m_size (original 50) is destroyed
        CHECK(defaultCtorCount == 0);
        CHECK(intCtorCount == 0);
        CHECK(copyCtorCount == 0);
        CHECK(moveCtorCount == 0);
        CHECK(copyAssignCount == 0);


        // --- Erase the first element (element 10 at index 0) ---
        resetCounters();
        itRet = tv.erase(tv.begin());
        CHECK(tv.size() == 3);
        REQUIRE(itRet != tv.end());
        CHECK(itRet->value == 20);
        CHECK(itRet == tv.begin());
        CHECK(tv[0].value == 20);
        CHECK(tv[1].value == 40);
        CHECK(tv[2].value == 50);
        // 3 move assignments (20->10, 40->20, 50->40)
        // 1 destructor call (for the element at the new end, originally 50)
        CHECK(moveAssignCount == 3);
        CHECK(dtorCount == 1);


        // --- Erase the last element (element 50 at index 2) ---
        resetCounters();
        itRet = tv.erase(tv.end() - 1);
        CHECK(tv.size() == 2);
        CHECK(itRet == tv.end());
        CHECK(tv[0].value == 20);
        CHECK(tv[1].value == 40);
        // 0 move assignments (no shifting)
        // 1 destructor call (for the erased element 50)
        CHECK(moveAssignCount == 0);
        CHECK(dtorCount == 1);


        // --- Erase remaining elements ---
        resetCounters();
        itRet = tv.erase(tv.end() - 1); // Erase 40
        CHECK(tv.size() == 1);
        CHECK(itRet == tv.end());
        CHECK(tv[0].value == 20);
        CHECK(moveAssignCount == 0);
        CHECK(dtorCount == 1);

        resetCounters();
        itRet = tv.erase(tv.begin()); // Erase 20
        CHECK(tv.size() == 0);
        CHECK(itRet == tv.end());
        CHECK(tv.empty());
        CHECK(moveAssignCount == 0);
        CHECK(dtorCount == 1);


        // --- Edge case: Erase from single-element vector ---
        resetCounters();
        za::InPlaceVector<Obj, 1> tvSingle;
        tvSingle.emplaceBack(100);
        REQUIRE(tvSingle.size() == 1);
        CHECK(intCtorCount == 1);
        resetCounters();

        itRet = tvSingle.erase(tvSingle.begin());
        CHECK(tvSingle.empty());
        CHECK(itRet == tvSingle.end());
        CHECK(moveAssignCount == 0);
        CHECK(dtorCount == 1);
    }

    SECTION("erase range")
    {
        za::InPlaceVector<Obj, 10> tv;
        // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
        for (int i = 0; i < 10; ++i)
            tv.emplaceBack(i);
        REQUIRE(tv.size() == 10);
        CHECK(intCtorCount == 10);
        resetCounters();

        // Erase [2, 3, 4] -> elements with value 2, 3, 4
        // Vector becomes: 0, 1, 5, 6, 7, 8, 9
        Obj* itRet = tv.erase(tv.begin() + 2, tv.begin() + 5);
        CHECK(tv.size() == 7);
        REQUIRE(itRet == tv.begin() + 2);
        CHECK(itRet->value == 5); // Element 5 is now at index 2
        int expectedValues1[] = {0, 1, 5, 6, 7, 8, 9};
        for (za::SizeT i = 0; i < tv.size(); ++i)
            CHECK(tv[i].value == expectedValues1[i]);

        CHECK(moveAssignCount == 5); // 5,6,7,8,9 are moved
        CHECK(dtorCount == 3);

        resetCounters();
        // Erase all remaining: 0, 1, 5, 6, 7, 8, 9
        itRet = tv.erase(tv.begin(), tv.end());
        CHECK(tv.empty());
        CHECK(itRet == tv.begin()); // eraseRange returns first if erasing to end
        CHECK(dtorCount == 7);
        CHECK(moveAssignCount == 0);

        // Erase empty range
        resetCounters();
        tv.emplaceBack(1);                                // {1}
        itRet = tv.erase(tv.begin() + 1, tv.begin() + 1); // Erase empty range at end
        CHECK(tv.size() == 1);
        CHECK(itRet == tv.begin() + 1);
        CHECK(dtorCount == 0);
        CHECK(moveAssignCount == 0);
    }

    SECTION("Swap")
    {
        resetCounters();
        za::InPlaceVector<Obj, 5> v1;
        v1.emplaceBack(1); // {1}
        v1.emplaceBack(2); // {1, 2}
        za::InPlaceVector<Obj, 5> v2;
        v2.emplaceBack(10); // {10}
        v2.emplaceBack(20); // {10, 20}
        v2.emplaceBack(30); // {10, 20, 30}

        CHECK(intCtorCount == 5); // 2 for v1, 3 for v2
        resetCounters();

        v1.swap(v2);

        CHECK(v1.size() == 3);
        CHECK(v1[0].value == 10);
        CHECK(v1[1].value == 20);
        CHECK(v1[2].value == 30);
        CHECK(v2.size() == 2);
        CHECK(v2[0].value == 1);
        CHECK(v2[1].value == 2);

        CHECK(moveCtorCount == (2 * 1) + 1); // 2 for swap of common, 1 for extra element
        CHECK(moveAssignCount == (2 * 2));   // 4 for swap of common
        CHECK(dtorCount == 3);               // For the moved-from Obj(30) in v2 storage

        // Test swap with self (no-op)
        resetCounters();
        v1.swap(v1);
        CHECK(v1.size() == 3);
        CHECK(v1[0].value == 10);
        CHECK(moveCtorCount == 0);
        CHECK(moveAssignCount == 0);
        CHECK(dtorCount == 0);

        // Test swap with empty vector
        resetCounters();
        za::InPlaceVector<Obj, 5> vEmpty;
        v1.swap(vEmpty); // v1: {10,20,30}, vEmpty: {}
        CHECK(v1.empty());
        CHECK(vEmpty.size() == 3);
        CHECK(vEmpty[0].value == 10);
        // 3 elements moved from v1 to vEmpty, 3 moved-from in v1 destroyed
        CHECK(moveCtorCount == 3);
        CHECK(moveAssignCount == 0); // No common part for za::swap
        CHECK(dtorCount == 3);
    }

    SECTION("Comparison operators")
    {
        za::InPlaceVector<Obj, 5> v1;
        v1.emplaceBack(1);
        v1.emplaceBack(2);
        za::InPlaceVector<Obj, 5> v2;
        v2.emplaceBack(1);
        v2.emplaceBack(2);
        za::InPlaceVector<Obj, 5> v3;
        v3.emplaceBack(1);
        v3.emplaceBack(3);
        za::InPlaceVector<Obj, 5> v4;
        v4.emplaceBack(1);

        CHECK((v1 == v2));
        CHECK((v2 == v1));
        CHECK((v1 != v3));
        CHECK((v3 != v1));
        CHECK((v1 != v4));
        CHECK((v4 != v1));
        CHECK((v1 == v1));

        za::InPlaceVector<Obj, 5> empty1;
        za::InPlaceVector<Obj, 5> empty2;
        CHECK((empty1 == empty2));
        CHECK((empty1 != v1));
    }

    SECTION("unsafePushBackMultiple and pushBackMultiple")
    {
        resetCounters();
        za::InPlaceVector<Obj, 10> tv;
        tv.pushBackMultiple(Obj(1), Obj(2), Obj(3));
        CHECK(tv.size() == 3);
        CHECK(intCtorCount == 3);  // For Obj(1), Obj(2), Obj(3) temporaries
        CHECK(moveCtorCount == 3); // Move from temporaries into vector
        CHECK(dtorCount == 3);     // For Obj(1), Obj(2), Obj(3) temporaries
        CHECK(tv[0].value == 1);
        CHECK(tv[1].value == 2);
        CHECK(tv[2].value == 3);

        resetCounters();
        tv.unsafePushBackMultiple(Obj(4), Obj(5)); // Assumes capacity is fine
        CHECK(tv.size() == 5);
        CHECK(intCtorCount == 2);
        CHECK(moveCtorCount == 2);
        CHECK(dtorCount == 2);
        CHECK(tv[3].value == 4);
        CHECK(tv[4].value == 5);
    }

    SECTION("emplaceRange and unsafeEmplaceBackRange")
    {
        resetCounters();
        za::InPlaceVector<Obj, 10> tv;
        Obj                        source[] = {Obj(10), Obj(20), Obj(30)};
        CHECK(intCtorCount == 3); // For source array
        resetCounters();

        tv.emplaceRange(source, 3);
        CHECK(tv.size() == 3);
        CHECK(copyCtorCount == 3); // emplaceRange uses copyRange
        CHECK(tv[0].value == 10);
        CHECK(tv[1].value == 20);
        CHECK(tv[2].value == 30);

        resetCounters();
        Obj source2[] = {Obj(40), Obj(50)};
        CHECK(intCtorCount == 2);
        resetCounters();

        tv.unsafeEmplaceBackRange(source2, 2); // Assumes capacity
        CHECK(tv.size() == 5);
        CHECK(copyCtorCount == 2);
        CHECK(tv[3].value == 40);
        CHECK(tv[4].value == 50);
    }

    SECTION("Non-movable and non-trivially-copyable")
    {
        struct NonMovable
        {
            NonMovable() = default;

            ~NonMovable() // NOLINT(modernize-use-equals-default)
            {
            }

            NonMovable(const NonMovable&) = delete;
            NonMovable(NonMovable&&)      = delete;

            NonMovable& operator=(const NonMovable&) = delete;
            NonMovable& operator=(NonMovable&&)      = delete;
        };

        za::InPlaceVector<NonMovable, 5> tv;
        tv.emplaceBack();
        CHECK(tv.size() == 1);
    }

    SECTION("emplace")
    {
        SUBCASE("Emplace into empty vector")
        {
            resetCounters();
            za::InPlaceVector<Obj, defaultCapacity> tv;

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

        SUBCASE("Emplace at the end")
        {
            resetCounters();
            za::InPlaceVector<Obj, defaultCapacity> tv;
            tv.emplaceBack(10);
            tv.emplaceBack(20);
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

        SUBCASE("Emplace in the middle")
        {
            resetCounters();
            za::InPlaceVector<Obj, defaultCapacity> tv;
            tv.emplaceBack(10);
            tv.emplaceBack(30);
            tv.emplaceBack(40);
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
            CHECK(intCtorCount == 1);
            CHECK(moveCtorCount == 2);
            CHECK(moveAssignCount == 1);
            CHECK(dtorCount == 2);
        }

        SUBCASE("Emplace at the beginning")
        {
            resetCounters();
            za::InPlaceVector<Obj, defaultCapacity> tv;
            tv.emplaceBack(20);
            tv.emplaceBack(30);
            REQUIRE(tv.size() == 2);
            resetCounters();

            Obj* itRet = tv.emplace(tv.begin(), 10);

            CHECK(tv.size() == 3);
            CHECK(tv[0].value == 10);
            CHECK(tv[1].value == 20);
            CHECK(tv[2].value == 30);
            CHECK(itRet == tv.begin());

            // Analysis: Shift 2 elements (20, 30) + temporary for new element
            CHECK(intCtorCount == 1);
            CHECK(moveCtorCount == 2);
            CHECK(moveAssignCount == 1);
            CHECK(dtorCount == 2);
        }
    }

    SECTION("Temporary assignment")
    {
        za::InPlaceVector<int, 5> v0;

        CHECK(v0.size() == 0);

        v0 = za::InPlaceVector<int, 5>{1, 2, 3};

        CHECK(v0.size() == 3);
        CHECK(v0[0] == 1);
        CHECK(v0[1] == 2);
        CHECK(v0[2] == 3);
    }

    SECTION("Init list assignment")
    {
        za::InPlaceVector<int, 5> v0;

        CHECK(v0.size() == 0);

        v0 = {1, 2, 3};

        CHECK(v0.size() == 3);
        CHECK(v0[0] == 1);
        CHECK(v0[1] == 2);
        CHECK(v0[2] == 3);
    }

    SECTION("Self-aliasing: pushBack from own element")
    {
        za::InPlaceVector<Obj, defaultCapacity> v;
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

    SECTION("Self-aliasing: insert at begin from last element")
    {
        za::InPlaceVector<Obj, defaultCapacity> v;
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
        za::InPlaceVector<Obj, defaultCapacity> v;
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
        za::InPlaceVector<Obj, defaultCapacity> v;
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

    SECTION("Self-aliasing: emplaceBack from own element")
    {
        za::InPlaceVector<Obj, defaultCapacity> v;
        v.emplaceBack(10);
        v.emplaceBack(20);
        v.emplaceBack(30);

        const Obj& ref = v[1];
        v.emplaceBack(ref);

        CHECK(v.size() == 4);
        CHECK(v[0].value == 10);
        CHECK(v[1].value == 20);
        CHECK(v[2].value == 30);
        CHECK(v[3].value == 20);
    }
}

} // namespace NonTrivialInPlaceVectorTest
} // namespace
