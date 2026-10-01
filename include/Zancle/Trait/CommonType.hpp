#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/HasBuiltin.hpp"


#if ZA_HAS_BUILTIN(__builtin_common_type)

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
using CommonTypeT = typename CommonTypeImpl<Ts...>::type;


////////////////////////////////////////////////////////////
template <typename... Ts>
struct CommonTypeImpl : __builtin_common_type<CommonTypeT, TypeIdentityImpl, EmptyImpl, Ts...>
{
};

} // namespace za::priv

    ////////////////////////////////////////////////////////////
    #define ZA_COMMON_TYPE(...) ::za::priv::CommonTypeT<__VA_ARGS__>

#else

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
    #include "Zancle/Trait/Conditional.hpp"
    #include "Zancle/Trait/Decay.hpp"
    #include "Zancle/Trait/DeclVal.hpp"
    #include "Zancle/Trait/IsSame.hpp"
    #include "Zancle/Trait/RemoveCVRef.hpp"
    #include "Zancle/Trait/VoidT.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
template <typename T, typename U>
using CondType = decltype(false ? declVal<T>() : declVal<U>());


////////////////////////////////////////////////////////////
template <typename T, typename U, typename = void>
struct CommonType3Impl
{
};


////////////////////////////////////////////////////////////
template <typename T, typename U>
struct CommonType3Impl<T, U, VoidT<CondType<const T&, const U&>>>
{
    using type = ZA_REMOVE_CVREF(CondType<const T&, const U&>);
};


////////////////////////////////////////////////////////////
template <typename T, typename U, typename = void>
struct CommonType2Impl : CommonType3Impl<T, U>
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
struct CommonTypeFoldImpl
{
};


////////////////////////////////////////////////////////////
template <typename... T>
struct CommonTypes;


////////////////////////////////////////////////////////////
template <typename... T>
struct CommonTypeImpl;


////////////////////////////////////////////////////////////
template <typename T, typename U>
struct CommonTypeFoldImpl<CommonTypes<T, U>, VoidT<typename CommonTypeImpl<T, U>::type>>
{
    using type = typename CommonTypeImpl<T, U>::type;
};


////////////////////////////////////////////////////////////
template <typename T, typename U, typename V, typename... Rest>
struct CommonTypeFoldImpl<CommonTypes<T, U, V, Rest...>, VoidT<typename CommonTypeImpl<T, U>::type>> :
    CommonTypeFoldImpl<CommonTypes<typename CommonTypeImpl<T, U>::type, V, Rest...>>
{
};


////////////////////////////////////////////////////////////
template <>
struct CommonTypeImpl<>
{
};


////////////////////////////////////////////////////////////
template <typename T>
struct CommonTypeImpl<T> : public CommonTypeImpl<T, T>
{
};


////////////////////////////////////////////////////////////
template <typename T, typename U>
struct CommonTypeImpl<T, U> :
    Conditional<ZA_IS_SAME(T, ZA_DECAY(T)) && ZA_IS_SAME(U, ZA_DECAY(U)), CommonType2Impl<T, U>, CommonTypeImpl<ZA_DECAY(T), ZA_DECAY(U)>>
{
};


////////////////////////////////////////////////////////////
template <typename T, typename U, typename V, typename... Rest>
struct CommonTypeImpl<T, U, V, Rest...> : CommonTypeFoldImpl<CommonTypes<T, U, V, Rest...>>
{
};

} // namespace za::priv

    ////////////////////////////////////////////////////////////
    #define ZA_COMMON_TYPE(...) typename ::za::priv::CommonTypeImpl<__VA_ARGS__>::type

#endif


namespace za
{
////////////////////////////////////////////////////////////
template <typename... Ts>
using CommonType = ZA_COMMON_TYPE(Ts...);

} // namespace za
