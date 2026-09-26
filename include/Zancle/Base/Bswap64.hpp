#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


#if __has_builtin(__builtin_bswap64)

    ////////////////////////////////////////////////////////////
    #define ZA_BSWAP64 __builtin_bswap64

#else

    #include <bit>

    ////////////////////////////////////////////////////////////
    // Converts to `unsigned long long` first, as the `std` function is
    // type-generic while the builtin always operates on 64 bits
    #define ZA_BSWAP64(...) ::std::byteswap(static_cast<unsigned long long>(__VA_ARGS__))

#endif
