#include "Tst/Tst.hpp"

#include "Zancle/Container/ChunkedVector.hpp"

#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/UIntPtrT.hpp"

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
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"


namespace
{
namespace ChunkedVectorTest // for unity builds
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

    [[nodiscard]] bool operator==(const Obj& rhs) const
    {
        return value == rhs.value;
    }
};

struct Bytes256
{
    char data[256];
};

struct Bytes70000
{
    char data[70'000];
};

struct alignas(64) OverAligned
{
    int value = 0;
};

struct IsTwo
{
    [[nodiscard]] bool operator()(const int x) const
    {
        return x == 2;
    }
};

// `findIf` returns a pointer into the container: calling it on an rvalue must be ill-formed
template <typename V>
concept CanFindIf = requires(V&& v) { static_cast<V&&>(v).findIf(IsTwo{}); };

static_assert(CanFindIf<za::ChunkedVector<int, 2>&>);
static_assert(CanFindIf<const za::ChunkedVector<int, 2>&>);
static_assert(!CanFindIf<za::ChunkedVector<int, 2>>);
static_assert(!CanFindIf<const za::ChunkedVector<int, 2>>);

// Default `BlockShift` targets 64 KiB blocks
static_assert(za::ChunkedVector<char>::blockShift == 16u);
static_assert(za::ChunkedVector<int>::blockShift == 14u);
static_assert(za::ChunkedVector<Bytes256>::blockShift == 8u);
static_assert(za::ChunkedVector<Bytes70000>::blockShift == 0u);

// An explicit `BlockShift` allows an incomplete `TItem`
struct SelfReferential
{
    za::ChunkedVector<SelfReferential, 2> children;
};

#ifdef __cpp_exceptions
////////////////////////////////////////////////////////////
// Constructors that throw on demand, tracking the number of live objects
struct Throwing
{
    static inline int liveCount        = 0;
    static inline int copiesUntilThrow = -1; // Negative: never throw

    int value;

    explicit Throwing(const int x) : value(x)
    {
        if (x < 0)
            throw 42;

        ++liveCount;
    }

    Throwing(const Throwing& rhs) : value(rhs.value)
    {
        if (copiesUntilThrow == 0)
            throw 42;

        if (copiesUntilThrow > 0)
            --copiesUntilThrow;

        ++liveCount;
    }

    Throwing& operator=(const Throwing&) = delete;

