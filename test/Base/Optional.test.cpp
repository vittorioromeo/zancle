#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsMoveAssignable.hpp"
#include "Zancle/Trait/IsMoveConstructible.hpp"
#include "Zancle/Trait/IsSame.hpp"
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
// Constructors that throw on demand, tracking the number of live objects
struct MaybeThrows
{
    static inline int  liveCount{};
    static inline bool throwOnCopy{};

    explicit MaybeThrows(const bool doThrow)
    {
        if (doThrow)
            throw 42;

        ++liveCount;
    }

    MaybeThrows(const MaybeThrows&)
    {
        if (throwOnCopy)
            throw 42;

        ++liveCount;
    }

    MaybeThrows& operator=(const MaybeThrows&) = default;

    ~MaybeThrows()
    {
        --liveCount;
    }
};


////////////////////////////////////////////////////////////
// Copy-assignable, but not copy-constructible
struct AssignOnly
{
    AssignOnly() = default;

    AssignOnly(const AssignOnly&) = delete;

    // NOLINTNEXTLINE(modernize-use-equals-default)
    AssignOnly& operator=(const AssignOnly&)
    {
        return *this;
    }
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

        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(DtorCounter));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(DtorCounter));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(DtorCounter));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Optional<DtorCounter>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Optional<DtorCounter>));

        // Assignment may need to construct the value, so it requires constructibility
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(AssignOnly));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::Optional<AssignOnly>));
    }

    SECTION("makeOptionalFromFunc decays reference results")
    {
        int  x = 5;
        auto o = za::makeOptionalFromFunc([&]() -> int& { return x; });

        STATIC_CHECK(ZA_IS_SAME(decltype(o), za::Optional<int>));
        REQUIRE(o.hasValue());
        CHECK(*o == 5);

        *o = 6;
        CHECK(x == 5); // holds a copy
    }

    SECTION("Assigning an empty optional destroys the value (trivial assignment, non-trivial destructor)")
    {
        za::Optional<DtorCounter> a{DtorCounter{}};
        za::Optional<DtorCounter> b;

        DtorCounter::dtorCount = 0;
        a                      = b;

        CHECK(!a.hasValue());
        CHECK(DtorCounter::dtorCount == 1);

        a.emplace();

        DtorCounter::dtorCount = 0;
        a                      = static_cast<za::Optional<DtorCounter>&&>(b);

        CHECK(!a.hasValue());
        CHECK(DtorCounter::dtorCount == 1);
    }

#ifdef __cpp_exceptions
    SECTION("Stays disengaged if construction throws")
    {
        const auto throws = [](auto&& f)
        {
            try
            {
                f();
            } catch (const int)
            {
                return true;
            }

            return false;
        };

        const auto throwingFactory = []() -> MaybeThrows { throw 42; };

        MaybeThrows::liveCount = 0;

        {
            za::Optional<MaybeThrows> o;

            CHECK(throws([&] { o.emplace(true); }));
            CHECK(!o.hasValue());

            CHECK(throws([&] { o.emplaceIfNeeded(true); }));
            CHECK(!o.hasValue());

            CHECK(throws([&] { o.emplaceFromFuncIfNeeded(throwingFactory); }));
            CHECK(!o.hasValue());

            // Replacing an existing value destroys it first
            o.emplace(false);
            CHECK(throws([&] { o.emplace(true); }));
            CHECK(!o.hasValue());
            CHECK(MaybeThrows::liveCount == 0);

            o.emplace(false);
            CHECK(throws([&] { o.emplaceFromFunc(throwingFactory); }));
            CHECK(!o.hasValue());
            CHECK(MaybeThrows::liveCount == 0);

            // Copy-assigning into an empty optional (move operations are `noexcept`)
            za::Optional<MaybeThrows> src;
            src.emplace(false);

            MaybeThrows::throwOnCopy = true;
            CHECK(throws([&] { o = src; }));
            CHECK(!o.hasValue());
            MaybeThrows::throwOnCopy = false;
        }

        CHECK(MaybeThrows::liveCount == 0); // no double destruction, no leak
    }
#endif
}

} // namespace OptionalTest
} // namespace
