#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/InPlacePImpl.hpp"

#include "Zancle/Base/MaxAlignT.hpp"


namespace
{
namespace InPlacePImplTest // for unity builds
{
////////////////////////////////////////////////////////////
struct Impl
{
    static inline int copies{};
    static inline int moves{};

    int value;

    explicit Impl(int v) : value(v)
    {
    }

    Impl(const Impl& rhs) : value(rhs.value)
    {
        ++copies;
    }

    Impl(Impl&& rhs) noexcept : value(rhs.value)
    {
        ++moves;
    }

    Impl& operator=(const Impl&) = default;
    Impl& operator=(Impl&&)      = default;
};

} // namespace InPlacePImplTest
} // namespace


TEST_CASE("[Base] Base/InPlacePImpl.hpp")
{
    using namespace InPlacePImplTest;
    using PImpl = za::InPlacePImpl<Impl, sizeof(Impl)>;

    SECTION("Alignment")
    {
        STATIC_CHECK(alignof(PImpl) == alignof(za::MaxAlignT));

        using TightPImpl = za::InPlacePImpl<Impl, sizeof(Impl), alignof(Impl)>;
        STATIC_CHECK(alignof(TightPImpl) == alignof(Impl));
        STATIC_CHECK(sizeof(TightPImpl) == sizeof(Impl));

        const TightPImpl p{3};
        CHECK(p->value == 3);
    }

    SECTION("Construction forwards arguments to the implementation")
    {
        const PImpl p{42};

        CHECK(p->value == 42);
        CHECK((*p).value == 42);
    }

    SECTION("Copy construction from a non-const lvalue copies the implementation")
    {
        PImpl p{7};

        Impl::copies = 0;
        PImpl copy(p); // NOLINT(performance-unnecessary-copy-initialization)

        CHECK(copy->value == 7);
        CHECK(Impl::copies == 1);
    }

    SECTION("Move construction moves the implementation")
    {
        PImpl p{9};

        Impl::moves = 0;
        PImpl moved(static_cast<PImpl&&>(p));

        CHECK(moved->value == 9);
        CHECK(Impl::moves == 1);
    }
}
