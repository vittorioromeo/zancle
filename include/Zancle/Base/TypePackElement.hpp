#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/HasBuiltin.hpp"

#include "Zancle/Base/SizeT.hpp"


#if ZA_HAS_BUILTIN(__type_pack_element)

    ////////////////////////////////////////////////////////////
    #define ZA_TYPE_PACK_ELEMENT(N, ...) __type_pack_element<N, __VA_ARGS__>

namespace za
{
////////////////////////////////////////////////////////////
template <SizeT N, typename... Ts>
using TypePackElement = ZA_TYPE_PACK_ELEMENT(N, Ts...);

} // namespace za

#else

namespace za::priv
{
////////////////////////////////////////////////////////////
template <typename T>
struct TypeWrapper
{
    using type = T;
};


////////////////////////////////////////////////////////////
template <SizeT N,
          typename T0 = void,
          typename T1 = void,
          typename T2 = void,
          typename T3 = void,
          typename T4 = void,
          typename T5 = void,
          typename T6 = void,
          typename T7 = void,
          typename T8 = void,
          typename T9 = void,
          typename... Ts>
[[nodiscard]] consteval auto typePackElementImpl() noexcept
{
    // clang-format off
    if constexpr(N == 0)      { return TypeWrapper<T0>{}; }
    else if constexpr(N == 1) { return TypeWrapper<T1>{}; }
    else if constexpr(N == 2) { return TypeWrapper<T2>{}; }
    else if constexpr(N == 3) { return TypeWrapper<T3>{}; }
    else if constexpr(N == 4) { return TypeWrapper<T4>{}; }
    else if constexpr(N == 5) { return TypeWrapper<T5>{}; }
    else if constexpr(N == 6) { return TypeWrapper<T6>{}; }
    else if constexpr(N == 7) { return TypeWrapper<T7>{}; }
    else if constexpr(N == 8) { return TypeWrapper<T8>{}; }
    else if constexpr(N == 9) { return TypeWrapper<T9>{}; }
    else                      { return typePackElementImpl<N - 10, Ts...>(); }
    // clang-format on
}

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
template <SizeT N, typename... Ts>
    requires(N < sizeof...(Ts)) // otherwise `typePackElementImpl` would silently yield `void`
using TypePackElement = typename decltype(priv::typePackElementImpl<N, Ts...>())::type;


    ////////////////////////////////////////////////////////////
    #define ZA_TYPE_PACK_ELEMENT(N, ...) ::za::TypePackElement<N, __VA_ARGS__>

} // namespace za

#endif


////////////////////////////////////////////////////////////
/// \file
///
/// \brief Compile-time access to the `N`-th type in a parameter pack
///
/// `TypePackElement<N, Ts...>` evaluates to the `N`-th type in `Ts...`.
/// Prefers the `__type_pack_element` compiler builtin (Clang/MSVC),
/// otherwise falls back to a manually unrolled `if constexpr`
/// implementation that recurses ten elements at a time.
///
////////////////////////////////////////////////////////////
