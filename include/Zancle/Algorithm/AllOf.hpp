#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


namespace za
{
////////////////////////////////////////////////////////////
/// \brief `true` if all elements in the range satisfy `predicate`
///
////////////////////////////////////////////////////////////
template <typename ForwardIt, typename Predicate>
[[nodiscard, gnu::always_inline]] constexpr bool allOf(ForwardIt rangeBegin, const ForwardIt rangeEnd, Predicate&& predicate)
{
    for (; rangeBegin != rangeEnd; ++rangeBegin)
        if (!predicate(*rangeBegin))
            return false;

    return true;
}

} // namespace za
