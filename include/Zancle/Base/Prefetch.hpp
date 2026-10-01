#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/HasBuiltin.hpp"


#if ZA_HAS_BUILTIN(__builtin_prefetch)

    ////////////////////////////////////////////////////////////
    #define ZA_PREFETCH(ptr, rw, locality) ((void)__builtin_prefetch(ptr, rw, locality))

#else

    ////////////////////////////////////////////////////////////
    #define ZA_PREFETCH(ptr, rw, locality) ((void)0)

#endif


////////////////////////////////////////////////////////////
/// \file
///
/// \brief Portable wrapper for `__builtin_prefetch`
///
/// Hints to the CPU that `ptr` will soon be accessed, so the cache
/// line containing it can be brought closer (typically into L1 or L2)
/// ahead of the actual load/store. Used to hide memory latency in
/// pointer-chasing loops where the next address is known one or more
/// iterations in advance.
///
/// Parameters (both `rw` and `locality` must be compile-time constants):
///   - `ptr`:      address to prefetch; need not be a valid object pointer
///   - `rw`:       0 = prepare for read, 1 = prepare for write
///   - `locality`: temporal locality hint, 0..3
///                 0 = no temporal locality (stream, evict after use)
///                 1 = low, keep in L3
///                 2 = moderate, keep in L2
///                 3 = high, keep in L1
///
/// Expands to `__builtin_prefetch` on compilers that support it, and
/// to a no-op elsewhere. Prefetch is purely a hint: incorrect `rw` or
/// `locality` values cannot cause undefined behavior, only suboptimal
/// cache usage. The fallback discards `ptr` entirely, so callers must
/// not rely on side effects in the argument expression.
///
////////////////////////////////////////////////////////////
