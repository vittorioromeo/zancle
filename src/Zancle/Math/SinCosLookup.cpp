// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Math/SinCosLookup.hpp"

#include "Zancle/Base/IntTypes.hpp"


namespace za::priv
{
namespace
{
////////////////////////////////////////////////////////////
[[nodiscard]] consteval SinTable makeSinTable() noexcept
{
    SinTable table{};

    for (U32 i = 0u; i < sinTableSize; ++i)
        table.data[i] = sinTableEntry(i);

    return table;
}

} // namespace


////////////////////////////////////////////////////////////
constinit const SinTable sinTable = makeSinTable();

} // namespace za::priv
