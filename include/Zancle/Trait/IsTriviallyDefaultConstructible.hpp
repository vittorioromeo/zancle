#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


#if __has_builtin(__is_trivially_constructible)

    ////////////////////////////////////////////////////////////
    #define ZA_IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE(...) __is_trivially_constructible(__VA_ARGS__)

#else

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
    #include <type_traits>


    ////////////////////////////////////////////////////////////
    #define ZA_IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE(...) ::std::is_trivially_default_constructible_v<__VA_ARGS__>

#endif


namespace za
{
////////////////////////////////////////////////////////////
template <typename T>
inline constexpr bool isTriviallyDefaultConstructible = ZA_IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE(T);

} // namespace za
