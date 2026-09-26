#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/String/StringView.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsSame.hpp"


namespace za
{
////////////////////////////////////////////////////////////
template <typename Splitter, typename F>
[[gnu::always_inline]] constexpr void StringView::forSplitsImpl(Splitter splitter, F&& f) const
{
    SizeT segStart = 0u;

    while (segStart < theSize)
    {
        const SizeT splitPos = find(splitter, segStart);

        if (splitPos == nPos)
        {
            f(StringView{theData + segStart, theSize - segStart});
            return;
        }

        f(StringView{theData + segStart, splitPos - segStart});

        if constexpr (ZA_IS_SAME(Splitter, char))
            segStart = splitPos + 1u;
        else
            segStart = splitPos + splitter.theSize;
    }
}


////////////////////////////////////////////////////////////
template <typename F>
[[gnu::always_inline, gnu::flatten]] constexpr void StringView::forSplits(const StringView splitter, F&& f) const
{
    ZA_ASSERT(!splitter.empty() && "Splitter must be non-empty");
    forSplitsImpl(splitter, static_cast<F&&>(f));
}


////////////////////////////////////////////////////////////
template <typename F>
[[gnu::always_inline, gnu::flatten]] constexpr void StringView::forSplits(const char splitter, F&& f) const
{
    forSplitsImpl(splitter, static_cast<F&&>(f));
}


////////////////////////////////////////////////////////////
template <typename F>
[[gnu::always_inline, gnu::flatten]] constexpr void StringView::forLines(F&& f) const
{
    forSplits('\n', static_cast<F&&>(f));
}

} // namespace za
