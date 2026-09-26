// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Config.hpp"

#include "Zancle/Concurrency/Atomic.hpp"

#if !defined(ZA_SYSTEM_LINUX) && !defined(ZA_SYSTEM_ANDROID) && !defined(ZA_SYSTEM_WINDOWS) && \
    !defined(ZA_SYSTEM_EMSCRIPTEN)

    #include "Zancle/Base/CpuRelax.hpp"

#endif


#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/InterferenceSize.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/UIntPtrT.hpp"


////////////////////////////////////////////////////////////
// Platform-specific includes
////////////////////////////////////////////////////////////
#if defined(ZA_SYSTEM_LINUX) || defined(ZA_SYSTEM_ANDROID)

    #include <linux/futex.h>
    #include <sys/syscall.h>
    #include <unistd.h>

    #include <ctime>

#elif defined(ZA_SYSTEM_WINDOWS)

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #include "Zancle/Base/WindowsHeader.hpp"

    #include <synchapi.h>

#elif defined(ZA_SYSTEM_EMSCRIPTEN)

    #include <emscripten/threading.h>

#endif


////////////////////////////////////////////////////////////
// Generic fallback (used by macOS / unknown platforms)
////////////////////////////////////////////////////////////
#if !defined(ZA_SYSTEM_LINUX) && !defined(ZA_SYSTEM_ANDROID) && !defined(ZA_SYSTEM_WINDOWS) && \
    !defined(ZA_SYSTEM_EMSCRIPTEN)
    #include <sched.h>
#endif


namespace za::priv
{
namespace
{
////////////////////////////////////////////////////////////
/// \brief Per-address contention slot
///
/// Tracks the number of threads currently parked on (or about to park
/// on) a given address. Every slot lives on its own cache line to
/// avoid false sharing between unrelated atomics whose addresses
/// happen to hash to neighbouring entries.
///
////////////////////////////////////////////////////////////
struct alignas(za::hardwareDestructiveInterferenceSize) ContentionSlot
{
    Atomic<za::U32> waiters{0u};
};


////////////////////////////////////////////////////////////
constexpr za::SizeT slotCount = 256u;


////////////////////////////////////////////////////////////
/// \brief Global parking-lot table
///
/// Indexed by a hash of the user's atomic address. Two atomics may
/// hash to the same slot, which causes a benign over-count of waiters
/// (a notify may issue a syscall that finds nobody to wake, but
/// correctness is unaffected -- `notifyOne`/`notifyAll` never miss a
/// real waiter).
///
////////////////////////////////////////////////////////////
constinit ContentionSlot slots[slotCount];


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::pure]] inline ContentionSlot& slotFor(const void* const addr) noexcept
{
    // Murmur3 64-bit finalizer -- gives good entropy even when most of
    // the input bits are predictable (heap addresses share a prefix).
    auto h = static_cast<za::U64>(reinterpret_cast<za::UIntPtrT>(addr));
    h ^= h >> 33;
    h *= 0xff'51'af'd7'ed'55'8c'cdULL;
    h ^= h >> 33;
    h *= 0xc4'ce'b9'fe'1a'85'ec'53ULL;
    h ^= h >> 33;

    return slots[h & (slotCount - 1u)];
}


#if !defined(ZA_SYSTEM_LINUX) && !defined(ZA_SYSTEM_ANDROID) && !defined(ZA_SYSTEM_WINDOWS) && \
    !defined(ZA_SYSTEM_EMSCRIPTEN)

////////////////////////////////////////////////////////////
template <typename T>
inline void spinWait(const T* const addr, const T expected) noexcept
{
    // Yield-based fallback used on platforms where we don't (yet)
    // have a native wait primitive (e.g. macOS pre-14.4). Burns CPU
    // proportionally to the number of waiters; paired with a matching
    // `notify` it just collapses into a busy-poll until the change is
    // observed.
    constexpr int spinIterations = 64;

    for (int i = 0; i < spinIterations; ++i)
    {
        if (__atomic_load_n(addr, __ATOMIC_ACQUIRE) != expected)
            return;

        ZA_CPU_RELAX();
    }

    while (__atomic_load_n(addr, __ATOMIC_ACQUIRE) == expected)
        sched_yield();
}

#endif


////////////////////////////////////////////////////////////
/// \brief Native blocking call on a 32-bit address
///
/// Returns when (a) the value at `addr` differs from `expected`,
/// (b) someone calls `platformWake` on `addr`, or (c) on Linux
/// every 2 seconds (defensive timeout -- see comment below).
///
////////////////////////////////////////////////////////////
void platformWait32(const za::U32* const addr, const za::U32 expected) noexcept
{
#if defined(ZA_SYSTEM_LINUX) || defined(ZA_SYSTEM_ANDROID)
    // 2-second timeout copied from libc++. The kernel cancels the
    // timer on a normal wake, so the happy path pays nothing extra.
    // The defensive value here is to recover from theoretically
    // possible missed-wake hazards (sandboxed runtimes, mismatched
    // FUTEX_PRIVATE flags, kernel ABI bugs, etc.) -- if such a thing
    // ever fires, the waiter loops back through the predicate check
    // and either makes forward progress or re-enters wait.
    const struct timespec timeout{2, 0};

    syscall(SYS_futex,
            const_cast<za::U32*>(addr),
            FUTEX_WAIT_PRIVATE,
            static_cast<int>(expected),
            &timeout,
            /* uaddr2 */ nullptr,
            /* val3   */ 0);

#elif defined(ZA_SYSTEM_WINDOWS)
    za::U32 compare = expected;
    WaitOnAddress(const_cast<za::U32*>(addr), &compare, sizeof(compare), INFINITE);

#elif defined(ZA_SYSTEM_EMSCRIPTEN)
    emscripten_atomic_wait_u32(const_cast<za::U32*>(addr), expected, /* maxWaitNanoseconds */ -1);

#else
    spinWait(addr, expected);
#endif
}


