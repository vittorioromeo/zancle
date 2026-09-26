// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Config.hpp" // IWYU pragma: keep

#include "Zancle/Base/StackTrace.hpp"

#include <cstdio>

#if !defined(ZA_SYSTEM_EMSCRIPTEN) && defined(__EXCEPTIONS)
    #define ZA_OPTIONAL_USE_EXCEPTIONS
#endif

#ifndef ZA_OPTIONAL_USE_EXCEPTIONS
    #include "Zancle/Base/Abort.hpp"
#endif


namespace za::priv
{
////////////////////////////////////////////////////////////
void onBadOptionalAccess()
{
    std::fflush(stdout);
    std::fputs("\n[[ZANCLE OPTIONAL FAILURE]]: not engaged!\n", stderr);
    printStackTrace();

#ifdef ZA_OPTIONAL_USE_EXCEPTIONS
    throw BadOptionalAccess{};
#else
    za::abort();
#endif
}

} // namespace za::priv
