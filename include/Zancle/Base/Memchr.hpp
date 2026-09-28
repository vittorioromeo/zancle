#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


#if __has_builtin(__builtin_memchr)

    ////////////////////////////////////////////////////////////
    #define ZA_MEMCHR __builtin_memchr

#else

    #include <cstring>

    ////////////////////////////////////////////////////////////
    #define ZA_MEMCHR ::std::memchr

#endif
