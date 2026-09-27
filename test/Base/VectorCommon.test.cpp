#include "Tst/Tst.hpp"

#include "Zancle/Container/InPlaceVector.hpp"
#include "Zancle/Container/SmallVector.hpp"
#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/UIntPtrT.hpp"

#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"


namespace
{
namespace VectorCommonTest // for unity builds
{
////////////////////////////////////////////////////////////
// Counts live objects; not trivially relocatable, so it exercises the element-wise paths
struct Counted
{
    static inline int live = 0;

    int value;

    Counted(const int v) : value{v}
    {
        ++live;
    }

    Counted(const Counted& rhs) : value{rhs.value}
    {
        ++live;
    }

    Counted(Counted&& rhs) noexcept : value{rhs.value}
    {
        ++live;
    }

    Counted& operator=(const Counted&) = default;
    Counted& operator=(Counted&&)      = default;

    ~Counted()
    {
        --live;
    }

    [[nodiscard]] bool operator==(const Counted&) const = default;
};


////////////////////////////////////////////////////////////
// Throws when constructed from `throwValue` (constructed objects are counted)
struct Thrower
{
    static inline int live = 0;

    static constexpr int throwValue = -1;

    int value;

    explicit Thrower(const int v) : value{v}
    {
#ifdef __cpp_exceptions
        if (v == throwValue)
            throw 0;
#endif

        ++live;
    }

    Thrower(const Thrower& rhs) : value{rhs.value}
    {
        ++live;
    }

    Thrower(Thrower&& rhs) noexcept : value{rhs.value}
    {
        ++live;
    }

    Thrower& operator=(const Thrower&) = default;

    ~Thrower()
    {
        --live;
    }
};


////////////////////////////////////////////////////////////
struct alignas(64) OverAligned
{
    int value;
};


////////////////////////////////////////////////////////////
struct Node
{
    int              value;
    za::Vector<Node> children;
};


////////////////////////////////////////////////////////////
template <typename T>
using SmallVec4 = za::SmallVector<T, 4>;

template <typename T>
using InPlaceVec16 = za::InPlaceVector<T, 16>;

using IntVector        = za::Vector<int>;
using IntSmallVector   = SmallVec4<int>;
using IntInPlaceVector = InPlaceVec16<int>;

using CountedVector        = za::Vector<Counted>;
using CountedSmallVector   = SmallVec4<Counted>;
using CountedInPlaceVector = InPlaceVec16<Counted>;

using ThrowerVector        = za::Vector<Thrower>;
using ThrowerSmallVector   = SmallVec4<Thrower>;
using ThrowerInPlaceVector = InPlaceVec16<Thrower>;


////////////////////////////////////////////////////////////
template <typename TVector>
void fillWithValues(TVector& v, const int count)
{
    for (int i = 0; i < count; ++i)
        v.emplaceBack(i);
}


////////////////////////////////////////////////////////////
template <typename TVector>
[[nodiscard]] bool valuesAre(const TVector& v, const za::InitializerList<int> expected)
{
    if (v.size() != expected.size())
        return false;

    za::SizeT i = 0u;
    for (const int e : expected)
        if (v[i++] != e)
            return false;

    return true;
}

} // namespace VectorCommonTest
} // namespace


ZA_TST_DECLARE_TYPE_NAME(VectorCommonTest::IntVector, "Vector<int>")
ZA_TST_DECLARE_TYPE_NAME(VectorCommonTest::IntSmallVector, "SmallVector<int, 4>")
ZA_TST_DECLARE_TYPE_NAME(VectorCommonTest::IntInPlaceVector, "InPlaceVector<int, 16>")
ZA_TST_DECLARE_TYPE_NAME(VectorCommonTest::CountedVector, "Vector<Counted>")
ZA_TST_DECLARE_TYPE_NAME(VectorCommonTest::CountedSmallVector, "SmallVector<Counted, 4>")
ZA_TST_DECLARE_TYPE_NAME(VectorCommonTest::CountedInPlaceVector, "InPlaceVector<Counted, 16>")
ZA_TST_DECLARE_TYPE_NAME(VectorCommonTest::ThrowerVector, "Vector<Thrower>")
ZA_TST_DECLARE_TYPE_NAME(VectorCommonTest::ThrowerSmallVector, "SmallVector<Thrower, 4>")
ZA_TST_DECLARE_TYPE_NAME(VectorCommonTest::ThrowerInPlaceVector, "InPlaceVector<Thrower, 16>")