////////////////////////////////////////////////////////////
void platformWait64(const za::U64* const addr, const za::U64 expected) noexcept
{
#if defined(ZA_SYSTEM_LINUX) || defined(ZA_SYSTEM_ANDROID)
    // Linux futex is 32-bit only. On little-endian targets we wait on
    // the lower 32 bits of the storage; correctness is preserved
    // because:
    //   - if the lower 32 bits no longer match, futex returns immediately
    //   - notify wakes regardless of value
    // Spurious wakeups (caused by upper-32-bit changes) are absorbed
    // by the caller's predicate loop.
    static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "platformWait64 fallback assumes little-endian");

    const auto            lower = static_cast<za::U32>(expected & 0xFF'FF'FF'FFu);
    const struct timespec timeout{2, 0};

    syscall(SYS_futex,
            reinterpret_cast<int*>(const_cast<za::U64*>(addr)),
            FUTEX_WAIT_PRIVATE,
            static_cast<int>(lower),
            &timeout,
            /* uaddr2 */ nullptr,
            /* val3   */ 0);

#elif defined(ZA_SYSTEM_WINDOWS)
    za::U64 compare = expected;
    WaitOnAddress(const_cast<za::U64*>(addr), &compare, sizeof(compare), INFINITE);

#elif defined(ZA_SYSTEM_EMSCRIPTEN)
    emscripten_atomic_wait_u64(const_cast<za::U64*>(addr), expected, /* maxWaitNanoseconds */ -1);

#else
    spinWait(addr, expected);
#endif
}


////////////////////////////////////////////////////////////
void platformWake(const void* const addr, const bool wakeOne) noexcept
{
#if defined(ZA_SYSTEM_LINUX) || defined(ZA_SYSTEM_ANDROID)
    syscall(SYS_futex,
            const_cast<void*>(addr),
            FUTEX_WAKE_PRIVATE,
            /* val: number of waiters to wake */ wakeOne ? 1 : __INT_MAX__,
            /* timeout */ nullptr,
            /* uaddr2  */ nullptr,
            /* val3    */ 0);

#elif defined(ZA_SYSTEM_WINDOWS)
    if (wakeOne)
        WakeByAddressSingle(const_cast<void*>(addr));
    else
        WakeByAddressAll(const_cast<void*>(addr));

#elif defined(ZA_SYSTEM_EMSCRIPTEN)
    emscripten_atomic_notify(const_cast<void*>(addr), wakeOne ? 1 : __INT_MAX__);

#else
    // Spin-wait fallback wakes itself by re-loading; nothing to do.
    (void)addr;
    (void)wakeOne;
#endif
}
} // namespace


////////////////////////////////////////////////////////////
void atomicWait32(const za::U32* const addr, const za::U32 expected) noexcept
{
    auto& slot = slotFor(addr);

    // Register as a waiter BEFORE re-checking the value. The SeqCst
    // RMW pairs with the SeqCst load in `atomicNotifyOne`/`All`: at
    // least one of the two threads sees the other's effect, so we can
    // never simultaneously (a) miss a wake and (b) park.
    slot.waiters.fetchAddSeqCst(1u);

    // Re-check the value under the new ordering. If it has already
    // changed (e.g. the writer wrote between our caller's pre-check
    // and our increment), skip the syscall entirely.
    if (__atomic_load_n(addr, __ATOMIC_ACQUIRE) == expected)
        platformWait32(addr, expected);

    slot.waiters.fetchSubRelease(1u);
}


////////////////////////////////////////////////////////////
void atomicWait64(const za::U64* const addr, const za::U64 expected) noexcept
{
    auto& slot = slotFor(addr);

    slot.waiters.fetchAddSeqCst(1u);

    if (__atomic_load_n(addr, __ATOMIC_ACQUIRE) == expected)
        platformWait64(addr, expected);

    slot.waiters.fetchSubRelease(1u);
}


////////////////////////////////////////////////////////////
void atomicNotifyOne(const void* const addr) noexcept
{
    // Skip the syscall when no thread is currently parked on this
    // address. The SeqCst load pairs with the SeqCst RMW in
    // `atomicWait32`/`64`: if a waiter has already incremented its
    // slot counter, we are guaranteed to observe a non-zero value
    // here; if we observe zero, the waiter (if any) has not yet
    // committed to parking and will see our value update through
    // its own re-check.
    //
    // NOTE: matches libstdc++/libc++ exactly -- no SC fence before
    // the load. The SC totality on `waiters` does not by itself force
    // visibility of a prior non-SC store on the user's value atomic,
    // so callers that pair `notify*` with a non-SC store (e.g. a plain
    // `storeRelease`) have a theoretical missed-wake race. The stdlibs
    // tacitly assume users pair with SC stores; users of this code must
    // do the same. To close the hole, insert
    // `__atomic_thread_fence(__ATOMIC_SEQ_CST)` immediately before the
    // load below.
    if (slotFor(addr).waiters.loadSeqCst() == 0u)
        return;

    platformWake(addr, /* wakeOne */ true);
}


////////////////////////////////////////////////////////////
void atomicNotifyAll(const void* const addr) noexcept
{
    // See `atomicNotifyOne` for the missed-wake caveat.
    if (slotFor(addr).waiters.loadSeqCst() == 0u)
        return;

    platformWake(addr, /* wakeOne */ false);
}

} // namespace za::priv
