#include "AlignedAllocationUtil.hpp"

#ifdef ALIGNED_ALLOCATION_UTIL_AVAILABLE

    #include "Zancle/Base/IntTypes.hpp"
    #include "Zancle/Base/SizeT.hpp"

    #ifdef _WIN32
        #include <malloc.h>
    #else
        #include <stdlib.h>
    #endif


namespace
{
////////////////////////////////////////////////////////////
// Per-thread state: trivial types, constant-initialized (usable from any allocation)
constinit thread_local bool      failureArmed      = false;
constinit thread_local bool      failureInjected   = false;
constinit thread_local za::SizeT successesLeft     = 0u;
constinit thread_local za::I64   allocationBalance = 0;


////////////////////////////////////////////////////////////
[[nodiscard]] void* allocateAligned(const std::size_t size, const std::align_val_t alignment) noexcept
{
    if (failureArmed)
    {
        if (successesLeft == 0u)
        {
            failureArmed    = false;
            failureInjected = true;
            return nullptr;
        }

        --successesLeft;
    }

    const auto align = static_cast<std::size_t>(alignment);

    #ifdef _WIN32
    void* const ptr = _aligned_malloc(size > 0u ? size : 1u, align);
    #else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, align > sizeof(void*) ? align : sizeof(void*), size > 0u ? size : 1u) != 0)
        ptr = nullptr;
    #endif

    if (ptr != nullptr)
        ++allocationBalance;

    return ptr;
}


////////////////////////////////////////////////////////////
void freeAligned(void* const ptr) noexcept
{
    if (ptr == nullptr)
        return;

    --allocationBalance;

    #ifdef _WIN32
    _aligned_free(ptr);
    #else
    free(ptr); // NOLINT(cppcoreguidelines-no-malloc, hicpp-no-malloc)
    #endif
}


////////////////////////////////////////////////////////////
[[nodiscard]] void* allocateAlignedOrThrow(const std::size_t size, const std::align_val_t alignment)
{
    if (void* const ptr = allocateAligned(size, alignment))
        return ptr;

    throw std::bad_alloc{};
}

} // namespace


////////////////////////////////////////////////////////////
void failAlignedAllocationAfter(const za::SizeT successes) noexcept
{
    failureArmed    = true;
    failureInjected = false;
    successesLeft   = successes;
}


////////////////////////////////////////////////////////////
bool stopFailingAlignedAllocations() noexcept
{
    failureArmed = false;
    return failureInjected;
}


////////////////////////////////////////////////////////////
za::I64 getAlignedAllocationBalance() noexcept
{
    return allocationBalance;
}


////////////////////////////////////////////////////////////
// Replacements of every aligned form of the global allocation functions
////////////////////////////////////////////////////////////
void* operator new(const std::size_t size, const std::align_val_t alignment)
{
    return allocateAlignedOrThrow(size, alignment);
}

void* operator new[](const std::size_t size, const std::align_val_t alignment)
{
    return allocateAlignedOrThrow(size, alignment);
}

void* operator new(const std::size_t size, const std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return allocateAligned(size, alignment);
}

void* operator new[](const std::size_t size, const std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return allocateAligned(size, alignment);
}

void operator delete(void* const ptr, std::align_val_t) noexcept
{
    freeAligned(ptr);
}

void operator delete[](void* const ptr, std::align_val_t) noexcept
{
    freeAligned(ptr);
}

void operator delete(void* const ptr, std::size_t, std::align_val_t) noexcept
{
    freeAligned(ptr);
}

void operator delete[](void* const ptr, std::size_t, std::align_val_t) noexcept
{
    freeAligned(ptr);
}

void operator delete(void* const ptr, std::align_val_t, const std::nothrow_t&) noexcept
{
    freeAligned(ptr);
}

void operator delete[](void* const ptr, std::align_val_t, const std::nothrow_t&) noexcept
{
    freeAligned(ptr);
}

#endif
