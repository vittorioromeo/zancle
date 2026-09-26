#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Combine multiple callables into a single overloaded callable
///
/// Standard "overloaded" idiom: inherits from each `Fs` and pulls in
/// their `operator()` so that the resulting object exposes one
/// overload per base. Typically used to write pattern-matching style
/// visitors for `Variant`.
///
/// Example:
/// \code
/// variant.recursiveMatch(
///     [](int i)         { ... },
///     [](const String&) { ... });
/// \endcode
///
////////////////////////////////////////////////////////////
template <typename... Fs>
struct [[nodiscard]] OverloadSet : Fs...
{
    // Intentionally an aggregate: no constructor to compile, nor to call in debug mode
    using Fs::operator()...;
};


////////////////////////////////////////////////////////////
/// \brief Deduction guide allowing brace-init from a list of callables
///
////////////////////////////////////////////////////////////
template <typename... Fs>
OverloadSet(Fs...) -> OverloadSet<Fs...>;

} // namespace za
