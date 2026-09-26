#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Small fork of Boost.PFR that does not depend on the Standard Libary.
// Also uses ideas from ZXShady's lahzam: https://github.com/ZXShady/lahzam
//
// Boost.PFR license:
/*
   Copyright (c) 2016-2025 Antony Polukhin

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
//
// Lazham license:
/*
MIT License

Copyright (c) 2025 ZXShady

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
////////////////////////////////////////////////////////////

#include "Zancle/Base/IndexSequence.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/MakeIndexSequence.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/TypePackElement.hpp"

#include "Zancle/Trait/DeclVal.hpp"
#include "Zancle/Trait/RemoveCVRef.hpp"
#include "Zancle/Trait/RemoveReference.hpp"


namespace za::rfl::priv
{
////////////////////////////////////////////////////////////
template <SizeT N, typename T>
struct TupleMember
{
    ////////////////////////////////////////////////////////////
    [[no_unique_address]] T value;
};


////////////////////////////////////////////////////////////
template <typename IdxSeq, typename... Ts>
struct TupleBase;


////////////////////////////////////////////////////////////
template <SizeT... Is, typename... Ts>
struct TupleBase<IndexSequence<Is...>, Ts...> : TupleMember<Is, Ts>...
{
    ////////////////////////////////////////////////////////////
    enum : SizeT
    {
        memberCount = sizeof...(Ts)
    };


    ////////////////////////////////////////////////////////////
    constexpr TupleBase() = default;


    ////////////////////////////////////////////////////////////
    constexpr TupleBase(const TupleBase&) = default;
    constexpr TupleBase(TupleBase&&)      = default;


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr TupleBase(Ts... v) : TupleMember<Is, Ts>{v}...
    {
    }


    ////////////////////////////////////////////////////////////
    template <SizeT I>
    [[gnu::always_inline]] constexpr auto& get()
    {
        return static_cast<TupleMember<I, ZA_TYPE_PACK_ELEMENT(I, Ts...)>&>(*this).value;
    }


    ////////////////////////////////////////////////////////////
    template <SizeT I>
    [[nodiscard, gnu::always_inline]] constexpr const auto& get() const
    {
        return static_cast<const TupleMember<I, ZA_TYPE_PACK_ELEMENT(I, Ts...)>&>(*this).value;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void forEach(auto&& f)
    {
        (..., f(static_cast<TupleMember<Is, Ts>&>(*this).value));
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void forEach(auto&& f) const
    {
        (..., f(static_cast<const TupleMember<Is, Ts>&>(*this).value));
    }
};


////////////////////////////////////////////////////////////
template <>
struct TupleBase<IndexSequence<>>
{
    ////////////////////////////////////////////////////////////
    enum : SizeT
    {
        memberCount = 0u
    };


    ////////////////////////////////////////////////////////////
    constexpr TupleBase() = default;
};


////////////////////////////////////////////////////////////
template <typename... Ts>
struct Tuple : TupleBase<ZA_INDEX_SEQUENCE_FOR(Ts), Ts...>
{
    ////////////////////////////////////////////////////////////
    using TupleBase<ZA_INDEX_SEQUENCE_FOR(Ts), Ts...>::TupleBase;
};


////////////////////////////////////////////////////////////
struct Anything
{
    ////////////////////////////////////////////////////////////
    template <typename T>
    [[nodiscard]] consteval operator T&() const;
};


////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] consteval SizeT countFields()
{
    Anything x;

    // NOLINTBEGIN(readability-misleading-indentation)

    // clang-format off
         if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x}; }) { return 64u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};   }) { return 63u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};     }) { return 62u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};       }) { return 61u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};         }) { return 60u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};           }) { return 59u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};             }) { return 58u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};               }) { return 57u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                 }) { return 56u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                   }) { return 55u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                     }) { return 54u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                       }) { return 53u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                         }) { return 52u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                           }) { return 51u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                             }) { return 50u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                               }) { return 49u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                 }) { return 48u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                   }) { return 47u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                     }) { return 46u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                       }) { return 45u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                         }) { return 44u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                           }) { return 43u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                             }) { return 42u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                               }) { return 41u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                 }) { return 40u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                   }) { return 39u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                     }) { return 38u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                       }) { return 37u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                         }) { return 36u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                           }) { return 35u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                             }) { return 34u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                               }) { return 33u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                 }) { return 32u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                   }) { return 31u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                     }) { return 30u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                       }) { return 29u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                         }) { return 28u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                           }) { return 27u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                             }) { return 26u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                               }) { return 25u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                                 }) { return 24u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                                   }) { return 23u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                                     }) { return 22u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                                       }) { return 21u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                                         }) { return 20u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                                           }) { return 19u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                                             }) { return 18u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                                               }) { return 17u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                                                 }) { return 16u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                                                   }) { return 15u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                                                     }) { return 14u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x,x};                                                                                                       }) { return 13u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x,x};                                                                                                         }) { return 12u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x,x};                                                                                                           }) { return 11u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x,x};                                                                                                             }) { return 10u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x,x};                                                                                                               }) { return 9u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x,x};                                                                                                                 }) { return 8u; }
    else if constexpr (requires { T{x,x,x,x,x,x,x};                                                                                                                   }) { return 7u; }
    else if constexpr (requires { T{x,x,x,x,x,x};                                                                                                                     }) { return 6u; }
    else if constexpr (requires { T{x,x,x,x,x};                                                                                                                       }) { return 5u; }
    else if constexpr (requires { T{x,x,x,x};                                                                                                                         }) { return 4u; }
    else if constexpr (requires { T{x,x,x};                                                                                                                           }) { return 3u; }
    else if constexpr (requires { T{x,x};                                                                                                                             }) { return 2u; }
    else if constexpr (requires { T{x};                                                                                                                               }) { return 1u; }
    else if constexpr (requires { T{};                                                                                                                                }) { return 0u; }
    // clang-format on
    else
    {
        static_assert(sizeof(T) == 0, "Type is not aggregate initializable or has more than 64 fields.");
    }

    // NOLINTEND(readability-misleading-indentation)
}


////////////////////////////////////////////////////////////
template <SizeT I, typename T>
[[gnu::always_inline]] constexpr T& getImpl(TupleMember<I, T>& t)
{
    return t.value;
}


////////////////////////////////////////////////////////////
template <SizeT I, typename T>
[[gnu::always_inline]] constexpr T&& getImpl(TupleMember<I, T>&& t)
{
    return t.value;
}


////////////////////////////////////////////////////////////
template <SizeT I, typename T>
[[gnu::always_inline]] constexpr const T& getImpl(const TupleMember<I, T>& t)
{
    return t.value;
}


////////////////////////////////////////////////////////////
#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wundefined-internal"
    #pragma clang diagnostic ignored "-Wundefined-var-template"
#endif


////////////////////////////////////////////////////////////
template <typename T>
struct FakeObjectWrapper
{
    const T value;
};


////////////////////////////////////////////////////////////
template <typename T>
extern const FakeObjectWrapper<T> fakeObjectImpl;


////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] consteval const T& getFakeObject() noexcept
{
    return fakeObjectImpl<T>.value;
}


////////////////////////////////////////////////////////////
#ifdef __clang__
    #pragma clang diagnostic pop
#endif


////////////////////////////////////////////////////////////
template <typename... Ts>
[[gnu::always_inline]] constexpr auto makeRefTuple(Ts&... args)
{
    return Tuple<Ts&...>{args...};
}

} // namespace za::rfl::priv


namespace za::rfl
{
////////////////////////////////////////////////////////////
template <typename T>
inline constexpr SizeT numFields = priv::countFields<T>();


////////////////////////////////////////////////////////////
template <typename T>
constexpr auto tieAsTuple(T&& obj)
{
    enum : SizeT
    {
        fieldCount = priv::countFields<ZA_REMOVE_CVREF(T)>()
    };

#define ZA_PRIV_RFL_RETURN_TIE_AS_TUPLE(N, ...) \
    auto& [__VA_ARGS__] = obj;                  \
    return priv::makeRefTuple(__VA_ARGS__)

    // clang-format off
#define ZA_PRIV_RFL_BRANCH(N, ...) \
    constexpr (fieldCount == N) { ZA_PRIV_RFL_RETURN_TIE_AS_TUPLE(N, __VA_ARGS__); }
    // clang-format on

    // NOLINTBEGIN(readability-misleading-indentation)

    // clang-format off
         if constexpr (fieldCount == 0)  { return priv::Tuple<>{}; }
    else if ZA_PRIV_RFL_BRANCH(1,  a)
    else if ZA_PRIV_RFL_BRANCH(2,  a,b)
    else if ZA_PRIV_RFL_BRANCH(3,  a,b,c)
    else if ZA_PRIV_RFL_BRANCH(4,  a,b,c,d)
    else if ZA_PRIV_RFL_BRANCH(5,  a,b,c,d,e)
    else if ZA_PRIV_RFL_BRANCH(6,  a,b,c,d,e,f)
    else if ZA_PRIV_RFL_BRANCH(7,  a,b,c,d,e,f,g)
    else if ZA_PRIV_RFL_BRANCH(8,  a,b,c,d,e,f,g,h)
    else if ZA_PRIV_RFL_BRANCH(9,  a,b,c,d,e,f,g,h,i)
    else if ZA_PRIV_RFL_BRANCH(10, a,b,c,d,e,f,g,h,i,j)
    else if ZA_PRIV_RFL_BRANCH(11, a,b,c,d,e,f,g,h,i,j,k)
    else if ZA_PRIV_RFL_BRANCH(12, a,b,c,d,e,f,g,h,i,j,k,l)
    else if ZA_PRIV_RFL_BRANCH(13, a,b,c,d,e,f,g,h,i,j,k,l,m)
    else if ZA_PRIV_RFL_BRANCH(14, a,b,c,d,e,f,g,h,i,j,k,l,m,n)
    else if ZA_PRIV_RFL_BRANCH(15, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)
    else if ZA_PRIV_RFL_BRANCH(16, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)
    else if ZA_PRIV_RFL_BRANCH(17, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)
    else if ZA_PRIV_RFL_BRANCH(18, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)
    else if ZA_PRIV_RFL_BRANCH(19, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s)
    else if ZA_PRIV_RFL_BRANCH(20, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)
    else if ZA_PRIV_RFL_BRANCH(21, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u)
    else if ZA_PRIV_RFL_BRANCH(22, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v)
    else if ZA_PRIV_RFL_BRANCH(23, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w)
    else if ZA_PRIV_RFL_BRANCH(24, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x)
    else if ZA_PRIV_RFL_BRANCH(25, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y)
    else if ZA_PRIV_RFL_BRANCH(26, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z)
    else if ZA_PRIV_RFL_BRANCH(27, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A)
    else if ZA_PRIV_RFL_BRANCH(28, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B)
    else if ZA_PRIV_RFL_BRANCH(29, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C)
    else if ZA_PRIV_RFL_BRANCH(30, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D)
    else if ZA_PRIV_RFL_BRANCH(31, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E)
    else if ZA_PRIV_RFL_BRANCH(32, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F)
    else if ZA_PRIV_RFL_BRANCH(33, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G)
    else if ZA_PRIV_RFL_BRANCH(34, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H)
    else if ZA_PRIV_RFL_BRANCH(35, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J)
    else if ZA_PRIV_RFL_BRANCH(36, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K)
    else if ZA_PRIV_RFL_BRANCH(37, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L)
    else if ZA_PRIV_RFL_BRANCH(38, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M)
    else if ZA_PRIV_RFL_BRANCH(39, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N)
    else if ZA_PRIV_RFL_BRANCH(40, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O)
    else if ZA_PRIV_RFL_BRANCH(41, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P)
    else if ZA_PRIV_RFL_BRANCH(42, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q)
    else if ZA_PRIV_RFL_BRANCH(43, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R)
    else if ZA_PRIV_RFL_BRANCH(44, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S)
    else if ZA_PRIV_RFL_BRANCH(45, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U)
    else if ZA_PRIV_RFL_BRANCH(46, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V)
    else if ZA_PRIV_RFL_BRANCH(47, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W)
    else if ZA_PRIV_RFL_BRANCH(48, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X)
    else if ZA_PRIV_RFL_BRANCH(49, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y)
    else if ZA_PRIV_RFL_BRANCH(50, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z)
    else if ZA_PRIV_RFL_BRANCH(51, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa)
    else if ZA_PRIV_RFL_BRANCH(52, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb)
    else if ZA_PRIV_RFL_BRANCH(53, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb,cc)
    else if ZA_PRIV_RFL_BRANCH(54, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb,cc,dd)
    else if ZA_PRIV_RFL_BRANCH(55, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb,cc,dd,ee)
    else if ZA_PRIV_RFL_BRANCH(56, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb,cc,dd,ee,ff)
    else if ZA_PRIV_RFL_BRANCH(57, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb,cc,dd,ee,ff,gg)
    else if ZA_PRIV_RFL_BRANCH(58, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb,cc,dd,ee,ff,gg,hh)
    else if ZA_PRIV_RFL_BRANCH(59, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb,cc,dd,ee,ff,gg,hh,ii)
    else if ZA_PRIV_RFL_BRANCH(60, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb,cc,dd,ee,ff,gg,hh,ii,jj)
    else if ZA_PRIV_RFL_BRANCH(61, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb,cc,dd,ee,ff,gg,hh,ii,jj,kk)
    else if ZA_PRIV_RFL_BRANCH(62, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb,cc,dd,ee,ff,gg,hh,ii,jj,kk,ll)
    else if ZA_PRIV_RFL_BRANCH(63, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb,cc,dd,ee,ff,gg,hh,ii,jj,kk,ll,mm)
    else if ZA_PRIV_RFL_BRANCH(64, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P,Q,R,S,U,V,W,X,Y,Z,aa,bb,cc,dd,ee,ff,gg,hh,ii,jj,kk,ll,mm,nn)
            // clang-format on
            else
        {
            static_assert(sizeof(T) == 0, "`tieAsTuple` supports up to 64 fields.");
        }

    // NOLINTEND(readability-misleading-indentation)

#undef ZA_PRIV_RFL_BRANCH
#undef ZA_PRIV_RFL_RETURN_TIE_AS_TUPLE
}


////////////////////////////////////////////////////////////
template <SizeT I>
[[gnu::always_inline]] constexpr auto&& getField(auto&& obj)
{
    return priv::getImpl<I>(tieAsTuple(ZA_FORWARD(obj)));
}


////////////////////////////////////////////////////////////
template <SizeT I, typename T>
using FieldType = ZA_REMOVE_REFERENCE(decltype(getField<I>(declVal<T&>())));


////////////////////////////////////////////////////////////
[[gnu::always_inline]] constexpr void forEachField(auto&& obj, auto&& f)
{
    tieAsTuple(ZA_FORWARD(obj)).forEach(ZA_FORWARD(f));
}

} // namespace za::rfl


////////////////////////////////////////////////////////////
/// \file
///
/// \brief Compile-time reflection for aggregate types (core)
///
/// `za::rfl` is a small fork of Boost.PFR that does not depend on the
/// C++ standard library. It supports aggregate types of up to 64
/// fields. This header carries the structural core:
///
/// - `numFields<T>` -- number of public data members
/// - `tieAsTuple(obj)` -- bind every field of `obj` into a tuple of refs
/// - `getField<I>(obj)` -- fetch the `I`-th field by index
/// - `forEachField(obj, fn)` -- invoke `fn` on each field in declaration order
///
/// Unions and C-style array types are explicitly rejected by the
/// public reflection entry points.
///
/// For compile-time **field-name** extraction, include
/// `Zancle/Reflection/RflNames.hpp` instead. Field names live in a
/// separate header because they pull in `String/StringView` and
/// `Container/Array`, which most users of structural reflection do not
/// need.
///
////////////////////////////////////////////////////////////
