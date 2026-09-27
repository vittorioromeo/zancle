#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Unstable in-place erase of elements satisfying `predicate`; returns the number removed
///
////////////////////////////////////////////////////////////
template <typename Vector, typename Predicate>
[[gnu::always_inline]] inline constexpr SizeT vectorSwapAndPopIf(Vector& vector, Predicate&& predicate)
{
    const SizeT initialSize = vector.size();
    SizeT       currentSize = initialSize;

    for (SizeT i = currentSize; i-- > 0u;)
    {
        if (!predicate(vector[i]))
            continue;

        --currentSize;
        vector[i] = ZA_MOVE(vector[currentSize]);
    }

    vector.erase(vector.begin() + currentSize, vector.end());
    return initialSize - currentSize;
}

} // namespace za
