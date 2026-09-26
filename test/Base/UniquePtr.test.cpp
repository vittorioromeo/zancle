#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"
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
namespace UniquePtrTest // for unity builds
{
////////////////////////////////////////////////////////////
// Stateful deleter that records which instance deleted the last object
struct TaggedDeleter
{
    static inline int lastDeleterTag{};

    int tag{};

    void operator()(int* const ptr) const noexcept
    {
        if (ptr == nullptr)
            return;

        lastDeleterTag = tag;
        delete ptr;
    }
};


////////////////////////////////////////////////////////////
// Deleter that records what its owning `UniquePtr` holds while deleting
struct ObservingDeleter
{
    static inline const za::UniquePtr<int, ObservingDeleter>* owner{};
    static inline const int*                                  ownerPtrDuringDeletion{};

    void operator()(int* const ptr) const noexcept
    {
        if (ptr == nullptr)
            return;

        ownerPtrDuringDeletion = owner->get();
        delete ptr;
    }
};

////////////////////////////////////////////////////////////
// Deleter that counts its invocations (including with null pointers)
struct CountingDeleter
{
    static inline int calls{};

    void operator()(int* const ptr) const noexcept
    {
        ++calls;
        delete ptr;
    }
};


////////////////////////////////////////////////////////////
// Deleter that is not trivially relocatable
struct NonRelocatableDeleter
{
    NonRelocatableDeleter() = default;

    // NOLINTNEXTLINE(modernize-use-equals-default)
    NonRelocatableDeleter(const NonRelocatableDeleter&)
    {
    }

    void operator()(int* const ptr) const noexcept
    {
        delete ptr;
    }
};

} // namespace UniquePtrTest
} // namespace


TEST_CASE("[Base] Base/UniquePtr.hpp")
{
    using namespace UniquePtrTest;

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::UniquePtr<int>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::UniquePtr<int>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::UniquePtr<int>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::UniquePtr<int>));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::UniquePtr<int>)); // because of member initializers
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(za::UniquePtr<int>));
        STATIC_CHECK(!ZA_IS_AGGREGATE(za::UniquePtr<int>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_COPYABLE(za::UniquePtr<int>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::UniquePtr<int>));
        STATIC_CHECK(!ZA_IS_TRIVIALLY_ASSIGNABLE(za::UniquePtr<int>, za::UniquePtr<int>));

        STATIC_CHECK(ZA_IS_TRIVIALLY_RELOCATABLE(za::UniquePtr<int>));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::UniquePtr<int>));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::UniquePtr<int, TaggedDeleter>));

        using NonRelocatablePtr = za::UniquePtr<int, NonRelocatableDeleter>;
        STATIC_CHECK(!ZA_IS_TRIVIALLY_RELOCATABLE(NonRelocatablePtr));
    }

    SECTION("The deleter is only invoked on non-null pointers")
    {
        CountingDeleter::calls = 0;

        {
            za::UniquePtr<int, CountingDeleter> p;
            p.reset();
        }

        CHECK(CountingDeleter::calls == 0);

        {
            za::UniquePtr<int, CountingDeleter> p{new int{1}};
            p.reset(new int{2});
            CHECK(CountingDeleter::calls == 1);
        }

        CHECK(CountingDeleter::calls == 2);
    }

    SECTION("Self move-assignment keeps the owned object")
    {
        auto  p   = za::makeUnique<int>(42);
        auto& ref = p;

        p = static_cast<za::UniquePtr<int>&&>(ref);

        REQUIRE(p.get() != nullptr);
        CHECK(*p == 42);
    }

    SECTION("Move-assignment destroys the old object with the old deleter")
    {
        za::UniquePtr<int, TaggedDeleter> a{new int{1}, TaggedDeleter{.tag = 1}};
        za::UniquePtr<int, TaggedDeleter> b{new int{2}, TaggedDeleter{.tag = 2}};

        TaggedDeleter::lastDeleterTag = 0;
        a                             = static_cast<za::UniquePtr<int, TaggedDeleter>&&>(b);

        CHECK(TaggedDeleter::lastDeleterTag == 1);
        CHECK(*a == 2);
        CHECK(b.get() == nullptr);

        a.reset();
        CHECK(TaggedDeleter::lastDeleterTag == 2); // the deleter was transferred along with the object
    }

    SECTION("reset() stores the new pointer before destroying the old object")
    {
        za::UniquePtr<int, ObservingDeleter> p{new int{1}};
        ObservingDeleter::owner = &p;

        int* const newPtr = new int{2};
        p.reset(newPtr);

        CHECK(ObservingDeleter::ownerPtrDuringDeletion == newPtr);
        CHECK(*p == 2);
    }

    SECTION("Move construction transfers the pointer and the deleter")
    {
        int* const raw = new int{3};

        za::UniquePtr<int, TaggedDeleter> a{raw, TaggedDeleter{.tag = 7}};
        za::UniquePtr<int, TaggedDeleter> b{static_cast<za::UniquePtr<int, TaggedDeleter>&&>(a)};

        CHECK(a.get() == nullptr);
        CHECK(b.get() == raw);

        TaggedDeleter::lastDeleterTag = 0;
        b.reset();
        CHECK(TaggedDeleter::lastDeleterTag == 7);
    }

    SECTION("Converting move construction")
    {
        struct Base
        {
            virtual ~Base() = default;
        };

        struct Derived : Base
        {
        };

        auto                d   = za::makeUnique<Derived>();
        Derived* const      raw = d.get();
        za::UniquePtr<Base> b{static_cast<za::UniquePtr<Derived>&&>(d)};

        CHECK(d.get() == nullptr);
        CHECK(b.get() == raw);
    }
}
