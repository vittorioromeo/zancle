#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


#if __has_builtin(__builtin_common_type)

namespace za::priv
{
////////////////////////////////////////////////////////////
template <typename T>
struct TypeIdentityImpl
{
    using type = T;
};


////////////////////////////////////////////////////////////
struct EmptyImpl
{
};


////////////////////////////////////////////////////////////
template <typename... Ts>
struct CommonTypeImpl;


////////////////////////////////////////////////////////////
template <typename... Ts>
using CommonType = typename CommonTypeImpl<Ts...>::type;


////////////////////////////////////////////////////////////
template <typename... Ts>
struct CommonTypeImpl : __builtin_common_type<CommonType, TypeIdentityImpl, EmptyImpl, Ts...>
{
};

} // namespace za::priv

    ////////////////////////////////////////////////////////////
    #define ZA_COMMON_TYPE(...) ::za::priv::CommonType<__VA_ARGS__>

#else

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
    #include "Zancle/Trait/Conditional.hpp"
    #include "Zancle/Trait/Decay.hpp"
    #include "Zancle/Trait/IsSame.hpp"
    #include "Zancle/Trait/RemoveCVRef.hpp"
    #include "Zancle/Trait/VoidT.hpp"

    #include "Zancle/Base/DeclVal.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
template <typename T, typename U>
using CondType = decltype(false ? declVal<T>() : declVal<U>());


////////////////////////////////////////////////////////////
template <typename T, typename U, typename = void>
struct CommonType3
{
};


////////////////////////////////////////////////////////////
template <typename T, typename U>
struct CommonType3<T, U, VoidT<CondType<const T&, const U&>>>
{
    using type = ZA_REMOVE_CVREF(CondType<const T&, const U&>);
};


////////////////////////////////////////////////////////////
template <typename T, typename U, typename = void>
struct CommonType2Impl : CommonType3<T, U>
{
};


////////////////////////////////////////////////////////////
template <typename T, typename U>
struct CommonType2Impl<T, U, VoidT<decltype(true ? declVal<T>() : declVal<U>())>>
{
    using type = ZA_DECAY(decltype(true ? declVal<T>() : declVal<U>()));
};


////////////////////////////////////////////////////////////
template <>
struct CommonType2Impl<void, void> // `declVal<void>()` is ill-formed
{
    using type = void;
};


////////////////////////////////////////////////////////////
template <typename, typename = void>
struct CommonTypeImpl
{
};


////////////////////////////////////////////////////////////
template <typename... T>
struct CommonTypes;


////////////////////////////////////////////////////////////
template <typename... T>
struct CommonType;


////////////////////////////////////////////////////////////
template <typename T, typename U>
struct CommonTypeImpl<CommonTypes<T, U>, VoidT<typename CommonType<T, U>::type>>
{
    using type = typename CommonType<T, U>::type;
};


////////////////////////////////////////////////////////////
template <typename T, typename U, typename V, typename... Rest>
struct CommonTypeImpl<CommonTypes<T, U, V, Rest...>, VoidT<typename CommonType<T, U>::type>> :
    CommonTypeImpl<CommonTypes<typename CommonType<T, U>::type, V, Rest...>>
{
};


////////////////////////////////////////////////////////////
template <>
struct CommonType<>
{
};


////////////////////////////////////////////////////////////
template <typename T>
struct CommonType<T> : public CommonType<T, T>
{
};


////////////////////////////////////////////////////////////
template <typename T, typename U>
struct CommonType<T, U> :
    Conditional<ZA_IS_SAME(T, ZA_DECAY(T)) && ZA_IS_SAME(U, ZA_DECAY(U)), CommonType2Impl<T, U>, CommonType<ZA_DECAY(T), ZA_DECAY(U)>>
{
};


////////////////////////////////////////////////////////////
template <typename T, typename U, typename V, typename... Rest>
struct CommonType<T, U, V, Rest...> : CommonTypeImpl<CommonTypes<T, U, V, Rest...>>
{
};

} // namespace za::priv

    ////////////////////////////////////////////////////////////
    #define ZA_COMMON_TYPE(...) typename ::za::priv::CommonType<__VA_ARGS__>::type

#endif


namespace za
{
////////////////////////////////////////////////////////////
template <typename... Ts>
using CommonType = ZA_COMMON_TYPE(Ts...);

} // namespace za
