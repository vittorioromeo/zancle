#include "Zancle/Base/FwdStdNew.hpp"

// Other header (no `<new>` in this translation unit: see `FwdStdNewWithStd.test.cpp` for both together)
#include "Tst/Tst.hpp"

#include "Zancle/Base/UIntPtrT.hpp"

#include "Zancle/Trait/IsSame.hpp"


////////////////////////////////////////////////////////////
// Defined in `FwdStdNewWithStd.test.cpp`, which includes `<new>`
[[nodiscard]] const void* fwdStdNewNothrowAddressWithStd() noexcept;


////////////////////////////////////////////////////////////
[[nodiscard]] const void* fwdStdNewNothrowAddress() noexcept
{
    return &std::nothrow;
}


TEST_CASE("[Base] Base/FwdStdNew.hpp")
{
    SECTION("Aliases")
    {
        STATIC_CHECK(ZA_IS_SAME(za::AlignValT, std::align_val_t));
        STATIC_CHECK(ZA_IS_SAME(za::NothrowT, std::nothrow_t));
        STATIC_CHECK(ZA_IS_SAME(decltype(std::nothrow), const std::nothrow_t));
        STATIC_CHECK(sizeof(za::AlignValT) == sizeof(decltype(sizeof(0))));
    }

    SECTION("`std::nothrow` is the standard library's object")
    {
        CHECK(fwdStdNewNothrowAddress() == fwdStdNewNothrowAddressWithStd());
    }

    SECTION("Nothrow allocation functions")
    {
        void* const p = ::operator new(16u, std::nothrow);
        REQUIRE(p != nullptr);
        ::operator delete(p, std::nothrow);

        void* const q = ::operator new[](16u, std::nothrow);
        REQUIRE(q != nullptr);
        ::operator delete[](q, std::nothrow);
    }

    SECTION("Aligned allocation functions")
    {
        void* const p = ::operator new(48u, za::AlignValT{256});
        REQUIRE(p != nullptr);
        CHECK(reinterpret_cast<za::UIntPtrT>(p) % 256u == 0u);
        ::operator delete(p, za::AlignValT{256});

        void* const q = ::operator new(48u, za::AlignValT{64}, std::nothrow);
        REQUIRE(q != nullptr);
        CHECK(reinterpret_cast<za::UIntPtrT>(q) % 64u == 0u);
        ::operator delete(q, za::AlignValT{64}, std::nothrow);

        void* const r = ::operator new[](48u, za::AlignValT{128}, std::nothrow);
        REQUIRE(r != nullptr);
        CHECK(reinterpret_cast<za::UIntPtrT>(r) % 128u == 0u);
        ::operator delete[](r, za::AlignValT{128}, std::nothrow);
    }
}
