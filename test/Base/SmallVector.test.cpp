#include "Tst/Tst.hpp"

#include "Zancle/Container/SmallVector.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/PlacementNew.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Swap.hpp"

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
namespace SmallVectorTest // for unity builds
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

    bool operator==(const Obj& rhs) const
    {
        return value == rhs.value;
    }

    friend void swap(Obj& lhs, Obj& rhs) noexcept // used (found via ADL)
    {
        za::genericSwap(lhs.value, rhs.value);
    }
};


constexpr za::SizeT inlineCapacity = 5;

TEST_CASE("[Base] Base/SmallVector.hpp")
{
    SECTION("Type traits")
    {
        using T = za::SmallVector<Obj, inlineCapacity>;

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

        // `Obj` is not trivially relocatable (user-defined move ctor and
        // destructor, no opt-in), so neither is a `SmallVector` of it: in
        // inline mode the elements live inside the object, and a `memcpy`
        // would bypass their move constructors.
        STATIC_CHECK(!ZA_IS_TRIVIALLY_RELOCATABLE(T));

        // A `SmallVector` of a trivially relocatable element type, however,
        // remains trivially relocatable (the inline elements relocate by
        // `memcpy`, and the `nullptr`-means-inline bookkeeping is recomputed
        // from the new `this`).
        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::SmallVector<int, inlineCapacity>));
    }

    SECTION("Inline Behavior (Size <= N)")
    {
        resetCounters();

        {
            za::SmallVector<Obj, inlineCapacity> vec;

            CHECK(vec.empty());
            CHECK(vec.size() == 0);
            CHECK(vec.capacity() == inlineCapacity);

            vec.emplaceBack(1);
            vec.emplaceBack(2);

            CHECK(vec.size() == 2);
            CHECK(vec.capacity() == inlineCapacity);
            CHECK(vec[0].value == 1);
            CHECK(vec[1].value == 2);

            // Check construction counts
            CHECK(intCtorCount == 2);
            CHECK(moveCtorCount == 0); // Constructed in place
            CHECK(dtorCount == 0);
        }

        // Destructor check at scope end
        CHECK(dtorCount == 2);
    }

    SECTION("Growth (Inline -> Heap)")
    {
        resetCounters();

        {
            za::SmallVector<Obj, inlineCapacity> vec;

            // Fill inline capacity
            for (int i = 0; i < 5; ++i)
                vec.emplaceBack(i);

            CHECK(vec.size() == 5);
            CHECK(vec.capacity() == inlineCapacity);
            CHECK(intCtorCount == 5);

            resetCounters();

            // Push one more to trigger allocation
            vec.emplaceBack(5);

            CHECK(vec.size() == 6);
            CHECK(vec.capacity() > inlineCapacity);

            // Verify data preservation
            for (za::SizeT i = 0u; i < 6; ++i)
                CHECK(vec[i].value == i);

            // Check movement cost
            // 1 new construction (5)
            // 5 moves (0-4) from inline to heap
            // 5 destructors for inline elements
            CHECK(intCtorCount == 1);
            CHECK(moveCtorCount == 5);
            CHECK(dtorCount == 5);
        }
    }

    SECTION("Reserve")
    {
        za::SmallVector<Obj, inlineCapacity> vec;

        // Reserve within inline capacity should do nothing
        vec.reserve(3);
        CHECK(vec.capacity() == inlineCapacity);

        // Reserve exceeding inline capacity triggers allocation
        vec.reserve(10);
        CHECK(vec.capacity() >= 10);
    }

    SECTION("ShrinkToFit (Heap -> Inline)")
    {
        za::SmallVector<Obj, inlineCapacity> vec;
        vec.reserve(20); // Force heap
        vec.emplaceBack(1);
        vec.emplaceBack(2);

        CHECK(vec.size() == 2);
        CHECK(vec.capacity() >= 20);

        resetCounters();
        vec.shrinkToFit();

        // Should fit back into inline storage
        CHECK(vec.size() == 2);
        CHECK(vec.capacity() == inlineCapacity);
        CHECK(vec[0].value == 1);
        CHECK(vec[1].value == 2);

        // Cost: 2 moves (heap->inline), 2 dtors (heap versions)
        CHECK(moveCtorCount == 2);
        CHECK(dtorCount == 2);
    }

    SECTION("Swap")
    {
        SECTION("Inline <-> Inline")
        {
            za::SmallVector<Obj, inlineCapacity> v1;
            v1.emplaceBack(1);
            v1.emplaceBack(2);

            za::SmallVector<Obj, inlineCapacity> v2;
            v2.emplaceBack(10);

            resetCounters();
            v1.swap(v2);

            CHECK(v1.size() == 1);
            CHECK(v1[0].value == 10);
            CHECK(v2.size() == 2);
            CHECK(v2[0].value == 1);
            CHECK(v2[1].value == 2);

            // Swap 1 element, move-construct 1, destroy 1
            // Exact counts depend on swap implementation details (member swap vs explicit moves)
            // But ensure correctness above all.
        }

        SECTION("Heap <-> Heap")
        {
            za::SmallVector<Obj, inlineCapacity> v1;
            v1.reserve(20);
            v1.emplaceBack(1);

            za::SmallVector<Obj, inlineCapacity> v2;
            v2.reserve(20);
            v2.emplaceBack(10);

            resetCounters();
            v1.swap(v2);

            CHECK(v1[0].value == 10);
            CHECK(v2[0].value == 1);
            // Pointer swap: cheap
            CHECK(moveCtorCount == 0);
            CHECK(copyCtorCount == 0);
        }

        SECTION("Heap <-> Inline")
        {
            za::SmallVector<Obj, inlineCapacity> vHeap;
            vHeap.reserve(20);
            vHeap.emplaceBack(100);

            za::SmallVector<Obj, inlineCapacity> vInline;
            vInline.emplaceBack(1);
            vInline.emplaceBack(2);

            resetCounters();
            vHeap.swap(vInline);

            // vHeap is now inline-sized (holding vInline's original data)
            // vInline is now heap-sized (holding vHeap's original data)

            CHECK(vHeap.size() == 2);
            CHECK(vHeap.capacity() == inlineCapacity); // Transitioned to inline
            CHECK(vHeap[0].value == 1);
            CHECK(vHeap[1].value == 2);

            CHECK(vInline.size() == 1);
            CHECK(vInline.capacity() >= 20); // Inherited heap buffer
            CHECK(vInline[0].value == 100);

            // Moves occurred for inline data (to vHeap's inline storage)
            // Pointers swapped for heap data
        }
    }

    SECTION("Move Semantics")
    {
        SECTION("Move Constructor (From Heap)")
        {
            za::SmallVector<Obj, inlineCapacity> src;
            src.reserve(20);
            src.emplaceBack(1);

            resetCounters();
            za::SmallVector<Obj, inlineCapacity> dst(ZA_MOVE(src));

            // Should steal pointers
            CHECK(dst.size() == 1);
            CHECK(dst.capacity() >= 20);
            CHECK(moveCtorCount == 0);               // No element moves
            CHECK(src.capacity() == inlineCapacity); // Reset
        }

        SECTION("Move Constructor (From Inline)")
        {
            za::SmallVector<Obj, inlineCapacity> src;
            src.emplaceBack(1);

            resetCounters();
            za::SmallVector<Obj, inlineCapacity> dst(ZA_MOVE(src));

            CHECK(dst.size() == 1);
            CHECK(dst.capacity() == inlineCapacity);
            CHECK(moveCtorCount == 1); // Moved element
        }
    }

    SECTION("Emplace/Insert Operations")
    {
        za::SmallVector<int, 5> vec;
        vec.emplaceBack(1);
        vec.emplaceBack(3);

        // Insert in middle
        vec.emplace(vec.begin() + 1, 2);

        CHECK(vec.size() == 3);
        CHECK(vec[0] == 1);
        CHECK(vec[1] == 2);
        CHECK(vec[2] == 3);
    }

    SECTION("Initializer List")
    {
        za::SmallVector<int, 5> vec = {1, 2, 3};
        CHECK(vec.size() == 3);
        CHECK(vec.capacity() == 5);
        CHECK(vec[2] == 3);

        za::SmallVector<int, 2> vec2 = {1, 2, 3};
        CHECK(vec2.size() == 3);
        CHECK(vec2.capacity() >= 3); // Forced heap allocation
    }

    SECTION("Construct from size")
    {
        resetCounters();

        {
            za::SmallVector<Obj, inlineCapacity> vec(3);

            CHECK(vec.size() == 3);
            CHECK(vec.capacity() == inlineCapacity);

            // Check construction counts
            CHECK(defaultCtorCount == 3);
            CHECK(dtorCount == 0);
        }

        CHECK(dtorCount == 3);
    }

    SECTION("Swap")
    {
        resetCounters();

        {
            za::SmallVector<Obj, inlineCapacity> v1(3);
            za::SmallVector<Obj, inlineCapacity> v2(2);

            CHECK(defaultCtorCount == 5);
            CHECK(dtorCount == 0);

            v1.swap(v2);

            CHECK(defaultCtorCount == 5);
            CHECK(dtorCount == 1);

            CHECK(v1.size() == 2);
            CHECK(v2.size() == 3);
        }

        CHECK(dtorCount == 6);
    }

    SECTION("Trivial relocation (memcpy) - inline mode")
    {
        // Simulate what happens when an outer Vector<SmallVector> grows and
        // trivially relocates (memcpy) a SmallVector that is using inline storage.
        using SV = za::SmallVector<int, 4>;
        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(SV));

        alignas(SV) unsigned char srcBuf[sizeof(SV)];
        alignas(SV) unsigned char dstBuf[sizeof(SV)];

        // Construct a SmallVector in srcBuf using inline storage
        SV* src = ZA_PLACEMENT_NEW(&srcBuf) SV();
        src->emplaceBack(10);
        src->emplaceBack(20);
        src->emplaceBack(30);

        CHECK(src->size() == 3);
        CHECK(src->capacity() == 4);
        CHECK(!src->isHeap());

        // Trivially relocate: memcpy to dstBuf, skip destructor on src
        ZA_MEMCPY(dstBuf, srcBuf, sizeof(SV));
        // Do NOT call src->~SV() -- that's the point of trivial relocation

        // The relocated SmallVector should work correctly from dstBuf
        SV* dst = reinterpret_cast<SV*>(dstBuf);

        CHECK(dst->size() == 3);
        CHECK(dst->capacity() == 4);
        CHECK(!dst->isHeap());
        CHECK((*dst)[0] == 10);
        CHECK((*dst)[1] == 20);
        CHECK((*dst)[2] == 30);

        // Can still mutate
        dst->emplaceBack(40);
        CHECK(dst->size() == 4);
        CHECK((*dst)[3] == 40);

        dst->~SV();
    }

    SECTION("Trivial relocation (memcpy) - heap mode")
    {
        using SV = za::SmallVector<int, 2>;

        alignas(SV) unsigned char srcBuf[sizeof(SV)];
        alignas(SV) unsigned char dstBuf[sizeof(SV)];

        // Construct a SmallVector that spills to heap
        SV* src = ZA_PLACEMENT_NEW(&srcBuf) SV();
        src->emplaceBack(1);
        src->emplaceBack(2);
        src->emplaceBack(3); // triggers heap allocation

        CHECK(src->size() == 3);
        CHECK(src->isHeap());

        // Trivially relocate
        ZA_MEMCPY(dstBuf, srcBuf, sizeof(SV));

        SV* dst = reinterpret_cast<SV*>(dstBuf);

        CHECK(dst->size() == 3);
        CHECK(dst->isHeap());
        CHECK((*dst)[0] == 1);
        CHECK((*dst)[1] == 2);
        CHECK((*dst)[2] == 3);

        dst->~SV();
    }

    SECTION("Trivial relocation via Vector<SmallVector> growth")
    {
        // End-to-end test: a Vector of SmallVectors will use trivial
        // relocation when it grows, exercising the real code path.
        using SV = za::SmallVector<int, 4>;

        za::Vector<SV> outer;

        // Add enough SmallVectors to trigger multiple reallocations
        for (za::SizeT i = 0u; i < 20u; ++i)
        {
            SV sv;
            sv.emplaceBack(static_cast<int>(i * 10u));
            sv.emplaceBack(static_cast<int>(i * 10u + 1u));
            sv.emplaceBack(static_cast<int>(i * 10u + 2u));

            outer.emplaceBack(ZA_MOVE(sv));
        }

        // Verify all data survived the relocations
        for (za::SizeT i = 0u; i < 20u; ++i)
        {
            CHECK(outer[i].size() == 3);
            CHECK(outer[i][0] == static_cast<int>(i * 10u));
            CHECK(outer[i][1] == static_cast<int>(i * 10u + 1u));
            CHECK(outer[i][2] == static_cast<int>(i * 10u + 2u));
        }
    }

    SECTION("Self-aliasing: pushBack from own element (inline, no reallocation)")
    {
        za::SmallVector<Obj, inlineCapacity> v;
        v.emplaceBack(10);
        v.emplaceBack(20);
        v.emplaceBack(30);
        REQUIRE(v.size() < v.capacity()); // still inline, room to spare

        v.pushBack(v[0]);

        CHECK(v.size() == 4);
        CHECK(v[0].value == 10);
        CHECK(v[1].value == 20);
        CHECK(v[2].value == 30);
        CHECK(v[3].value == 10);
    }

    SECTION("Self-aliasing: pushBack from own element (triggers heap allocation)")
    {
        za::SmallVector<Obj, inlineCapacity> v;
        for (za::SizeT i = 0; i < inlineCapacity; ++i)
            v.emplaceBack(static_cast<int>(i * 10));
        REQUIRE(v.size() == v.capacity()); // full inline buffer

        v.pushBack(v[0]); // must reallocate to heap

        CHECK(v.size() == inlineCapacity + 1);
        CHECK(v[0].value == 0);
        CHECK(v[inlineCapacity].value == 0);
    }

    SECTION("Self-aliasing: insert at begin from last element")
    {
        za::SmallVector<Obj, inlineCapacity> v;
        v.emplaceBack(10);
        v.emplaceBack(20);
        v.emplaceBack(30);
        REQUIRE(v.size() < v.capacity());

        v.insert(v.begin(), v[2]);

        CHECK(v.size() == 4);
        CHECK(v[0].value == 30);
        CHECK(v[1].value == 10);
        CHECK(v[2].value == 20);
        CHECK(v[3].value == 30);
    }

    SECTION("Self-aliasing: insert at middle from element that gets shifted")
    {
        za::SmallVector<Obj, inlineCapacity> v;
        v.emplaceBack(10);
        v.emplaceBack(20);
        v.emplaceBack(30);
        REQUIRE(v.size() < v.capacity());

        v.insert(v.begin() + 1, v[2]);

        CHECK(v.size() == 4);
        CHECK(v[0].value == 10);
        CHECK(v[1].value == 30);
        CHECK(v[2].value == 20);
        CHECK(v[3].value == 30);
    }

    SECTION("Self-aliasing: emplace at begin from back()")
    {
        za::SmallVector<Obj, inlineCapacity> v;
        v.emplaceBack(10);
        v.emplaceBack(20);
        v.emplaceBack(30);
        REQUIRE(v.size() < v.capacity());

        const Obj& backRef = v.back();
        v.emplace(v.begin(), backRef);

        CHECK(v.size() == 4);
        CHECK(v[0].value == 30);
        CHECK(v[1].value == 10);
        CHECK(v[2].value == 20);
        CHECK(v[3].value == 30);
    }

    SECTION("Self-aliasing: emplaceBack from own element (triggers heap allocation)")
    {
        za::SmallVector<Obj, inlineCapacity> v;
        for (za::SizeT i = 0; i < inlineCapacity; ++i)
            v.emplaceBack(static_cast<int>((i + 1) * 10));
        REQUIRE(v.size() == v.capacity());

        const Obj& ref = v[1];
        v.emplaceBack(ref);

        CHECK(v.size() == inlineCapacity + 1);
        CHECK(v[1].value == 20);
        CHECK(v[inlineCapacity].value == 20);
    }
}

} // namespace SmallVectorTest
} // namespace
