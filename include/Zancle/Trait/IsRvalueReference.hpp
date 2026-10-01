#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/HasBuiltin.hpp"


#if ZA_HAS_BUILTIN(__is_rvalue_reference)

    ////////////////////////////////////////////////////////////
    #define ZA_IS_RVALUE_REFERENCE(...) __is_rvalue_reference(__VA_ARGS__)

#else

namespace za::priv
{
////////////////////////////////////////////////////////////
template <typename T>
inline constexpr bool isRvalueReferenceImpl = false;


////////////////////////////////////////////////////////////
template <typename T>
inline constexpr bool isRvalueReferenceImpl<T&&> = true;

} // namespace za::priv

    ////////////////////////////////////////////////////////////
    #define ZA_IS_RVALUE_REFERENCE(...) ::za::priv::isRvalueReferenceImpl<__VA_ARGS__>

#endif


namespace za
{
////////////////////////////////////////////////////////////
template <typename T>
inline constexpr bool isRvalueReference = ZA_IS_RVALUE_REFERENCE(T);

} // namespace za