    ~Throwing()
    {
        --liveCount;
    }
};
#endif

TEST_CASE("[Base] Base/ChunkedVector.hpp")
{
    const auto asConst = [](auto& x) -> const auto& { return x; };

    SECTION("Type traits")
    {
        using T = za::ChunkedVector<int, 2>;

        STATIC_CHECK(!ZA_IS_TRIVIAL(T));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPYABLE(T));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(T));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(T));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(T));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(T));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(T));

        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(T));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(T));
        STATIC_CHECK(ZA_IS_MOVE_CONSTRUCTIBLE(T));
        STATIC_CHECK(ZA_IS_MOVE_ASSIGNABLE(T));
        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(T));
    }

    SECTION("Empty")
    {
#define DO_EMPTY_CHECKS_CV(tv)                                   \
    CHECK_UNARY(((tv).begin() == (tv).end()));                   \
    CHECK_UNARY((asConst((tv)).begin() == asConst((tv)).end())); \
    CHECK_UNARY(((tv).cbegin() == (tv).cend()));                 \
    CHECK((tv).size() == 0u);                                    \
    CHECK((tv).capacity() == 0u);                                \
    CHECK((tv).empty());

        za::ChunkedVector<int, 2> tv;
        DO_EMPTY_CHECKS_CV(tv);

        tv.clear();
        DO_EMPTY_CHECKS_CV(tv);

        za::ChunkedVector<int, 2> tv2 = tv;
        DO_EMPTY_CHECKS_CV(tv2);

        za::ChunkedVector<int, 2> tv3 = ZA_MOVE(tv);
        DO_EMPTY_CHECKS_CV(tv3);

        za::ChunkedVector<int, 2> tv4;
        tv4 = tv2;
        DO_EMPTY_CHECKS_CV(tv4);

        za::ChunkedVector<int, 2> tv5;
        tv5 = ZA_MOVE(tv4);
        DO_EMPTY_CHECKS_CV(tv5);
    }

    SECTION("Constructors and equality")
    {
        const int src[] = {1, 2, 3, 4, 5};

        const za::ChunkedVector<int, 2> fromRange(src, src + 5);
        const za::ChunkedVector<int, 2> fromList{1, 2, 3, 4, 5};
        const za::ChunkedVector<int, 2> filled(5, 7);

        CHECK(fromRange.size() == 5u);
        CHECK(fromRange == fromList);
        CHECK(fromRange != filled);
        CHECK(filled.size() == 5u);

        for (za::SizeT i = 0u; i < 5u; ++i)
            CHECK(filled[i] == 7);
    }

    SECTION("Push back, reserve, pointer stability, and shrinkToFit")
    {
        za::ChunkedVector<int, 2> vec;

        for (int i = 0; i < 10; ++i)
            vec.pushBack(i);

        CHECK(vec.size() == 10u);
        CHECK(vec.capacity() == 12u);
        CHECK(vec.front() == 0);
        CHECK(vec.back() == 9);

        for (za::SizeT i = 0u; i < vec.size(); ++i)
            CHECK(vec[i] == static_cast<int>(i));

        int* const stablePtr = &vec[3];
        vec.reserve(19u);

        CHECK(vec.capacity() == 20u);
        CHECK(stablePtr == &vec[3]);
        CHECK(*stablePtr == 3);

        vec.resize(5u);
        CHECK(vec.size() == 5u);
        CHECK(vec.capacity() == 20u);
        CHECK(vec.back() == 4);

        vec.shrinkToFit();
        CHECK(vec.capacity() == 8u);
        CHECK(stablePtr == &vec[3]); // element storage remains stable even when directory shrinks

        vec.clear();
        CHECK(vec.empty());
        CHECK(vec.capacity() == 8u);

        vec.shrinkToFit();
        CHECK(vec.capacity() == 0u);
    }

    SECTION("Resize, unsafe append helpers, and popBack")
    {
        za::ChunkedVector<int, 2> vec;
        vec.resize(6u);

        CHECK(vec.size() == 6u);
        CHECK(vec.capacity() == 8u);

        for (za::SizeT i = 0u; i < vec.size(); ++i)
            CHECK(vec[i] == 0);

        const int src[] = {10, 20, 30};

        vec.reserve(12u);
        vec.clear();
        vec.unsafeEmplaceBackRange(src, 3u);
        vec.unsafePushBackMultiple(40, 50);

        CHECK(vec == za::ChunkedVector<int, 2>{10, 20, 30, 40, 50});

        vec.popBack();
        CHECK(vec == za::ChunkedVector<int, 2>{10, 20, 30, 40});
    }

    SECTION("Callback iteration helpers")
    {
        za::ChunkedVector<int, 2> vec;

        for (int i = 0; i < 10; ++i)
            vec.pushBack(i);

        int mutatedSum = 0;
        vec.forEach([&](int& x)
        {
            mutatedSum += x;
            x *= 2;
        });

        CHECK(mutatedSum == 45);

        za::SizeT indexedCount = 0u;
        vec.forEachIndexed([&](const za::SizeT i, int& x)
        {
            CHECK(x == static_cast<int>(i * 2u));
            ++indexedCount;
        });

        CHECK(indexedCount == vec.size());

        za::SizeT blockCount   = 0u;
        za::SizeT elementCount = 0u;
        int       blockSum     = 0;

        asConst(vec).forEachBlock([&](const int* begin, const int* end)
        {
            ++blockCount;
            elementCount += static_cast<za::SizeT>(end - begin);

            for (const int* p = begin; p != end; ++p)
                blockSum += *p;
        });

        CHECK(blockCount == 3u);
        CHECK(elementCount == 10u);
        CHECK(blockSum == 90);

        CHECK(vec.findIf([](const int x) { return x == 12; }) == &vec[6]);
        CHECK(asConst(vec).findIf([](const int x) { return x == 18; }) == &asConst(vec)[9]);
        CHECK(vec.findIf([](const int x) { return x == 999; }) == nullptr);

        const int reduced = asConst(vec).reduce(0, [](int acc, const int x) { return acc + x; });
        CHECK(reduced == 90);
    }

    SECTION("Iterator compatibility")
    {
        za::ChunkedVector<int, 2> vec{0, 1, 2, 3, 4, 5, 6};

        int rangeForSum = 0;
        for (const int x : vec)
            rangeForSum += x;

        CHECK(rangeForSum == 21);

        auto it = vec.begin();
        it += 5;

        CHECK(*it == 5);
        CHECK(it[-2] == 3);
        CHECK(it - vec.begin() == 5);
        CHECK(*(it - 1) == 4);
        CHECK(vec.end() - vec.begin() == 7);

        const auto cit = asConst(vec).begin() + 6;
        CHECK(*cit == 6);
    }

    SECTION("Non-trivial growth does not relocate existing elements")
    {
        resetCounters();

        {
            za::ChunkedVector<Obj, 2> vec;
            vec.emplaceBack(1);
            vec.emplaceBack(2);
            vec.emplaceBack(3);

            Obj* const stablePtr = &vec[1];

            CHECK(intCtorCount == 3);
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 0);

            vec.reserve(12u);

            CHECK(stablePtr == &vec[1]);
            CHECK(stablePtr->value == 2);
            CHECK(copyCtorCount == 0);
            CHECK(moveCtorCount == 0);
        }

        CHECK(dtorCount == 3);
    }

    SECTION("Non-trivial copy and move semantics")
    {
        resetCounters();

        za::ChunkedVector<Obj, 2> source;
        source.emplaceBack(1);
        source.emplaceBack(2);
        source.emplaceBack(3);

        Obj* const stablePtr = &source[1];

        resetCounters();

        za::ChunkedVector<Obj, 2> copy(source);
        za::ChunkedVector<Obj, 2> assigned;
        assigned = source;

        CHECK(copyCtorCount == 6);
        CHECK(copyAssignCount == 0);
        CHECK(moveCtorCount == 0);
        CHECK(moveAssignCount == 0);
        CHECK_UNARY(copy == source);
        CHECK_UNARY(assigned == source);

        za::ChunkedVector<Obj, 2> moved(ZA_MOVE(source));
        CHECK(source.empty());
        CHECK(stablePtr == &moved[1]);

        za::ChunkedVector<Obj, 2> moveAssigned;
        moveAssigned = ZA_MOVE(copy);

        CHECK(copy.empty());
        CHECK(moveAssigned.size() == 3u);
        CHECK(moveAssigned[0].value == 1);
        CHECK(moveAssigned[1].value == 2);
        CHECK(moveAssigned[2].value == 3);
    }

    SECTION("Non-trivial resize, popBack, and clear destroy the right objects")
    {
        resetCounters();

        za::ChunkedVector<Obj, 2> vec;
        vec.emplaceBack(1);
        vec.emplaceBack(2);
        vec.emplaceBack(3);
        vec.resize(5u);

        CHECK(intCtorCount == 3);
        CHECK(defaultCtorCount == 2);
        CHECK(dtorCount == 0);

        vec.resize(2u);
        CHECK(dtorCount == 3);

        vec.popBack();
        CHECK(dtorCount == 4);

        vec.clear();
        CHECK(dtorCount == 5);
        CHECK(vec.empty());
    }

    SECTION("Self-assignment")
    {
        za::ChunkedVector<int, 2> vec{1, 2, 3, 4, 5};

        const auto* const stablePtr = &vec[2];

        auto& ref = vec;
        vec       = ref;
        CHECK(vec.size() == 5u);
        CHECK(vec[0] == 1);
        CHECK(vec[2] == 3);
        CHECK(vec[4] == 5);
        CHECK(stablePtr == &vec[2]);

        auto& moveRef = vec;
        vec           = ZA_MOVE(moveRef);
        CHECK(vec.size() == 5u);
        CHECK(vec[0] == 1);
        CHECK(vec[2] == 3);
        CHECK(vec[4] == 5);
    }

    SECTION("Swap")
    {
        za::ChunkedVector<int, 2> a{1, 2, 3};
        za::ChunkedVector<int, 2> b{10, 20};

        a.swap(b);
        CHECK(a.size() == 2u);
        CHECK(a[0] == 10);
        CHECK(a[1] == 20);
        CHECK(b.size() == 3u);
        CHECK(b[0] == 1);
        CHECK(b[1] == 2);
        CHECK(b[2] == 3);

        swap(a, b);
        CHECK(a == za::ChunkedVector<int, 2>{1, 2, 3});
        CHECK(b == za::ChunkedVector<int, 2>{10, 20});

        // Self-swap
        a.swap(a);
        CHECK(a == za::ChunkedVector<int, 2>{1, 2, 3});
    }

    SECTION("Reserve smaller than current capacity is a no-op")
    {
        za::ChunkedVector<int, 2> vec{1, 2, 3, 4, 5, 6};

        const auto        capBefore = vec.capacity();
        const auto* const stablePtr = &vec[3];

        vec.reserve(2u);
        CHECK(vec.capacity() == capBefore);
        CHECK(vec.size() == 6u);
        CHECK(stablePtr == &vec[3]);
    }

    SECTION("ShrinkToFit when already tight")
    {
        za::ChunkedVector<int, 2> vec;

        // Push exactly one block worth of elements (blockSize == 4 for BlockShift=2)
        vec.pushBack(1);
        vec.pushBack(2);
        vec.pushBack(3);
        vec.pushBack(4);

        const auto capBefore = vec.capacity();
        CHECK(capBefore == 4u);

        vec.shrinkToFit();
        CHECK(vec.capacity() == capBefore);
        CHECK(vec.size() == 4u);

        for (za::SizeT i = 0u; i < 4u; ++i)
            CHECK(vec[i] == static_cast<int>(i + 1));
    }

    SECTION("Block boundary crossing")
    {
        za::ChunkedVector<int, 2> vec;

        // Push exactly blockSize elements (fills one block exactly)
        for (za::SizeT i = 0u; i < 4u; ++i)
            vec.pushBack(static_cast<int>(i * 10u));

        CHECK(vec.size() == 4u);
        CHECK(vec.capacity() == 4u);

        // Push one more -- crosses into a second block
        vec.pushBack(40);
        CHECK(vec.size() == 5u);
        CHECK(vec.capacity() == 8u);

        for (za::SizeT i = 0u; i < 5u; ++i)
            CHECK(vec[i] == static_cast<int>(i * 10u));

        // Verify pointer stability across the boundary
        const int* const ptrInBlock0 = &vec[3];
        const int* const ptrInBlock1 = &vec[4];

        vec.pushBack(50);
        vec.pushBack(60);
        vec.pushBack(70);

        CHECK(ptrInBlock0 == &vec[3]);
        CHECK(ptrInBlock1 == &vec[4]);
        CHECK(*ptrInBlock0 == 30);
        CHECK(*ptrInBlock1 == 40);
    }

    SECTION("Reduce on empty vector")
    {
        const za::ChunkedVector<int, 2> vec;
        CHECK(vec.reduce(42, [](int acc, int x) { return acc + x; }) == 42);
    }

    SECTION("FindIf on empty vector")
    {
        za::ChunkedVector<int, 2> vec;
        CHECK(vec.findIf([](int) { return true; }) == nullptr);
        CHECK(asConst(vec).findIf([](int) { return true; }) == nullptr);
    }

    SECTION("Default block shift")
    {
        za::ChunkedVector<int> vec{1, 2, 3};

        CHECK(vec.size() == 3u);
        CHECK(vec.capacity() == za::SizeT{16'384u});
        CHECK(vec == za::ChunkedVector<int>{1, 2, 3});

        SelfReferential root;
        root.children.emplaceBack();
        root.children[0].children.emplaceBack();
        CHECK(root.children.size() == 1u);
        CHECK(root.children[0].children.size() == 1u);
    }

    SECTION("Small block shifts")
    {
        const auto test = [&]<za::SizeT Shift>()
        {
            za::ChunkedVector<int, Shift> vec;

            for (int i = 0; i < 10; ++i)
                vec.pushBack(i);

            CHECK(vec.size() == 10u);
            CHECK(vec.capacity() == ((10u + (1u << Shift) - 1u) >> Shift) << Shift);

            for (za::SizeT i = 0u; i < vec.size(); ++i)
                CHECK(vec[i] == static_cast<int>(i));

            za::SizeT blockCount = 0u;
            int       blockSum   = 0;

            vec.forEachBlock([&](const int* begin, const int* end)
            {
                ++blockCount;
                CHECK(static_cast<za::SizeT>(end - begin) <= (za::SizeT{1u} << Shift));

                for (const int* p = begin; p != end; ++p)
                    blockSum += *p;
            });

            CHECK(blockCount == (10u + (1u << Shift) - 1u) >> Shift);
            CHECK(blockSum == 45);

            int expected = 0;
            for (const int x : vec)
                CHECK(x == expected++);

            CHECK(expected == 10);

            auto it = vec.end();
            for (int i = 9; i >= 0; --i)
                CHECK(*--it == i);

            CHECK(it == vec.begin());
            CHECK(*(vec.begin() + 7) == 7);
            CHECK(vec.findIf([](const int x) { return x == 5; }) == &vec[5]);

            const za::ChunkedVector<int, Shift> copy = vec;
            CHECK(copy == vec);

            vec.popBack();
            vec.shrinkToFit();
            CHECK(vec.size() == 9u);
            CHECK(vec.back() == 8);
            CHECK(copy != vec);
        };

        test.template operator()<0u>();
        test.template operator()<1u>();
    }

    SECTION("Iterator operations")
    {
        za::ChunkedVector<int, 2> vec{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

        // Plain assignment (regression: used to warn with `-Wunused-result`)
        auto it = vec.begin();
        it      = vec.begin() + 1;
        CHECK(*it == 1);

        // Pre/post increment and decrement across block boundaries
        it = vec.begin() + 3;
        CHECK(*it++ == 3);
        CHECK(*it == 4);
        CHECK(*it-- == 4);
        CHECK(*it == 3);
        CHECK(*++it == 4);
        CHECK(*--it == 3);

        // `n + it`
        CHECK(*(2 + vec.begin()) == 2);
        CHECK(*(5 + asConst(vec).begin()) == 5);
        CHECK(*(vec.end() - 1) == 9);
        CHECK(vec.begin()[9] == 9);

        it += 6;
        CHECK(*it == 9);
        it -= 9;
        CHECK(*it == 0);
        CHECK(&*it == &vec[0]);

        // Relational operators
        const auto a = vec.begin() + 2;
        const auto b = vec.begin() + 6;

        CHECK(a < b);
        CHECK(a <= b);
        CHECK(b > a);
        CHECK(b >= a);
        CHECK(a <= a);
        CHECK(a >= a);
        CHECK_FALSE(a < a);
        CHECK_FALSE(b < a);
        CHECK(a != b);
        CHECK(b - a == 4);
        CHECK(a - b == -4);

        // Mixed mutable/const operations, in both directions
        const za::ChunkedVector<int, 2>::ConstIterator ca = asConst(vec).begin() + 2;
        const za::ChunkedVector<int, 2>::ConstIterator cb = asConst(vec).begin() + 6;

        CHECK(a == ca);
        CHECK(ca == a);
        CHECK(a != cb);
        CHECK(cb != a);
        CHECK(a < cb);
        CHECK(ca < b);
        CHECK(b > ca);
        CHECK(cb > a);
        CHECK(a <= ca);
        CHECK(ca >= a);
        CHECK(b - ca == 4);
        CHECK(cb - a == 4);
        CHECK(a - cb == -4);
        CHECK(vec.end() - asConst(vec).begin() == 10);
        CHECK(asConst(vec).end() - vec.begin() == 10);

        za::ChunkedVector<int, 2>::ConstIterator converted = a;
        converted                                          = b;
        CHECK(*converted == 6);
    }

    SECTION("Iterator arrow operator")
    {
        za::ChunkedVector<Obj, 2> vec;

        for (int i = 0; i < 6; ++i)
            vec.emplaceBack(i);

        auto it = vec.begin() + 4;
        CHECK(it->value == 4);

        it->value = 40;
        CHECK(vec[4].value == 40);
        CHECK((asConst(vec).begin() + 4)->value == 40);
    }

    SECTION("Decrementing from end when size is a multiple of the block size")
    {
        za::ChunkedVector<int, 2> vec{0, 1, 2, 3, 4, 5, 6, 7};
        CHECK(vec.size() == vec.capacity());

        auto it = vec.end();
        CHECK(*--it == 7);
        CHECK(*(vec.end() - 5) == 3);
        CHECK(vec.end() - vec.begin() == 8);
        CHECK(vec.begin() + 8 == vec.end());

        za::ChunkedVector<int, 2>::Iterator fromBegin = vec.begin();
        for (int i = 0; i < 8; ++i)
            ++fromBegin;

        CHECK(fromBegin == vec.end());
    }

    SECTION("Iterators remain valid across pushBack")
    {
        za::ChunkedVector<int, 2> vec{0, 1, 2, 3};
        CHECK(vec.size() == vec.capacity());

        auto       it   = vec.begin() + 1;
        auto       last = vec.begin() + 3;
        const auto cit  = asConst(vec).begin() + 2;

        // Grow well past the initial directory capacity (forces directory reallocation)
        for (int i = 4; i < 100; ++i)
            vec.pushBack(i);

        CHECK(*it == 1);
        CHECK(*cit == 2);
        CHECK(*last == 3);

        ++last; // Crosses into a block allocated after the iterator was created
        CHECK(*last == 4);

        it += 50;
        CHECK(*it == 51);

        int expected = 1;
        for (auto i = vec.begin() + 1; i != vec.end(); ++i)
            CHECK(*i == expected++);

        CHECK(expected == 100);
    }

    SECTION("Sizes that are exact multiples of the block size")
    {
        za::ChunkedVector<int, 2> vec{0, 1, 2, 3, 4, 5, 6, 7};

        za::SizeT blockCount   = 0u;
        za::SizeT elementCount = 0u;

        vec.forEachBlock([&](const int* begin, const int* end)
        {
            ++blockCount;
            CHECK(end - begin == 4);
            elementCount += static_cast<za::SizeT>(end - begin);
        });

        CHECK(blockCount == 2u);
        CHECK(elementCount == 8u);

        const za::ChunkedVector<int, 2> copy = vec;
        CHECK(copy.size() == 8u);
        CHECK(copy.capacity() == 8u);
        CHECK(copy == vec);

        za::ChunkedVector<int, 2> other{0, 1, 2, 3, 4, 5, 6, 8};
        CHECK(other != vec);

        za::SizeT indexedCount = 0u;
        vec.forEachIndexed([&](const za::SizeT i, const int x)
        {
            CHECK(x == static_cast<int>(i));
            ++indexedCount;
        });

        CHECK(indexedCount == 8u);
        CHECK(vec.findIf([](const int x) { return x == 7; }) == &vec[7]);
    }

    SECTION("Directory regrowth after shrinkToFit")
    {
        za::ChunkedVector<int, 2> vec;

        for (int i = 0; i < 5; ++i)
            vec.pushBack(i);

        vec.shrinkToFit();
        CHECK(vec.capacity() == 8u);

        for (int i = 5; i < 40; ++i)
            vec.pushBack(i);

        CHECK(vec.size() == 40u);

        for (za::SizeT i = 0u; i < vec.size(); ++i)
            CHECK(vec[i] == static_cast<int>(i));

        vec.clear();
        vec.shrinkToFit();
        CHECK(vec.capacity() == 0u);

        for (int i = 0; i < 30; ++i)
            vec.pushBack(i);

        CHECK(vec.size() == 30u);
        CHECK(vec.back() == 29);
    }

    SECTION("Copy-assign into destination with more blocks")
    {
        za::ChunkedVector<int, 2> dst;

        for (int i = 0; i < 20; ++i)
            dst.pushBack(i);

        const za::ChunkedVector<int, 2> src{7, 8, 9};

        dst = src;
        CHECK(dst.size() == 3u);
        CHECK(dst.capacity() == 20u); // Existing blocks are reused
        CHECK(dst == src);

        dst.pushBack(10);
        CHECK(dst == za::ChunkedVector<int, 2>{7, 8, 9, 10});
    }

    SECTION("Move-assign into non-empty non-trivial vector")
    {
        za::ChunkedVector<Obj, 2> dst;
        za::ChunkedVector<Obj, 2> src;

        for (int i = 0; i < 5; ++i)
            dst.emplaceBack(i);

        for (int i = 10; i < 13; ++i)
            src.emplaceBack(i);

        resetCounters();

        dst = ZA_MOVE(src);

        CHECK(dtorCount == 5); // The old elements of `dst`
        CHECK(copyCtorCount == 0);
        CHECK(moveCtorCount == 0);
        CHECK(src.empty());
        CHECK(src.capacity() == 0u);
        CHECK(dst.size() == 3u);
        CHECK(dst[0].value == 10);
        CHECK(dst[2].value == 12);
    }

    SECTION("Over-aligned items")
    {
        za::ChunkedVector<OverAligned, 2> vec;

        for (int i = 0; i < 10; ++i)
            vec.pushBack(OverAligned{i});

        for (za::SizeT i = 0u; i < vec.size(); ++i)
        {
            CHECK(reinterpret_cast<za::UIntPtrT>(&vec[i]) % 64u == 0u);
            CHECK(vec[i].value == static_cast<int>(i));
        }
    }

    SECTION("Fill constructor and resize with value")
    {
        resetCounters();

        {
            za::ChunkedVector<Obj, 2> vec(5, Obj{7});

            CHECK(intCtorCount == 1);
            CHECK(copyCtorCount == 5);
            CHECK(dtorCount == 1); // The temporary
            CHECK(vec.size() == 5u);

            for (za::SizeT i = 0u; i < vec.size(); ++i)
                CHECK(vec[i].value == 7);

            resetCounters();

            vec[0].value = 3;
            vec.resize(11u, vec[0]); // Aliasing an existing element is supported

            CHECK(copyCtorCount == 6);
            CHECK(defaultCtorCount == 0);
            CHECK(vec.size() == 11u);

            for (za::SizeT i = 5u; i < vec.size(); ++i)
                CHECK(vec[i].value == 3);

            vec.resize(2u, Obj{99});
            CHECK(vec.size() == 2u);
            CHECK(copyCtorCount == 6);
            CHECK(dtorCount == 9 + 1); // 9 removed elements + the temporary

            resetCounters();
        }

        CHECK(dtorCount == 2);
    }

    SECTION("FindIf at block boundaries and in the tail")
    {
        za::ChunkedVector<int, 2> vec{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

        CHECK(vec.findIf([](const int x) { return x == 0; }) == &vec[0]);
        CHECK(vec.findIf([](const int x) { return x == 3; }) == &vec[3]);
        CHECK(vec.findIf([](const int x) { return x == 4; }) == &vec[4]);
        CHECK(vec.findIf([](const int x) { return x == 8; }) == &vec[8]);
        CHECK(vec.findIf([](const int x) { return x == 9; }) == &vec[9]);
        CHECK(vec.findIf([](const int x) { return x > 5; }) == &vec[6]); // First match
        CHECK(vec.findIf([](const int x) { return x == 10; }) == nullptr);

        za::SizeT calls = 0u;
        CHECK(vec.findIf([&](const int x)
        {
            ++calls;
            return x == 5;
        }) == &vec[5]);

        CHECK(calls == 6u); // Stops at the first match
    }

    SECTION("Equality differing only in the tail or in the first block")
    {
        const za::ChunkedVector<int, 2> a{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        const za::ChunkedVector<int, 2> b{0, 1, 2, 3, 4, 5, 6, 7, 8, 10};
        const za::ChunkedVector<int, 2> c{-1, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        const za::ChunkedVector<int, 2> d{0, 1, 2, 3, 4, 5, 6, 7, 8};

        CHECK(a != b);
        CHECK(a != c);
        CHECK(a != d);
        CHECK(a == a);
        CHECK(a == za::ChunkedVector<int, 2>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    }

    SECTION("unsafeEmplaceBackRange with null and zero count")
    {
        za::ChunkedVector<int, 2> vec;
        vec.unsafeEmplaceBackRange(nullptr, 0u);
        CHECK(vec.empty());

        vec.reserve(4u);
        vec.unsafeEmplaceBackRange(nullptr, 0u);
        CHECK(vec.empty());

        const za::ChunkedVector<int, 2> fromEmptyRange(static_cast<const int*>(nullptr), static_cast<const int*>(nullptr));
        CHECK(fromEmptyRange.empty());
    }

    SECTION("Reduce moves the accumulator")
    {
        const za::ChunkedVector<int, 2> vec{1, 2, 3, 4, 5, 6};

        resetCounters();

        const Obj result = vec.reduce(Obj{0},
                                      [](Obj&& acc, const int x)
        {
            acc.value += x;
            return static_cast<Obj&&>(acc);
        });

        CHECK(result.value == 21);
        CHECK(copyCtorCount == 0);
        CHECK(copyAssignCount == 0);
    }

#ifdef __cpp_exceptions
    SECTION("Throwing emplaceBack leaves the vector unchanged")
    {
        Throwing::liveCount = 0;

        {
            za::ChunkedVector<Throwing, 2> vec;

            for (int i = 0; i < 4; ++i)
                vec.emplaceBack(i);

            bool threw = false;

            try
            {
                vec.emplaceBack(-1); // Throws right after allocating a new block
            } catch (int)
            {
                threw = true;
            }

            CHECK(threw);
            CHECK(vec.size() == 4u);
            CHECK(Throwing::liveCount == 4);

            vec.emplaceBack(4);

            threw = false;

            try
            {
                vec.emplaceBack(-1); // Throws within an existing block
            } catch (int)
            {
                threw = true;
            }

            CHECK(threw);
            CHECK(vec.size() == 5u);
            CHECK(Throwing::liveCount == 5);

            vec.emplaceBack(5);
            CHECK(vec.size() == 6u);
            CHECK(vec.back().value == 5);
            CHECK(vec[4].value == 4);
        }

        // Only constructed elements were destroyed
        CHECK(Throwing::liveCount == 0);
    }

    SECTION("Throwing copy in copy constructor runs the destructor")
    {
        Throwing::liveCount = 0;

        {
            za::ChunkedVector<Throwing, 2> src;

            for (int i = 0; i < 10; ++i)
                src.emplaceBack(i);

            CHECK(Throwing::liveCount == 10);

            Throwing::copiesUntilThrow = 5; // The 6th copy (second element of the second block) throws

            bool threw = false;

            try
            {
                const za::ChunkedVector<Throwing, 2> copy(src);
            } catch (int)
            {
                threw = true;
            }

            Throwing::copiesUntilThrow = -1;

            CHECK(threw);

            // The first block (4 copies) was committed to the size, and destroyed by the destructor, which also
            // freed the blocks. The single copy made into the second block before the throw is leaked (by design,
            // to avoid per-element bookkeeping).
            CHECK(Throwing::liveCount == 10 + 1);

            Throwing::copiesUntilThrow = 3; // The 4th copy throws, within the first block

            threw = false;

            try
            {
                const za::ChunkedVector<Throwing, 2> filled(6, src[0]);
            } catch (int)
            {
                threw = true;
            }

            Throwing::copiesUntilThrow = -1;

            CHECK(threw);
            CHECK(Throwing::liveCount == 10 + 1 + 3); // Nothing destroyed twice, nothing unconstructed destroyed
        }

        CHECK(Throwing::liveCount == 1 + 3);
    }
#endif
}

} // namespace ChunkedVectorTest
} // namespace
