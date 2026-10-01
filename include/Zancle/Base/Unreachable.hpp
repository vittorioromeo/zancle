#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/HasBuiltin.hpp"

#include "Zancle/Config.hpp" // IWYU pragma: keep


#ifdef ZA_DEBUG

    #include "Zancle/Base/Assert.hpp" // IWYU pragma: keep

    ////////////////////////////////////////////////////////////
    #define ZA_UNREACHABLE() ::za::priv::assertFailure("false /* ZA_UNREACHABLE() */", __FILE__, __LINE__)

#elif ZA_HAS_BUILTIN(__builtin_unreachable)

    ////////////////////////////////////////////////////////////
    #define ZA_UNREACHABLE() __builtin_unreachable()

#elif ZA_HAS_BUILTIN(__assume)

    ////////////////////////////////////////////////////////////
    #define ZA_UNREACHABLE() __assume(false)

#else

    #include "Zancle/Base/Abort.hpp"

    ////////////////////////////////////////////////////////////
    #define ZA_UNREACHABLE() ::za::abort()

#endif


////////////////////////////////////////////////////////////
/// \file
///
/// \brief Portable "unreachable" hint
///
/// Marks a code path as unreachable so that the optimizer can omit
/// any safety checks leading to it.
///
/// In debug mode (`ZA_DEBUG`), reaching it is reported as an assertion
/// failure (with a stack trace, if enabled) and aborts the program, as
/// `__builtin_unreachable()` would otherwise silently fall through into
/// unrelated code in unoptimized builds.
///
/// In release mode, reaching it is undefined behavior. On compilers that
/// lack a dedicated builtin, it falls back to `za::abort()`.
///
////////////////////////////////////////////////////////////
