#include "CatNames.hpp"
#include "CatType.hpp"

#include "Zancle/String/StringView.hpp"

#include "Zancle/Algorithm/Shuffle.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
[[nodiscard]] za::Vector<za::StringView> getShuffledCatNames(const CatType catType, za::FastNonCryptoRng& rng)
{
    const auto span = catNamesPerType[asIdx(catType)];

    za::Vector<za::StringView> names;
    names.reserve(span.size());

    for (const char* str : span)
        names.emplaceBack(str);

    za::shuffle(names.begin(), names.end(), [&](const za::SizeT min, const za::SizeT max) {
        return rng.getI<za::SizeT>(min, max);
    });

    return names;
}
