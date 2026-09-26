#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/IndexSequence.hpp"
#include "Zancle/Base/SizeT.hpp"


#if __has_builtin(__integer_pack)

    ////////////////////////////////////////////////////////////
    #define ZA_MAKE_INDEX_SEQUENCE(N) ::za::IndexSequence<__integer_pack(N)...>

namespace za
{
////////////////////////////////////////////////////////////
template <SizeT N>
using MakeIndexSequence = ZA_MAKE_INDEX_SEQUENCE(N);

} // namespace za

#elif __has_builtin(__make_integer_seq)

namespace za::priv
{
////////////////////////////////////////////////////////////
// Alias (rather than a class with a nested `type`) avoids one class
// template instantiation per distinct `N`
template <typename T, T... Is>
using MakeIndexSequenceHelper = IndexSequence<Is...>;

} // namespace za::priv

namespace za
{
////////////////////////////////////////////////////////////
template <SizeT N>
using MakeIndexSequence = __make_integer_seq<priv::MakeIndexSequenceHelper, SizeT, N>;

} // namespace za

    ////////////////////////////////////////////////////////////
    #define ZA_MAKE_INDEX_SEQUENCE(N) ::za::MakeIndexSequence<N>

#else

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
    #include <utility>


namespace za::priv
{
////////////////////////////////////////////////////////////
// Converts `std::index_sequence` to `za::IndexSequence` (declaration only)
template <SizeT... Is>
IndexSequence<Is...> toIndexSequence(std::index_sequence<Is...>);

} // namespace za::priv

namespace za
{
////////////////////////////////////////////////////////////
template <SizeT N>
using MakeIndexSequence = decltype(priv::toIndexSequence(std::make_index_sequence<N>{}));

} // namespace za

    ////////////////////////////////////////////////////////////
    #define ZA_MAKE_INDEX_SEQUENCE(N) ::za::MakeIndexSequence<N>

#endif


////////////////////////////////////////////////////////////
/// \brief Macro form of `std::index_sequence_for`: produces an index sequence matching a parameter pack
///
////////////////////////////////////////////////////////////
#define ZA_INDEX_SEQUENCE_FOR(...) ZA_MAKE_INDEX_SEQUENCE(sizeof...(__VA_ARGS__))


////////////////////////////////////////////////////////////
/// \file
///
/// \brief `std::make_index_sequence<N>` replacement
///
/// `MakeIndexSequence<N>` evaluates to `IndexSequence<0, 1, ..., N-1>`.
/// The implementation prefers compiler builtins (`__integer_pack` on
/// GCC, `__make_integer_seq` on Clang/MSVC) so that the alias does not
/// drag in `<utility>`. Falls back to `std::make_index_sequence` only
/// when no builtin is available.
///
////////////////////////////////////////////////////////////
