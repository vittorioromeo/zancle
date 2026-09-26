#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsMoveAssignable.hpp"
#include "Zancle/Trait/IsMoveConstructible.hpp"
#include "Zancle/Trait/IsTrivial.hpp"
#include "Zancle/Trait/IsTriviallyCopyConstructible.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyMoveAssignable.hpp"
#include "Zancle/Trait/IsTriviallyMoveConstructible.hpp"
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"


namespace
{
namespace OptionalTest // to support unity builds
{
////////////////////////////////////////////////////////////
struct Trivial
{
};


////////////////////////////////////////////////////////////
struct NonTrivial
{
    // NOLINTNEXTLINE(modernize-use-equals-default)
    NonTrivial()
    {
    }

    // NOLINTNEXTLINE(modernize-use-equals-default)
    ~NonTrivial()
    {
    }

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NonTrivial(const NonTrivial&)
    {
    }

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NonTrivial(NonTrivial&&) noexcept
    {
    }

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NonTrivial& operator=(const NonTrivial&)
    {
        return *this;
    }

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NonTrivial& operator=(NonTrivial&&) noexcept
    {
        return *this;
    }
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
struct MoveOnly
{
    MoveOnly()  = default;
    ~MoveOnly() = default;

    MoveOnly(const MoveOnly&)            = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    MoveOnly(MoveOnly&&) noexcept            = default;
    MoveOnly& operator=(MoveOnly&&) noexcept = default;
};


TEST_CASE("[Base] Base/Optional.hpp")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_TRIVIAL(Trivial));
        STATIC_CHECK(!ZA_IS_TRIVIAL(NonTrivial));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Optional<Trivial>));
        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Optional<NonTrivial>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Optional<Trivial>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPYABLE(za::Optional<NonTrivial>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Optional<Trivial>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Optional<NonTrivial>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Optional<Trivial>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Optional<NonTrivial>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Optional<Trivial>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Optional<NonTrivial>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Optional<Trivial>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Optional<NonTrivial>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Optional<Trivial>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Optional<NonTrivial>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::Optional<Trivial>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_RELOCATABLE(za::Optional<NonTrivial>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::Optional<NonTrivialButRelocatable>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(MoveOnly));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(MoveOnly));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(MoveOnly));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(MoveOnly));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(MoveOnly));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(MoveOnly));
        STATIC_CHECK(ZA_IS_MOVE_CONSTRUCTIBLE(MoveOnly));
        STATIC_CHECK(ZA_IS_MOVE_ASSIGNABLE(MoveOnly));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(MoveOnly));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(MoveOnly));

// Clang bug, see https://stackoverflow.com/questions/78885178
#ifndef __clang__
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Optional<MoveOnly>));
#endif
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Optional<MoveOnly>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Optional<MoveOnly>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Optional<MoveOnly>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Optional<MoveOnly>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Optional<MoveOnly>));
        STATIC_CHECK(ZA_IS_MOVE_CONSTRUCTIBLE(za::Optional<MoveOnly>));
        STATIC_CHECK(ZA_IS_MOVE_ASSIGNABLE(za::Optional<MoveOnly>));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::Optional<MoveOnly>));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::Optional<MoveOnly>));
    }
}

} // namespace OptionalTest
} // namespace
