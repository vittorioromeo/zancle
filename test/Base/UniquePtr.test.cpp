#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/UniquePtr.hpp"

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
}
