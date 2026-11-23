// The forward declarations first, then `<new>`: the standard library's definitions must complete and redeclare the
// same entities (a mismatched declaration fails to compile). The other order: `FwdStdNewWithStd.test.cpp`.
#include "Zancle/Base/FwdStdNew.hpp"

#include <new>

// Other headers
#include "Tst/Tst.hpp"

#include "Zancle/Trait/IsSame.hpp"


TEST_CASE("[Base] Base/FwdStdNew.hpp, then <new>")
{
    STATIC_CHECK(ZA_IS_SAME(za::AlignValT, std::align_val_t));
    STATIC_CHECK(ZA_IS_SAME(za::NothrowT, std::nothrow_t));
    STATIC_CHECK(sizeof(std::nothrow_t) == 1u); // complete now

    void* const p = ::operator new(8u, std::align_val_t{16}, std::nothrow);
    REQUIRE(p != nullptr);
    ::operator delete(p, za::AlignValT{16}, std::nothrow);
}
