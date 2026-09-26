#include "Tst/Tst.hpp"

#include "Zancle/Container/Array.hpp"

#include "Zancle/Trait/EnableTrivialRelocation.hpp"
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

} // namespace ArrayTest
} // namespace


TEST_CASE("[Base] Base/Array.hpp")
{
    SECTION("Type traits")
    {
        using namespace ArrayTest;

        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Array<int, 5>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Array<int, 5>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Array<int, 5>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Array<int, 5>));

        STATIC_CHECK(ZA_IS_TRIVIAL(za::Array<int, 5>)); // because of member initializers
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

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Array<NonTrivial, 5>)); // because of member initializers
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

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Array<NonTrivialButRelocatable, 5>)); // because of member initializers
        STATIC_CHECK(!ZA_IS_STANDARD_LAYOUT(za::Array<NonTrivialButRelocatable, 5>));
        STATIC_CHECK(ZA_IS_AGGREGATE(za::Array<NonTrivialButRelocatable, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPYABLE(za::Array<NonTrivialButRelocatable, 5>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Array<NonTrivialButRelocatable, 5>));
        STATIC_CHECK(
            !ZA_IS_TRIVIALLY_ASSIGNABLE(za::Array<NonTrivialButRelocatable, 5>, za::Array<NonTrivialButRelocatable, 5>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::Array<NonTrivialButRelocatable, 5>));
    }
}
