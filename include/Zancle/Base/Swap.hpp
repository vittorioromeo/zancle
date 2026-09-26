#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/DeclVal.hpp"


namespace za::priv::swap_adl
{
////////////////////////////////////////////////////////////
// A deleted generic template. If ADL finds a generic `std::swap`, it will
// collide with this and cause an ambiguity.
template <typename T>
void swap(T&, T&) = delete;


////////////////////////////////////////////////////////////
using SizeT = decltype(sizeof(0));


////////////////////////////////////////////////////////////
// Whether swapping two `T` lvalues via `SwapFn` cannot throw. Mirrors the
// dispatch of `SwapFn::operator()` below, and is the single source of truth
// for both its `noexcept`-specifier and `za::isNoThrowSwappable`. Uses
// builtins only, as `Base` cannot depend on the `Trait` module.
template <typename T>
[[nodiscard]] consteval bool isNoThrowSwappableImpl() noexcept
{
    if constexpr (requires(T& a, T& b) { a.swap(b); })
        return noexcept(declVal<T&>().swap(declVal<T&>()));
    else if constexpr (requires(T& a, T& b) { swap(a, b); })
        return noexcept(swap(declVal<T&>(), declVal<T&>()));
    else // nested requirement: `false` rather than a hard error for e.g. `void`
        return requires { requires __is_nothrow_constructible(T, T&&) && __is_nothrow_assignable(T&, T&&); };
}


////////////////////////////////////////////////////////////
template <typename T>
inline constexpr bool isNoThrowSwappable = isNoThrowSwappableImpl<T>();


////////////////////////////////////////////////////////////
// Arrays are swapped element-wise (see `SwapFn`'s array overload)
template <typename T, SizeT N>
inline constexpr bool isNoThrowSwappable<T[N]> = isNoThrowSwappable<T>;


////////////////////////////////////////////////////////////
// Niebloid (Customization Point Object)
struct SwapFn
{
    ////////////////////////////////////////////////////////////
    template <typename T, SizeT N>
    [[gnu::always_inline]] static constexpr void operator()(T (&a)[N], T (&b)[N]) noexcept(isNoThrowSwappable<T>)
    {
        for (SizeT i = 0; i < N; ++i)
            operator()(a[i], b[i]);
    }


    ////////////////////////////////////////////////////////////
    template <typename T>
    [[gnu::always_inline]] static constexpr void operator()(T& a, T& b) noexcept(isNoThrowSwappable<T>)
    {
        if constexpr (requires { a.swap(b); })
        {
            // Highest priority: explicit member function exists
            a.swap(b);
        }
        else if constexpr (requires { swap(a, b); }) // Fails in case of ambiguity too
        {
            // A specialized ADL swap exists and is unambiguous
            swap(a, b);
        }
        else
        {
            // No valid specialized ADL swap was found, or an ambiguity occurred
            T tempA = static_cast<T&&>(a);
            a       = static_cast<T&&>(b);
            b       = static_cast<T&&>(tempA);
        }
    }
};

} // namespace za::priv::swap_adl


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Customization point object that swaps two values
///
/// Behaves like `std::ranges::swap`: prefers a member `swap`, then a
/// well-formed unqualified `swap` found via ADL, and finally falls
/// back to a manual three-step move-swap. Calling
/// `genericSwap(a, b)` is therefore the recommended way to swap
/// arbitrary objects in code that wants to honor user-provided
/// `swap` overloads without needing to write the customary
/// `using std::swap; swap(a, b);` dance.
///
/// It is `noexcept` exactly when the selected operation is (see
/// `za::isNoThrowSwappable`), so exceptions thrown by a user-provided
/// `swap` or by move operations propagate instead of terminating.
///
////////////////////////////////////////////////////////////
inline constexpr priv::swap_adl::SwapFn genericSwap{};


////////////////////////////////////////////////////////////
/// \brief Swap the elements pointed to by two iterators
///
////////////////////////////////////////////////////////////
template <typename ForwardIt1, typename ForwardIt2>
[[gnu::always_inline]] inline constexpr void iterSwap(const ForwardIt1 a, const ForwardIt2 b)
{
    genericSwap(*a, *b);
}


////////////////////////////////////////////////////////////
/// \brief Pairwise swap two ranges of equal length
///
/// \return Iterator to the element past the last element of the second range
///
////////////////////////////////////////////////////////////
template <typename ForwardIt1, typename ForwardIt2>
[[gnu::always_inline]] inline constexpr ForwardIt2 swapRanges(ForwardIt1 first1, const ForwardIt1 last1, ForwardIt2 first2)
{
    for (; first1 != last1; ++first1, ++first2)
        iterSwap(first1, first2);

    return first2;
}

} // namespace za
