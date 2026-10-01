#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/HasBuiltin.hpp"


#if ZA_HAS_BUILTIN(__remove_const)

    ////////////////////////////////////////////////////////////
    #define ZA_REMOVE_CONST(...) __remove_const(__VA_ARGS__)

#else

namespace za::priv
{
////////////////////////////////////////////////////////////
// clang-format off
template <typename T> struct RemoveConstImpl          { using type = T; };
template <typename T> struct RemoveConstImpl<const T> { using type = T; };
// clang-format on

} // namespace za::priv

    ////////////////////////////////////////////////////////////
    #define ZA_REMOVE_CONST(...) typename ::za::priv::RemoveConstImpl<__VA_ARGS__>::type

#endif


namespace za
{
////////////////////////////////////////////////////////////
template <typename T>
using RemoveConst = ZA_REMOVE_CONST(T);

} // namespace za
