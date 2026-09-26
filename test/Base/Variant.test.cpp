#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/Variant.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
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

#include "Zancle/Base/IndexSequence.hpp"
#include "Zancle/Base/MakeIndexSequence.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace
{
namespace VariantTest // for unity builds
{
////////////////////////////////////////////////////////////
struct NonTrivial
{
    static inline int si{};

    NonTrivial(const NonTrivial&) : i(si)
    {
    }

    // NOLINTNEXTLINE(modernize-use-equals-default)
    ~NonTrivial()
    {
    }

    int& i; // NOLINT(cppcoreguidelines-use-default-member-init, modernize-use-default-member-init)
};


////////////////////////////////////////////////////////////
struct NonTrivialButRelocatable
{
    using TriviallyRelocatableTag = NonTrivialButRelocatable;

    static inline int si{};

    NonTrivialButRelocatable(const NonTrivialButRelocatable&) : i(si)
    {
    }

    // NOLINTNEXTLINE(modernize-use-equals-default)
    ~NonTrivialButRelocatable()
    {
    }

    int& i; // NOLINT(cppcoreguidelines-use-default-member-init, modernize-use-default-member-init)
};


////////////////////////////////////////////////////////////
// Counts special-member invocations so tests can assert which path
// the variant's assignment operators took (destroy+construct vs.
// same-alternative assignment fast path).
struct Tracker
{
    static inline int copyCtor   = 0;
    static inline int moveCtor   = 0;
    static inline int copyAssign = 0;
    static inline int moveAssign = 0;
    static inline int dtor       = 0;

    int tag = 0;

    Tracker() noexcept = default;
    explicit Tracker(int t) noexcept : tag(t)
    {
    }
    Tracker(const Tracker& other) noexcept : tag(other.tag)
    {
        ++copyCtor;
    }
    Tracker(Tracker&& other) noexcept : tag(other.tag)
    {
        ++moveCtor;
    }
    Tracker& operator=(const Tracker& other) noexcept
    {
        tag = other.tag;
        ++copyAssign;
        return *this;
    }
    Tracker& operator=(Tracker&& other) noexcept
    {
        tag = other.tag;
        ++moveAssign;
        return *this;
    }
    ~Tracker() noexcept
    {
        ++dtor;
    }

    static void reset() noexcept
    {
        copyCtor = moveCtor = copyAssign = moveAssign = dtor = 0;
    }
};


////////////////////////////////////////////////////////////
struct OtherAlt
{
    int x = 0;
};


////////////////////////////////////////////////////////////
// Trivially copy/move assignable, but with a non-trivial destructor
struct DtorCounter
{
    static inline int dtorCount{};

    DtorCounter() = default;

    DtorCounter(const DtorCounter&)            = default;
    DtorCounter& operator=(const DtorCounter&) = default;

    DtorCounter(DtorCounter&&) noexcept            = default;
    DtorCounter& operator=(DtorCounter&&) noexcept = default;

    ~DtorCounter()
    {
        ++dtorCount;
    }
};


////////////////////////////////////////////////////////////
template <za::SizeT I>
struct IndexedAlt
{
    za::SizeT value = I;
};


////////////////////////////////////////////////////////////
template <typename>
struct IndexedVariantImpl;

template <za::SizeT... Is>
struct IndexedVariantImpl<za::IndexSequence<Is...>>
{
    using type = za::Variant<IndexedAlt<Is>...>;
};

template <za::SizeT N>
using IndexedVariant = typename IndexedVariantImpl<za::MakeIndexSequence<N>>::type;


////////////////////////////////////////////////////////////
template <za::SizeT N>
[[nodiscard]] bool recursiveVisitHitsEveryAlternative()
{
    return []<za::SizeT... Is>(za::IndexSequence<Is...>)
    {
        return (... &&
                (IndexedVariant<N>{za::inPlaceIndex<Is>}.recursiveVisit([](const auto& alt) { return alt.value; }) == Is));
    }(za::MakeIndexSequence<N>{});
}

} // namespace VariantTest
} // namespace


