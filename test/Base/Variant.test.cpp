#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/Variant.hpp"

#include "Zancle/Base/IndexSequence.hpp"
#include "Zancle/Base/MakeIndexSequence.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/EnableTrivialRelocation.hpp"
#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsAssignable.hpp"
#include "Zancle/Trait/IsConstructible.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsMoveAssignable.hpp"
#include "Zancle/Trait/IsMoveConstructible.hpp"
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
#include "Zancle/Trait/RemoveCVRef.hpp"


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
    ZA_ENABLE_TRIVIAL_RELOCATION;

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
struct MoveOnlyResult
{
    explicit MoveOnlyResult(int v) : value(v)
    {
    }

    MoveOnlyResult(const MoveOnlyResult&) = delete;
    MoveOnlyResult(MoveOnlyResult&&)      = default;

    int value;
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

    SECTION("linearVisit returning const and rvalue references")
    {
        using namespace VariantTest;

        static constexpr int fallback = 0;

        const za::Variant<int, float> cv{5};

        const int& r = cv.linearVisit([](const auto& x) -> const int&
        {
            if constexpr (ZA_IS_SAME(ZA_REMOVE_CVREF(decltype(x)), int))
                return x;
            else
                return fallback;
        });

        CHECK(&r == &cv.as<int>());

        za::Variant<int> v{7};
        int&& rr = static_cast<za::Variant<int>&&>(v).linearVisit([](int&& x) -> int&& { return static_cast<int&&>(x); });

        CHECK(&rr == &v.as<int>());
    }

    SECTION("linearVisit moves (never copies) a by-value result")
    {
        using namespace VariantTest;

        za::Variant<int, OtherAlt> v{5};

        Tracker::reset();
        const Tracker t = v.linearVisit([](const auto&) { return Tracker{3}; });

        CHECK(t.tag == 3);
        CHECK(Tracker::copyCtor == 0);
        CHECK(Tracker::moveCtor == 1);
        CHECK(Tracker::dtor == 1); // the internal result buffer
    }

#ifdef __cpp_exceptions
    SECTION("linearVisit does not destroy an unconstructed result if the visitor throws")
    {
        using namespace VariantTest;

        za::Variant<int, OtherAlt> v{5};

        Tracker::reset();

        bool caught = false;

        try
        {
            (void)v.linearVisit([](const auto&) -> Tracker { throw 42; });
        } catch (const int)
        {
            caught = true;
        }

        CHECK(caught);
        CHECK(Tracker::dtor == 0);
    }
#endif

    SECTION("linearVisit supports move-only results")
    {
        using namespace VariantTest;

        za::Variant<int, OtherAlt> v{OtherAlt{4}};

        const MoveOnlyResult result = v.linearVisit([](const auto& x)
        {
            if constexpr (ZA_IS_SAME(ZA_REMOVE_CVREF(decltype(x)), int))
                return MoveOnlyResult{x};
            else
                return MoveOnlyResult{x.x};
        });

        CHECK(result.value == 4);
    }

    SECTION("recursiveMatch and linearMatch")
    {
        using namespace VariantTest;

        za::Variant<int, OtherAlt> v{OtherAlt{3}};

        CHECK(v.recursiveMatch([](int x) { return x; }, [](const OtherAlt& o) { return o.x * 10; }) == 30);
        CHECK(v.linearMatch([](int x) { return x; }, [](const OtherAlt& o) { return o.x * 10; }) == 30);

        // Reference results are forwarded as-is
        int& r = v.linearMatch([](int& x) -> int& { return x; }, [](OtherAlt& o) -> int& { return o.x; });
        STATIC_CHECK(ZA_IS_SAME(decltype(v.recursiveMatch([](int& x) -> int& { return x; },
                                                          [](OtherAlt& o) -> int& { return o.x; })),
                                int&));

        CHECK(&r == &v.as<OtherAlt>().x);
    }

    SECTION("Construction and assignment only accept exact alternatives")
    {
        using namespace VariantTest;
        using V = za::Variant<int, float>;

        STATIC_CHECK(ZA_IS_CONSTRUCTIBLE(V, int));
        STATIC_CHECK(ZA_IS_CONSTRUCTIBLE(V, const float&));
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(V, double));
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(V, OtherAlt));

        STATIC_CHECK(ZA_IS_ASSIGNABLE(V&, int));
        STATIC_CHECK(ZA_IS_ASSIGNABLE(V&, const float&));
        STATIC_CHECK(!ZA_IS_ASSIGNABLE(V&, double));
        STATIC_CHECK(!ZA_IS_ASSIGNABLE(V&, OtherAlt));
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

    SECTION("Special members are only available if all alternatives support them")
    {
        struct MoveOnly // non-trivial, to exercise the user-provided special member path
        {
            MoveOnly() = default;

            // NOLINTNEXTLINE(modernize-use-equals-default)
            MoveOnly(MoveOnly&&) noexcept
            {
            }

            // NOLINTNEXTLINE(modernize-use-equals-default)
            MoveOnly& operator=(MoveOnly&&) noexcept
            {
                return *this;
            }
        };

        struct ConstMember // copy/move constructible, but not assignable
        {
            const int i;

            // NOLINTNEXTLINE(modernize-use-equals-default)
            ~ConstMember()
            {
            }
        };

        struct TrivialConstMember // same, via the trivial special member path
        {
            const int i;
        };

        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::Variant<MoveOnly, int>));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::Variant<MoveOnly, int>));
        STATIC_CHECK(ZA_IS_MOVE_CONSTRUCTIBLE(za::Variant<MoveOnly, int>));
        STATIC_CHECK(ZA_IS_MOVE_ASSIGNABLE(za::Variant<MoveOnly, int>));

        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::Variant<ConstMember, int>));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::Variant<ConstMember, int>));
        STATIC_CHECK(ZA_IS_MOVE_CONSTRUCTIBLE(za::Variant<ConstMember, int>));
        STATIC_CHECK(!ZA_IS_MOVE_ASSIGNABLE(za::Variant<ConstMember, int>));

        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::Variant<TrivialConstMember, int>));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::Variant<TrivialConstMember, int>));
        STATIC_CHECK(!ZA_IS_MOVE_ASSIGNABLE(za::Variant<TrivialConstMember, int>));

        za::Variant<MoveOnly, int> v0{MoveOnly{}};
        za::Variant<MoveOnly, int> v1{static_cast<za::Variant<MoveOnly, int>&&>(v0)};
        v0 = static_cast<za::Variant<MoveOnly, int>&&>(v1);
        CHECK(v0.is<MoveOnly>());
    }

    SECTION("Duplicate alternatives are accessed and visited by index")
    {
        using V = za::Variant<int, float, int>;

        const V v{za::inPlaceIndex<2>, 42};
        CHECK(v.hasIndex(2));
        CHECK(v.getByIndex<2>() == 42);

        CHECK(v.linearVisit([](const auto& x) { return static_cast<int>(x); }) == 42);
        CHECK(v.recursiveVisit([](const auto& x) { return static_cast<int>(x); }) == 42);

        V copy{v};
        CHECK(copy.hasIndex(2));
        CHECK(copy.getByIndex<2>() == 42);

        copy = V{za::inPlaceIndex<0>, 7};
        CHECK(copy.hasIndex(0));
        CHECK(copy.getByIndex<0>() == 7);
    }
}
