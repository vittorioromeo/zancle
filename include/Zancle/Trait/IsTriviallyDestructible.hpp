#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


#if __has_builtin(__is_trivially_destructible)

    ////////////////////////////////////////////////////////////
    #define ZA_IS_TRIVIALLY_DESTRUCTIBLE(...) __is_trivially_destructible(__VA_ARGS__)

#else

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
    #include <type_traits>


    ////////////////////////////////////////////////////////////
    #define ZA_IS_TRIVIALLY_DESTRUCTIBLE(...) ::std::is_trivially_destructible_v<__VA_ARGS__>

#endif


namespace za
{
////////////////////////////////////////////////////////////
template <typename T>
inline constexpr bool isTriviallyDestructible = ZA_IS_TRIVIALLY_DESTRUCTIBLE(T);

} // namespace za