////////////////////////////////////////////////////////////
TEMPLATE_TEST_CASE("[Base] Vector family: aliasing arguments",
                   "",
                   VectorCommonTest::IntVector,
                   VectorCommonTest::IntSmallVector,
                   VectorCommonTest::IntInPlaceVector,
                   VectorCommonTest::CountedVector,
                   VectorCommonTest::CountedSmallVector,
                   VectorCommonTest::CountedInPlaceVector)
{
    using namespace VectorCommonTest;

    Counted::live = 0;

    SECTION("pushBackMultiple with elements of the vector itself (growing when possible)")
    {
        TestType v;
        fillWithValues(v, 4);
        v.shrinkToFit(); // full: the next append grows (except for `InPlaceVector`)

        v.pushBackMultiple(v[0], v[3], v[1]);
        CHECK(valuesAre(v, {0, 1, 2, 3, 0, 3, 1}));
    }

    SECTION("emplaceBackRange from the vector itself (growing when possible)")
    {
        TestType v;
        fillWithValues(v, 3);
        v.shrinkToFit();

        v.emplaceBackRange(v.data(), v.size());
        CHECK(valuesAre(v, {0, 1, 2, 0, 1, 2}));

        v.emplaceBackRange(v.data() + 1, 2);
        CHECK(valuesAre(v, {0, 1, 2, 0, 1, 2, 1, 2}));
    }

    SECTION("assignRange from a subrange of the vector itself")
    {
        TestType v;
        fillWithValues(v, 6);

        v.assignRange(v.data() + 1, v.data() + 4);
        CHECK(valuesAre(v, {1, 2, 3}));

        v.assignRange(v.data(), v.data() + v.size()); // whole range: no-op
        CHECK(valuesAre(v, {1, 2, 3}));

        v.assignRange(v.data() + 2, v.data() + 3); // suffix
        CHECK(valuesAre(v, {3}));

        v.assignRange(v.data(), v.data()); // empty subrange
        CHECK(v.empty());
    }

    SECTION("insert an element of the vector itself (growing when possible)")
    {
        TestType v;
        fillWithValues(v, 4);
        v.shrinkToFit();

        v.insert(v.cbegin(), v[2]);
        CHECK(valuesAre(v, {2, 0, 1, 2, 3}));

        v.insert(v.cbegin() + 2, v[4]);
        CHECK(valuesAre(v, {2, 0, 3, 1, 2, 3}));
    }

    SECTION("pushBack and emplaceBack of an element of the vector itself")
    {
        TestType v;
        fillWithValues(v, 4);
        v.shrinkToFit();

        v.pushBack(v[1]);
        v.emplaceBack(v[0]);
        CHECK(valuesAre(v, {0, 1, 2, 3, 1, 0}));
    }

    if constexpr (ZA_IS_SAME(typename TestType::value_type, Counted))
        CHECK(Counted::live == 0);
}


