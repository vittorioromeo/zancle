#include "Tst/Tst.hpp"

#include "Zancle/Container/Array.hpp"

#include "Zancle/Trait/EnableTrivialRelocation.hpp"
#include "Zancle/Trait/IsAggregate.hpp"
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
namespace ArrayTest // for unity builds
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
template <typename... Ts>
concept CanDeduceArray = requires(Ts... xs) { za::Array{xs...}; };


////////////////////////////////////////////////////////////
// Deduction guide: all elements must have the same type.
////////////////////////////////////////////////////////////
static_assert(za::isSame<decltype(za::Array{1, 2, 3}), za::Array<int, 3>>);
static_assert(za::isSame<decltype(za::Array{1.0f}), za::Array<float, 1>>);
static_assert(za::isSame<decltype(za::Array{"a", "bcd"}), za::Array<const char*, 2>>); // decays
static_assert(CanDeduceArray<int, int>);
static_assert(!CanDeduceArray<float, double>);
static_assert(!CanDeduceArray<unsigned int, int>);
static_assert(!CanDeduceArray<int, int, long>);


////////////////////////////////////////////////////////////
// `size()` is a static constexpr.
////////////////////////////////////////////////////////////
static_assert(za::Array<int, 1>::size() == 1u);
static_assert(za::Array<int, 7>::size() == 7u);


////////////////////////////////////////////////////////////
// Usable in constant expressions.
////////////////////////////////////////////////////////////
static_assert([]
{
    za::Array<int, 4> a{1, 2, 3, 4};
    a[2] = 10;

    int sum = 0;
    for (const int x : a)
        sum += x;

    return sum == 1 + 2 + 10 + 4 && a.size() == 4u && *a.data() == 1 && a.end() - a.begin() == 4;
}());

static_assert(za::Array{1, 2, 3} == za::Array{1, 2, 3});
static_assert(za::Array{1, 2, 3} != za::Array{1, 2, 4});

} // namespace ArrayTest
} // namespace


TEST_CASE("[Base] Container/Array.hpp")
{
    SECTION("Element access")
    {
        za::Array<int, 3> a{10, 20, 30};

        CHECK(a[0] == 10);
        CHECK(a[1] == 20);
        CHECK(a[2] == 30);

        a[1] = 25;
        CHECK(a[1] == 25);
        CHECK(a.elements[1] == 25);

        const za::Array<int, 3>& ca = a;
        CHECK(ca[0] == 10);
        CHECK(ca[1] == 25);
        CHECK(ca[2] == 30);
    }

    SECTION("Partial aggregate initialization value-initializes the rest")
    {
        const za::Array<int, 4> a{7};

        CHECK(a[0] == 7);
        CHECK(a[1] == 0);
        CHECK(a[2] == 0);
        CHECK(a[3] == 0);
    }

    SECTION("size / data / iterators")
    {
        za::Array<int, 4>        a{1, 2, 3, 4};
        const za::Array<int, 4>& ca = a;

        CHECK(a.size() == 4u);
        CHECK(ca.size() == 4u);

        CHECK(a.data() == &a[0]);
        CHECK(ca.data() == &ca[0]);

        CHECK(a.begin() == a.data());
        CHECK(a.end() == a.data() + 4);
        CHECK(ca.begin() == ca.data());
        CHECK(ca.end() == ca.data() + 4);
        CHECK(a.cbegin() == a.data());
        CHECK(a.cend() == a.data() + 4);

        STATIC_CHECK(za::isSame<decltype(a.begin()), int*>);
        STATIC_CHECK(za::isSame<decltype(ca.begin()), const int*>);
        STATIC_CHECK(za::isSame<decltype(a.cbegin()), const int*>);
        STATIC_CHECK(za::isSame<decltype(a.cend()), const int*>);

        int expected = 1;
        for (const int x : ca)
            CHECK(x == expected++);

        for (int& x : a)
            x *= 2;

        CHECK(a[0] == 2);
        CHECK(a[3] == 8);
    }

    SECTION("Equality")
    {
        const za::Array<int, 3> a{1, 2, 3};
        const za::Array<int, 3> b{1, 2, 3};
        const za::Array<int, 3> c{1, 2, 4};

        CHECK(a == b);
        CHECK(!(a != b));
        CHECK(a != c);
        CHECK(!(a == c));
    }

    SECTION("Deduction guide")
    {
        const za::Array a{1.5f, 2.5f};

        STATIC_CHECK(za::isSame<decltype(a), const za::Array<float, 2>>);
        CHECK(a.size() == 2u);
        CHECK(a[0] == 1.5f);
        CHECK(a[1] == 2.5f);
    }

    SECTION("Type traits")
    {
        using namespace ArrayTest;

        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Array<int, 5>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Array<int, 5>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Array<int, 5>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Array<int, 5>));

        STATIC_CHECK(ZA_IS_TRIVIAL(za::Array<int, 5>));
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(za::Array<int, 5>));
        STATIC_CHECK(ZA_IS_AGGREGATE(za::Array<int, 5>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Array<int, 5>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Array<int, 5>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_ASSIGNABLE(za::Array<int, 5>, za::Array<int, 5>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::Array<int, 5>));


        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Array<NonTrivial, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Array<NonTrivial, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Array<NonTrivial, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Array<NonTrivial, 5>));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Array<NonTrivial, 5>));
        STATIC_CHECK(!ZA_IS_STANDARD_LAYOUT(za::Array<NonTrivial, 5>));
        STATIC_CHECK(ZA_IS_AGGREGATE(za::Array<NonTrivial, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPYABLE(za::Array<NonTrivial, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Array<NonTrivial, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_ASSIGNABLE(za::Array<NonTrivial, 5>, za::Array<NonTrivial, 5>));

        STATIC_CHECK(!ZA_IS_TRIVIALLY_RELOCATABLE(za::Array<NonTrivial, 5>));


        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Array<NonTrivialButRelocatable, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Array<NonTrivialButRelocatable, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Array<NonTrivialButRelocatable, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Array<NonTrivialButRelocatable, 5>));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Array<NonTrivialButRelocatable, 5>));
        STATIC_CHECK(!ZA_IS_STANDARD_LAYOUT(za::Array<NonTrivialButRelocatable, 5>));
        STATIC_CHECK(ZA_IS_AGGREGATE(za::Array<NonTrivialButRelocatable, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPYABLE(za::Array<NonTrivialButRelocatable, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Array<NonTrivialButRelocatable, 5>));
        STATIC_CHECK(
            !ZA_IS_TRIVIALLY_ASSIGNABLE(za::Array<NonTrivialButRelocatable, 5>, za::Array<NonTrivialButRelocatable, 5>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::Array<NonTrivialButRelocatable, 5>));
    }
}
