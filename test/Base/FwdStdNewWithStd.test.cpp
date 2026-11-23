// `<new>` first, then the forward declarations: they must declare the same entities (a mismatched declaration fails
// to compile). The other order: `FwdStdNewThenStd.test.cpp`.
// clang-format off
#include <new>

#include "Zancle/Base/FwdStdNew.hpp"
// clang-format on

// Other headers
#include "Tst/Tst.hpp"

#include "Zancle/Trait/IsSame.hpp"


////////////////////////////////////////////////////////////
[[nodiscard]] const void* fwdStdNewNothrowAddressWithStd() noexcept
{
    return &std::nothrow;
}


namespace
{
////////////////////////////////////////////////////////////
// The forms a replacement would define, taken by address: overload resolution finds exactly the standard's
using NothrowNew        = void* (*)(decltype(sizeof(0)), const za::NothrowT&) noexcept;
using AlignedNothrowNew = void* (*)(decltype(sizeof(0)), za::AlignValT, const za::NothrowT&) noexcept;
using AlignedDelete     = void (*)(void*, za::AlignValT) noexcept;

} // namespace


TEST_CASE("[Base] Base/FwdStdNew.hpp with <new>")
{
    STATIC_CHECK(ZA_IS_SAME(za::AlignValT, std::align_val_t));
    STATIC_CHECK(ZA_IS_SAME(za::NothrowT, std::nothrow_t));

    const NothrowNew        nothrowNew        = &::operator new;
    const AlignedNothrowNew alignedNothrowNew = &::operator new;
    const AlignedDelete     alignedDelete     = &::operator delete;

    void* const p = nothrowNew(8u, std::nothrow);
    REQUIRE(p != nullptr);
    ::operator delete(p, std::nothrow);

    void* const q = alignedNothrowNew(8u, std::align_val_t{32}, std::nothrow);
    REQUIRE(q != nullptr);
    alignedDelete(q, za::AlignValT{32});
}