TEST_CASE("[Base] Base/Variant.hpp")
{
    SECTION("Type traits")
    {
        using namespace VariantTest;

        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Variant<int, char>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Variant<int, char>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Variant<int, char>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Variant<int, char>));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Variant<int, char>));
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(za::Variant<int, char>));
        STATIC_CHECK(!ZA_IS_AGGREGATE(za::Variant<int, char>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Variant<int, char>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Variant<int, char>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_ASSIGNABLE(za::Variant<int, char>, za::Variant<int, char>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::Variant<int, char>));


        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Variant<NonTrivial, char>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Variant<NonTrivial, char>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Variant<NonTrivial, char>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Variant<NonTrivial, char>));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Variant<NonTrivial, char>));
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(za::Variant<NonTrivial, char>));
        STATIC_CHECK(!ZA_IS_AGGREGATE(za::Variant<NonTrivial, char>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPYABLE(za::Variant<NonTrivial, char>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Variant<NonTrivial, char>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_ASSIGNABLE(za::Variant<NonTrivial, char>, za::Variant<NonTrivial, char>));

        STATIC_CHECK(!ZA_IS_TRIVIALLY_RELOCATABLE(za::Variant<NonTrivial, char>));


        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Variant<NonTrivialButRelocatable, char>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Variant<NonTrivialButRelocatable, char>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Variant<NonTrivialButRelocatable, char>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Variant<NonTrivialButRelocatable, char>));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Variant<NonTrivialButRelocatable, char>));
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(za::Variant<NonTrivialButRelocatable, char>));
        STATIC_CHECK(!ZA_IS_AGGREGATE(za::Variant<NonTrivialButRelocatable, char>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPYABLE(za::Variant<NonTrivialButRelocatable, char>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Variant<NonTrivialButRelocatable, char>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_ASSIGNABLE(za::Variant<NonTrivialButRelocatable, char>,
                                                 za::Variant<NonTrivialButRelocatable, char>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::Variant<NonTrivialButRelocatable, char>));
    }

    SECTION("Copy-assign fast path (same alternative)")
    {
        using namespace VariantTest;
        using V = za::Variant<Tracker, OtherAlt>;

        V a{Tracker{1}};
        V b{Tracker{2}};

        Tracker::reset();
        a = b;

        CHECK(Tracker::copyAssign == 1);
        CHECK(Tracker::copyCtor == 0);
        CHECK(Tracker::dtor == 0);
        CHECK(a.is<Tracker>());
        CHECK(a.as<Tracker>().tag == 2);
    }

    SECTION("Move-assign fast path (same alternative)")
    {
        using namespace VariantTest;
        using V = za::Variant<Tracker, OtherAlt>;

        V a{Tracker{1}};
        V b{Tracker{2}};

        Tracker::reset();
        a = static_cast<V&&>(b);

        CHECK(Tracker::moveAssign == 1);
        CHECK(Tracker::moveCtor == 0);
        CHECK(Tracker::dtor == 0);
        CHECK(a.is<Tracker>());
        CHECK(a.as<Tracker>().tag == 2);
    }

    SECTION("Copy-assign across different alternatives still destroys + constructs")
    {
        using namespace VariantTest;
        using V = za::Variant<Tracker, OtherAlt>;

        V a{Tracker{1}};
        V b{OtherAlt{5}};

        Tracker::reset();
        a = b;

        CHECK(Tracker::dtor == 1);
        CHECK(Tracker::copyAssign == 0);
        CHECK(a.is<OtherAlt>());
        CHECK(a.as<OtherAlt>().x == 5);
    }

    SECTION("operator=(T&&) is aliasing-safe when x refers to the active alternative")
    {
        using namespace VariantTest;
        using V = za::Variant<Tracker, OtherAlt>;

        V v{Tracker{42}};

        Tracker::reset();
        // `x` binds to the currently-held value. The same-alternative fast
        // path avoids destroying storage that `x` aliases.
        v = v.as<Tracker>();

        CHECK(v.is<Tracker>());
        CHECK(v.as<Tracker>().tag == 42);
        CHECK(Tracker::copyAssign == 1);
        CHECK(Tracker::dtor == 0);
    }

    SECTION("operator=(T&&) changes alternative on type mismatch")
    {
        using namespace VariantTest;
        using V = za::Variant<Tracker, OtherAlt>;

        V v{Tracker{1}};

        Tracker::reset();
        v = OtherAlt{7};

        CHECK(Tracker::dtor == 1);
        CHECK(v.is<OtherAlt>());
        CHECK(v.as<OtherAlt>().x == 7);
    }

    SECTION("Assignment across alternatives destroys the old value (trivial assignment, non-trivial destructor)")
    {
        using namespace VariantTest;
        using V = za::Variant<int, DtorCounter>;

        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(V));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(V));

        V a{DtorCounter{}};
        V b{5};

        DtorCounter::dtorCount = 0;
        a                      = b;

        CHECK(a.is<int>());
        CHECK(a.as<int>() == 5);
        CHECK(DtorCounter::dtorCount == 1);

        a = DtorCounter{};

        DtorCounter::dtorCount = 0;
        a                      = static_cast<V&&>(b);

        CHECK(a.is<int>());
        CHECK(DtorCounter::dtorCount == 1);
    }

    SECTION("recursiveVisit dispatches correctly for any alternative count")
    {
        using namespace VariantTest;

        // Counts around the boundaries of the 5-way and 10-way unrolled chunks
        CHECK(recursiveVisitHitsEveryAlternative<1>());
        CHECK(recursiveVisitHitsEveryAlternative<4>());
        CHECK(recursiveVisitHitsEveryAlternative<5>());
        CHECK(recursiveVisitHitsEveryAlternative<6>());
        CHECK(recursiveVisitHitsEveryAlternative<9>());
        CHECK(recursiveVisitHitsEveryAlternative<10>());
        CHECK(recursiveVisitHitsEveryAlternative<11>());
        CHECK(recursiveVisitHitsEveryAlternative<15>());
        CHECK(recursiveVisitHitsEveryAlternative<16>());
        CHECK(recursiveVisitHitsEveryAlternative<20>());
        CHECK(recursiveVisitHitsEveryAlternative<21>());
    }
}
