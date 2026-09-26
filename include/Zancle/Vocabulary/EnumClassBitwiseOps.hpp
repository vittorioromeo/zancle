#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Trait/UnderlyingType.hpp"


////////////////////////////////////////////////////////////
/// \brief Define `&`, `|`, `^`, `~`, `&=`, `|=`, `^=`, and `!` for a scoped enum
///
/// Generates the standard set of bitwise operators that operate on the
/// enum's underlying type and return values back as the enum. Useful
/// for flag-style scoped enums (`enum class Flags : unsigned int`).
///
////////////////////////////////////////////////////////////
#define ZA_DEFINE_ENUM_CLASS_BITWISE_OPS(enumType)                                                                               \
                                                                                                                                 \
    [[nodiscard, maybe_unused, gnu::always_inline, gnu::const]] inline constexpr bool operator!(const enumType lhs) noexcept     \
    {                                                                                                                            \
        return !static_cast<bool>(lhs);                                                                                          \
    }                                                                                                                            \
                                                                                                                                 \
    [[nodiscard, maybe_unused, gnu::always_inline, gnu::const]] inline constexpr enumType operator|(const enumType lhs,          \
                                                                                                    const enumType rhs) noexcept \
    {                                                                                                                            \
        return static_cast<enumType>(                                                                                            \
            static_cast<ZA_UNDERLYING_TYPE(enumType)>(lhs) | static_cast<ZA_UNDERLYING_TYPE(enumType)>(rhs));                    \
    }                                                                                                                            \
                                                                                                                                 \
    [[nodiscard, maybe_unused, gnu::always_inline, gnu::const]] inline constexpr enumType operator&(const enumType lhs,          \
                                                                                                    const enumType rhs) noexcept \
    {                                                                                                                            \
        return static_cast<enumType>(                                                                                            \
            static_cast<ZA_UNDERLYING_TYPE(enumType)>(lhs) & static_cast<ZA_UNDERLYING_TYPE(enumType)>(rhs));                    \
    }                                                                                                                            \
                                                                                                                                 \
    [[nodiscard, maybe_unused, gnu::always_inline, gnu::const]] inline constexpr enumType operator^(const enumType lhs,          \
                                                                                                    const enumType rhs) noexcept \
    {                                                                                                                            \
        return static_cast<enumType>(                                                                                            \
            static_cast<ZA_UNDERLYING_TYPE(enumType)>(lhs) ^ static_cast<ZA_UNDERLYING_TYPE(enumType)>(rhs));                    \
    }                                                                                                                            \
                                                                                                                                 \
    [[nodiscard, maybe_unused, gnu::always_inline, gnu::const]] inline constexpr enumType operator~(const enumType lhs) noexcept \
    {                                                                                                                            \
        return static_cast<enumType>(~static_cast<ZA_UNDERLYING_TYPE(enumType)>(lhs));                                           \
    }                                                                                                                            \
                                                                                                                                 \
    [[maybe_unused, gnu::always_inline]] inline constexpr enumType& operator|=(enumType& lhs, const enumType rhs) noexcept       \
    {                                                                                                                            \
        return lhs = (lhs | rhs);                                                                                                \
    }                                                                                                                            \
                                                                                                                                 \
    [[maybe_unused, gnu::always_inline]] inline constexpr enumType& operator&=(enumType& lhs, const enumType rhs) noexcept       \
    {                                                                                                                            \
        return lhs = (lhs & rhs);                                                                                                \
    }                                                                                                                            \
                                                                                                                                 \
    [[maybe_unused, gnu::always_inline]] inline constexpr enumType& operator^=(enumType& lhs, const enumType rhs) noexcept       \
    {                                                                                                                            \
        return lhs = (lhs ^ rhs);                                                                                                \
    }                                                                                                                            \
                                                                                                                                 \
    static_assert(true)
