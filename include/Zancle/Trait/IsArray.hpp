#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


#if __has_builtin(__is_array)

    ////////////////////////////////////////////////////////////
    #define ZA_IS_ARRAY(...) __is_array(__VA_ARGS__)

#else

namespace za::priv
{
////////////////////////////////////////////////////////////
// clang-format off
template <typename>              inline constexpr bool isArrayImpl          = false;
template <typename T, auto Size> inline constexpr bool isArrayImpl<T[Size]> = true;
template <typename T>            inline constexpr bool isArrayImpl<T[]>     = true;
// clang-format on

} // namespace za::priv

    ////////////////////////////////////////////////////////////
    #define ZA_IS_ARRAY(...) ::za::priv::isArrayImpl<__VA_ARGS__>

#endif


namespace za
{
////////////////////////////////////////////////////////////
template <typename T>
inline constexpr bool isArray = ZA_IS_ARRAY(T);

} // namespace za
