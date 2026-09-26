#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/Span.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsConstructible.hpp"
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/IsStandardLayout.hpp"
#include "Zancle/Trait/IsTrivial.hpp"
#include "Zancle/Trait/IsTriviallyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyConstructible.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyMoveAssignable.hpp"
#include "Zancle/Trait/IsTriviallyMoveConstructible.hpp"
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"


namespace
{
namespace SpanTest // for unity builds
{
////////////////////////////////////////////////////////////
struct Base
{
    int a;
};


////////////////////////////////////////////////////////////
struct Derived : Base
{
    int b;
};

} // namespace SpanTest
} // namespace


TEST_CASE("[Base] Base/Span.hpp")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Span<int>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Span<int>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Span<int>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Span<int>));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Span<int>)); // because of member initializers
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(za::Span<int>));
        STATIC_CHECK(!ZA_IS_AGGREGATE(za::Span<int>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Span<int>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Span<int>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_ASSIGNABLE(za::Span<int>, za::Span<int>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::Span<int>));
    }

    SECTION("Structured bindings")
    {
        za::Span<int> span{nullptr, 0u};

        auto [data, size] = span;

        CHECK(data == nullptr);
        CHECK(size == 0u);
    }

    SECTION("Range constructor from za::Vector")
    {
        za::Vector<int> vec;
        vec.pushBack(10);
        vec.pushBack(20);
        vec.pushBack(30);

        za::Span<int> span{vec};

        CHECK(span.data() == vec.data());
        CHECK(span.size() == 3u);
        CHECK(span[0] == 10);
        CHECK(span[1] == 20);
        CHECK(span[2] == 30);
    }

    SECTION("Range constructor const-correctness")
    {
        za::Vector<int> vec;
        vec.pushBack(1);
        vec.pushBack(2);

        const za::Vector<int>& cvec = vec;

        za::Span<const int> cspan{cvec};
        CHECK(cspan.data() == cvec.data());
        CHECK(cspan.size() == 2u);

        za::Span<const int> cspan2{vec};
        CHECK(cspan2.size() == 2u);

        // Constructing a non-const Span from a non-const range is allowed.
        STATIC_CHECK(ZA_IS_CONSTRUCTIBLE(za::Span<int>, za::Vector<int>&));

        // Constructing a non-const Span from a const range must be rejected.
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::Span<int>, const za::Vector<int>&));

        // Constructing a const Span from either is allowed.
        STATIC_CHECK(ZA_IS_CONSTRUCTIBLE(za::Span<const int>, za::Vector<int>&));
        STATIC_CHECK(ZA_IS_CONSTRUCTIBLE(za::Span<const int>, const za::Vector<int>&));

        // Types lacking .data()/.size() must not match the range constructor.
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::Span<int>, int));
    }

    SECTION("data() preserves the element type's constness")
    {
        STATIC_CHECK(ZA_IS_SAME(decltype(za::Span<int>{}.data()), int*));
        STATIC_CHECK(ZA_IS_SAME(decltype(za::Span<const int>{}.data()), const int*));

        int           array[]{1, 2, 3};
        za::Span<int> span{array};

        *span.data() = 10;
        CHECK(array[0] == 10);
    }

    SECTION("Element types must match exactly (up to added cv-qualifiers)")
    {
        using namespace SpanTest;

        // A `Span<Base>` over `Derived` objects would index with the wrong stride.
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::Span<Base>, za::Vector<Derived>&));
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::Span<const Base>, const za::Vector<Derived>&));
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::Span<Base>, Derived*, za::SizeT));

        STATIC_CHECK(ZA_IS_CONSTRUCTIBLE(za::Span<Base>, Base*, za::SizeT));
        STATIC_CHECK(ZA_IS_CONSTRUCTIBLE(za::Span<const Base>, Base*, za::SizeT));
        STATIC_CHECK(ZA_IS_CONSTRUCTIBLE(za::Span<const Base>, za::Vector<Base>&));
        STATIC_CHECK(ZA_IS_CONSTRUCTIBLE(za::Span<Base>, decltype(nullptr), za::SizeT));
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::Span<Base>, const Base*, za::SizeT));
    }
}
