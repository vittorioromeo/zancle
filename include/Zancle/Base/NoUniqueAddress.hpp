#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
/// \brief Portable `[[no_unique_address]]` attribute
///
/// On the MSVC ABI (including clang-cl), the standard attribute is
/// accepted but silently ignored for ABI compatibility, so empty
/// members still occupy (and pad) storage. The vendor-specific
/// `[[msvc::no_unique_address]]` must be used there instead.
///
////////////////////////////////////////////////////////////
#ifdef _MSC_VER

    #define ZA_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]

#else

    #define ZA_NO_UNIQUE_ADDRESS [[no_unique_address]]

#endif
