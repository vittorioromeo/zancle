#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


#if __has_builtin(__is_member_pointer)

    ////////////////////////////////////////////////////////////
    #define ZA_IS_MEMBER_POINTER(...) __is_member_pointer(__VA_ARGS__)

#else

namespace za::priv
{
////////////////////////////////////////////////////////////
template <typename>
inline constexpr bool isMemberPointerImpl = false;

////////////////////////////////////////////////////////////
// clang-format off
template <typename T, typename C> inline constexpr bool isMemberPointerImpl<T C::*>                = true;
template <typename T, typename C> inline constexpr bool isMemberPointerImpl<T C::* const>          = true;
template <typename T, typename C> inline constexpr bool isMemberPointerImpl<T C::* volatile>       = true;
template <typename T, typename C> inline constexpr bool isMemberPointerImpl<T C::* const volatile> = true;
// clang-format on

} // namespace za::priv

    ////////////////////////////////////////////////////////////
    #define ZA_IS_MEMBER_POINTER(...) ::za::priv::isMemberPointerImpl<__VA_ARGS__>

#endif


namespace za
{
////////////////////////////////////////////////////////////
template <typename T>
inline constexpr bool isMemberPointer = ZA_IS_MEMBER_POINTER(T);

} // namespace za
