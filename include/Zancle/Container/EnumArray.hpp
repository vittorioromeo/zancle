#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/LifetimeAttributes.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/EnableTrivialRelocation.hpp"
#include "Zancle/Trait/IsEnum.hpp"
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"
#include "Zancle/Trait/UnderlyingType.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Fixed-size array container indexed by an enumeration
///
/// `EnumArray<Enum, Value, Count>` stores `Count` instances of `Value`
/// and exposes them through `operator[]` taking an `Enum` key. The
/// underlying enum value is converted to its integer representation,
/// which must be in `[0, Count)` (asserted in debug builds).
///
/// An aggregate (like `za::Array`), propagating trivial relocatability
/// from `Value`. Zero-sized arrays are not allowed.
///
/// Useful for tables keyed by an enum class, e.g. per-event-type or
/// per-shader-channel state.
///
////////////////////////////////////////////////////////////
template <typename Enum, typename Value, SizeT Count>
struct [[nodiscard]] ZA_GSL_OWNER(Value) EnumArray
{
    ////////////////////////////////////////////////////////////
    ZA_ENABLE_TRIVIAL_RELOCATION_IF(ZA_IS_TRIVIALLY_RELOCATABLE(Value));


    ////////////////////////////////////////////////////////////
    static_assert(ZA_IS_ENUM(Enum));
    static_assert(Count > 0, "Zero-sized enum arrays are not supported");


    ////////////////////////////////////////////////////////////
    /// \brief Convert `key` to an index, asserting it is in `[0, Count)`
    ///
    /// Goes through the underlying type and `U64` so that wide
    /// underlying types cannot silently truncate on 32-bit `SizeT`,
    /// and negative values of signed underlying types wrap to huge
    /// values that fail the range assertion.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] static constexpr SizeT indexOf(const Enum key) noexcept
    {
        const auto index = static_cast<U64>(static_cast<ZA_UNDERLYING_TYPE(Enum)>(key));

        ZA_ASSERT(index < Count);
        return static_cast<SizeT>(index);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Returns a reference to the element associated to specified \a key
    ///
    /// No bounds checking is performed in release builds.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Value& operator[](const Enum key) noexcept ZA_LIFETIMEBOUND
    {
        return elements[indexOf(key)];
    }


    ////////////////////////////////////////////////////////////
    /// \brief Returns a reference to the element associated to specified \a key
    ///
    /// No bounds checking is performed in release builds.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr const Value& operator[](const Enum key) const noexcept ZA_LIFETIMEBOUND
    {
        return elements[indexOf(key)];
    }


    ////////////////////////////////////////////////////////////
    /// \brief Assign `fillValue` to every element of the array
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr void fill(const Value& fillValue)
    {
        for (Value& value : elements)
            value = fillValue;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Underlying contiguous storage; exposed publicly to remain an aggregate
    ///
    ////////////////////////////////////////////////////////////
    Value elements[Count];
};

} // namespace za