////////////////////////////////////////////////////////////
TEMPLATE_TEST_CASE("[Base] Vector family: const positions and misc",
                   "",
                   VectorCommonTest::IntVector,
                   VectorCommonTest::IntSmallVector,
                   VectorCommonTest::IntInPlaceVector)
{
    using namespace VectorCommonTest;

    SECTION("erase, insert, and emplace accept const positions")
    {
        TestType v;
        fillWithValues(v, 5);

        const TestType& cv = v;

        int* const erased = v.erase(cv.begin() + 1);
        CHECK(erased == v.data() + 1);
        CHECK(valuesAre(v, {0, 2, 3, 4}));

        v.erase(v.cbegin() + 1, v.cbegin() + 3);
        CHECK(valuesAre(v, {0, 4}));

        v.insert(v.cbegin() + 1, 7);
        v.emplace(v.cbegin(), 9);
        CHECK(valuesAre(v, {9, 0, 7, 4}));
    }

    SECTION("empty ranges with null pointers")
    {
        TestType v;
        v.emplaceBackRange(nullptr, 0u);
        v.unsafeEmplaceBackRange(nullptr, 0u);
        v.assignRange(nullptr, nullptr);
        CHECK(v.empty());

        const TestType fromEmpty(static_cast<const int*>(nullptr), static_cast<const int*>(nullptr));
        CHECK(fromEmpty.empty());
    }

    SECTION("pushBackMultiple within capacity")
    {
        TestType v;
        v.reserve(8);
        v.pushBackMultiple(1, 2, 3);
        v.pushBackMultiple();
        CHECK(valuesAre(v, {1, 2, 3}));
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Vector family: element-wise equality")
{
    // No self-comparison shortcut: NaN elements compare unequal, even to themselves
    const float nan = __builtin_nanf("");

    const za::Vector<float> v{1.f, nan};
    CHECK(!(v == v));

    const za::SmallVector<float, 4> sv{1.f, nan};
    CHECK(!(sv == sv));

    const za::InPlaceVector<float, 4> ipv{1.f, nan};
    CHECK(!(ipv == ipv));

    const za::Vector<float> a{1.f, 2.f};
    CHECK(a == a);
    CHECK(a == za::Vector<float>{1.f, 2.f});
    CHECK(!(a == za::Vector<float>{1.f, 3.f}));
    CHECK(!(a == za::Vector<float>{1.f}));
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Vector family: capacity policy")
{
    SECTION("Vector growth from empty, and after `reserve(1)`")
    {
        za::Vector<int> v;
        v.pushBack(0);
        CHECK(v.capacity() == 4u); // floored at 4

        za::Vector<int> r;
        r.reserve(1);
        CHECK(r.capacity() == 1u); // explicit reservations are exact when growing from empty

        r.pushBack(0);
        r.pushBack(1);
        CHECK(r.capacity() == 4u); // floored at 4 (not 1 -> 2 -> 3 -> 4)

        for (int i = 2; i < 5; ++i)
            r.pushBack(i);

        CHECK(r.capacity() == 6u); // x1.5
    }

    SECTION("Constructors and assignments allocate exactly")
    {
        za::Vector<int> big(101, 0);
        CHECK(big.capacity() == 101u);

        za::Vector<int> dst;
        dst.reserve(100);
        dst = big;
        CHECK(dst.capacity() == 101u);

        const za::SmallVector<int, 8> sv(9);
        CHECK(sv.capacity() == 9u);

        const za::SmallVector<int, 8> svCopy(sv);
        CHECK(svCopy.capacity() == 9u);

        za::SmallVector<int, 8> svAssigned;
        svAssigned = sv;
        CHECK(svAssigned.capacity() == 9u);
    }

    SECTION("SmallVector growth out of a tiny inline buffer")
    {
        za::SmallVector<int, 1> v;
        v.pushBack(0);
        CHECK(!v.isHeap());

        v.pushBack(1);
        CHECK(v.isHeap());
        CHECK(v.capacity() == 4u);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Vector family: over-aligned elements")
{
    using namespace VectorCommonTest;

    const auto isAligned = [](const void* p) { return reinterpret_cast<za::UIntPtrT>(p) % 64u == 0u; };

    za::Vector<OverAligned> v;
    for (int i = 0; i < 100; ++i)
    {
        v.pushBack(OverAligned{i});
        CHECK(isAligned(v.data()));
    }

    v.resize(10);
    v.shrinkToFit();
    CHECK(isAligned(v.data()));
    CHECK(v[9].value == 9);

    za::SmallVector<OverAligned, 2> sv;
    for (int i = 0; i < 10; ++i)
    {
        sv.pushBack(OverAligned{i});
        CHECK(isAligned(sv.data()));
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Vector family: assigning from a vector owned by an element")
{
    using namespace VectorCommonTest;

    const auto makeTree = []
    {
        za::Vector<Node> v;
        v.pushBack(Node{1, {Node{10, {}}, Node{11, {}}}});
        v.pushBack(Node{2, {}});
        return v;
    };

    SECTION("Copy")
    {
        za::Vector<Node> v = makeTree();
        v                  = v[0].children;

        REQUIRE(v.size() == 2u);
        CHECK(v[0].value == 10);
        CHECK(v[1].value == 11);
    }

    SECTION("Move")
    {
        za::Vector<Node> v = makeTree();
        v                  = ZA_MOVE(v[0].children);

        REQUIRE(v.size() == 2u);
        CHECK(v[0].value == 10);
        CHECK(v[1].value == 11);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] Vector family: traits")
{
    using namespace VectorCommonTest;

    STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::InPlaceVector<int, 4>));
    STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::InPlaceVector<Counted, 4>));

    STATIC_CHECK(za::InPlaceVector<int, 4>::capacity() == 4u);
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] SmallVector: inline/heap transitions keep the live count balanced")
{
    using namespace VectorCommonTest;

    using SV = za::SmallVector<Counted, 2>;

    const auto make = [](const int count)
    {
        SV v;
        fillWithValues(v, count);
        return v;
    };

    Counted::live = 0;

    {
        const int sizes[] = {0, 1, 2, 5};

        for (const int lhsSize : sizes)
            for (const int rhsSize : sizes)
            {
                SV copyAssigned = make(lhsSize);
                copyAssigned    = make(rhsSize);
                CHECK(copyAssigned == make(rhsSize));

                SV moveAssigned = make(lhsSize);
                SV source       = make(rhsSize);
                moveAssigned    = ZA_MOVE(source);
                CHECK(moveAssigned == make(rhsSize));
                CHECK(source.empty());

                SV lhs = make(lhsSize);
                SV rhs = make(rhsSize);
                lhs.swap(rhs);
                CHECK(lhs == make(rhsSize));
                CHECK(rhs == make(lhsSize));

                SV copied(lhs);
                CHECK(copied == lhs);
            }

        SV v = make(5);
        CHECK(v.isHeap());

        v.erase(v.cbegin() + 1, v.cbegin() + 4);
        CHECK(valuesAre(v, {0, 4}));

        v.shrinkToFit(); // back to inline storage
        CHECK(!v.isHeap());
        CHECK(valuesAre(v, {0, 4}));

        v.resize(4, Counted{9}); // grows to the heap
        CHECK(v.isHeap());
        CHECK(valuesAre(v, {0, 4, 9, 9}));

        v.insert(v.cbegin() + 1, Counted{7}); // middle insertion
        CHECK(valuesAre(v, {0, 7, 4, 9, 9}));

        v.popBack();
        v.reEmplaceByIndex(0, 5);
        CHECK(valuesAre(v, {5, 7, 4, 9}));

        v.clear();
        CHECK(v.empty());
    }

    CHECK(Counted::live == 0);
}


#ifdef __cpp_exceptions
////////////////////////////////////////////////////////////
TEMPLATE_TEST_CASE("[Base] Vector family: a throwing constructor does not leave an unconstructed element",
                   "",
                   VectorCommonTest::ThrowerVector,
                   VectorCommonTest::ThrowerSmallVector,
                   VectorCommonTest::ThrowerInPlaceVector)
{
    using namespace VectorCommonTest;

    Thrower::live = 0;

    {
        TestType v;
        v.reserve(4); // no growth below

        v.emplaceBack(1);

        bool threw = false;

        try
        {
            v.emplaceBack(Thrower::throwValue);
        } catch (int)
        {
            threw = true;
        }

        CHECK(threw);
        CHECK(v.size() == 1u);

        threw = false;

        try
        {
            v.pushBackMultiple(2, Thrower::throwValue); // the elements are constructed in place
        } catch (int)
        {
            threw = true;
        }

        CHECK(threw);
        REQUIRE(v.size() == 2u); // only the element that was constructed
        CHECK(v[0].value == 1);
        CHECK(v[1].value == 2);
    }

    CHECK(Thrower::live == 0);
}
#endif
