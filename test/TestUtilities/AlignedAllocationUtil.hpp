// Header for Zancle unit tests.
//
// Fault injection and accounting for the global aligned `operator new`
// (e.g. `za::Thread`'s entry blocks), to test what happens when an
// allocation or a constructor throws. `AlignedAllocationUtil.cpp` replaces
// every aligned form of the global `operator new`/`operator delete` of the
// test executables that link it, with equivalents that add the accounting
// below (nothing changes for allocations made while no failure is armed).
//
// Only available (`ALIGNED_ALLOCATION_UTIL_AVAILABLE` defined) with C++
// exceptions enabled, and where the allocation functions can be replaced:
// not with libc++ on Windows, whose headers declare them `dllimport`.

#pragma once

#ifdef __cpp_exceptions
    #include <new> // `_LIBCPP_VERSION`

    #if !(defined(_WIN32) && defined(_LIBCPP_VERSION))
        #define ALIGNED_ALLOCATION_UTIL_AVAILABLE
    #endif
#endif


#ifdef ALIGNED_ALLOCATION_UTIL_AVAILABLE

    #include "Zancle/Base/IntTypes.hpp"
    #include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// On the calling thread: let the next `successes` aligned allocations
// succeed, then make the following one throw `std::bad_alloc` (once)
////////////////////////////////////////////////////////////
void failAlignedAllocationAfter(za::SizeT successes) noexcept;


////////////////////////////////////////////////////////////
// On the calling thread: cancel a pending `failAlignedAllocationAfter`
//
// Returns `true` if the failure was injected since it was requested.
////////////////////////////////////////////////////////////
[[nodiscard]] bool stopFailingAlignedAllocations() noexcept;


////////////////////////////////////////////////////////////
// Aligned blocks allocated by the calling thread, minus aligned blocks
// it freed (from any thread's allocations)
////////////////////////////////////////////////////////////
[[nodiscard]] za::I64 getAlignedAllocationBalance() noexcept;

#endif
