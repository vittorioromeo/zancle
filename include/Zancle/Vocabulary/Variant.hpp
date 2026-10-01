#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Vocabulary/OverloadSet.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IndexSequence.hpp"
#include "Zancle/Base/Launder.hpp"
#include "Zancle/Base/LifetimeAttributes.hpp"
#include "Zancle/Base/MakeIndexSequence.hpp"
#include "Zancle/Base/PlacementNew.hpp"
#include "Zancle/Base/ScopeGuard.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/TypePackElement.hpp"
#include "Zancle/Base/TypePackIndex.hpp"

#include "Zancle/Trait/Conditional.hpp"
#include "Zancle/Trait/CopyCV.hpp"
#include "Zancle/Trait/DeclVal.hpp"
#include "Zancle/Trait/EnableTrivialRelocation.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsMoveAssignable.hpp"
#include "Zancle/Trait/IsMoveConstructible.hpp"
#include "Zancle/Trait/IsReference.hpp"
#include "Zancle/Trait/IsRvalueReference.hpp"
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/IsTriviallyCopyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyConstructible.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyMoveAssignable.hpp"
#include "Zancle/Trait/IsTriviallyMoveConstructible.hpp"
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"
#include "Zancle/Trait/RemoveCVRef.hpp"
#include "Zancle/Trait/RemoveReference.hpp"


////////////////////////////////////////////////////////////
#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wshadow"
#endif


namespace za::priv
{
////////////////////////////////////////////////////////////
[[nodiscard]] consteval auto variadicMax(auto x, auto... xs) noexcept
{
    decltype(x) result = x;
    ((result = (result < xs) ? xs : result), ...);
    return result;
}


////////////////////////////////////////////////////////////
template <typename>
struct InPlaceType
{
};


////////////////////////////////////////////////////////////
template <SizeT>
struct InPlaceIndex
{
};


////////////////////////////////////////////////////////////
// Models `std::forward_like<Self, T>`'s result type: propagates
// the cv-ref qualification of `Self` (as deduced by a C++23
// "deducing this" parameter `this Self&& self`) onto `T`.
template <typename Self, typename T>
using LikeT = Conditional<ZA_IS_RVALUE_REFERENCE(Self&&), CopyCV<ZA_REMOVE_REFERENCE(Self), T>&&, CopyCV<ZA_REMOVE_REFERENCE(Self), T>&>;

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Tag value used to construct a `Variant` in-place by alternative type
///
////////////////////////////////////////////////////////////
template <typename T>
inline constexpr priv::InPlaceType<T> inPlaceType{};


////////////////////////////////////////////////////////////
/// \brief Tag value used to construct a `Variant` in-place by alternative index
///
////////////////////////////////////////////////////////////
template <SizeT N>
inline constexpr priv::InPlaceIndex<N> inPlaceIndex{};


////////////////////////////////////////////////////////////
#define ZA_VARIANT_NTH_TYPE(i) ZA_TYPE_PACK_ELEMENT(i, Alternatives...)


////////////////////////////////////////////////////////////
/// \brief Lightweight tagged-union replacement for `std::variant`
///
/// Holds exactly one value chosen from the type pack `Alternatives...`.
/// Storage is in-place and never allocates. Special members are
/// conditionally defaulted when all alternatives are trivially
/// copy/move/destructible to keep the type trivial whenever possible.
///
/// Compared to `std::variant`, `za::Variant`:
/// - avoids the heavy `<variant>` standard header
/// - has no "valueless by exception" state: it is intentionally not
///   exception-safe, so alternatives must not throw from their special
///   members (construction, assignment, destruction)
/// - exposes both a recursive and a linear visit strategy, so callers
///   can pick the one with the best codegen for their alternative count
/// - propagates trivial relocatability when all alternatives are
///   trivially relocatable
///
/// The number of alternatives is limited to 254 (the discriminator is
/// stored in a single byte).
///
////////////////////////////////////////////////////////////
template <typename... Alternatives>
class Variant
{
    static_assert(sizeof...(Alternatives) < 255);

private:
    ////////////////////////////////////////////////////////////
    using Byte = unsigned char;


    ////////////////////////////////////////////////////////////
    enum : SizeT
    {
        alternativeCount = sizeof...(Alternatives),
        maxAlignment     = priv::variadicMax(alignof(Alternatives)...),
        maxSize          = priv::variadicMax(sizeof(Alternatives)...)
    };


    ////////////////////////////////////////////////////////////
    static inline constexpr bool triviallyDestructible      = (... && isTriviallyDestructible<Alternatives>);
    static inline constexpr bool triviallyCopyConstructible = (... && isTriviallyCopyConstructible<Alternatives>);
    static inline constexpr bool triviallyMoveConstructible = (... && isTriviallyMoveConstructible<Alternatives>);

    // Assignment can switch the active alternative, so a byte-wise copy is
    // only correct if construction and destruction are trivial too
    static inline constexpr bool triviallyCopyAssignable = (... && isTriviallyCopyAssignable<Alternatives>) &&
                                                           triviallyCopyConstructible && triviallyDestructible;
    static inline constexpr bool triviallyMoveAssignable = (... && isTriviallyMoveAssignable<Alternatives>) &&
                                                           triviallyMoveConstructible && triviallyDestructible;

    // The non-trivial special members additionally require every alternative to support
    // the operation, so that e.g. `isCopyConstructible<Variant<int, UniquePtr<int>>>` is `false`
    static inline constexpr bool copyConstructible = (... && isCopyConstructible<Alternatives>);
    static inline constexpr bool moveConstructible = (... && isMoveConstructible<Alternatives>);
    static inline constexpr bool copyAssignable    = copyConstructible && (... && isCopyAssignable<Alternatives>);
    static inline constexpr bool moveAssignable    = moveConstructible && (... && isMoveAssignable<Alternatives>);


    ////////////////////////////////////////////////////////////
    using DiscriminatorType = unsigned char; // Supports up to 254 alternatives (see static assertion above)


public:
    ////////////////////////////////////////////////////////////
    ZA_ENABLE_TRIVIAL_RELOCATION_IF(ZA_IS_TRIVIALLY_RELOCATABLE(Alternatives) && ...);


    ////////////////////////////////////////////////////////////
    template <typename T>
    static constexpr SizeT indexOf = getTypePackIndex<T, Alternatives...>();


    ////////////////////////////////////////////////////////////
    template <typename T>
    static constexpr bool isAlternative = indexOf<T> != badTypePackIndex;


private:
    ////////////////////////////////////////////////////////////
    static constexpr MakeIndexSequence<alternativeCount> alternativeIndexSequence{};


    ////////////////////////////////////////////////////////////
    alignas(maxAlignment) Byte m_buffer[maxSize];
    DiscriminatorType m_index;


    ////////////////////////////////////////////////////////////
#define ZA_VARIANT_STATIC_ASSERT_INDEX_VALIDITY(I)                                             \
    static_assert((I) != ::za::badTypePackIndex, "Alternative type not supported by variant"); \
    static_assert((I) < alternativeCount, "Alternative index out of range")


    ////////////////////////////////////////////////////////////
    // Short-circuits after the active alternative, so only the necessary
    // comparisons are performed (relevant in debug mode).
#define ZA_VARIANT_DO_WITH_CURRENT_INDEX_OBJ(obj, Is, ...)                                                 \
    do                                                                                                     \
    {                                                                                                      \
        [&]<SizeT... Is> [[gnu::always_inline]] (IndexSequence<Is...>)                                     \
        {                                                                                                  \
            static_cast<void>((... || (((obj).m_index == Is) && (static_cast<void>(__VA_ARGS__), true)))); \
        }(alternativeIndexSequence);                                                                       \
    } while (false)


    ////////////////////////////////////////////////////////////
#define ZA_VARIANT_DO_WITH_CURRENT_INDEX(Is, ...) ZA_VARIANT_DO_WITH_CURRENT_INDEX_OBJ((*this), Is, __VA_ARGS__)


    ////////////////////////////////////////////////////////////
    // Unchecked buffer accessor used by internal paths whose callers
    // have already proven the active alternative (e.g. from inside
    // `DO_WITH_CURRENT_INDEX`). Public access goes through `as<T>()`,
    // which additionally asserts the discriminator.
    ////////////////////////////////////////////////////////////
    template <typename T, typename Self>
    [[nodiscard, gnu::always_inline]] auto&& bufferAs(this Self&& self) noexcept
    {
        using Ret = priv::LikeT<Self, T>;
        using Ptr = ZA_REMOVE_REFERENCE(Ret)*;
        return static_cast<Ret>(*ZA_LAUNDER_CAST(Ptr, self.m_buffer));
    }


    ////////////////////////////////////////////////////////////
    // Unlike the built-in `&`, also accepts xvalues (e.g. a visitor returning `T&&`)
    template <typename T>
    [[nodiscard, gnu::always_inline, gnu::const]] static auto* addressOf(T&& x) noexcept
    {
        return &x;
    }


    ////////////////////////////////////////////////////////////
    template <SizeT I>
    [[gnu::always_inline]] void destroyAt() noexcept
    {
        using Type = ZA_VARIANT_NTH_TYPE(I);
        // Discriminator check omitted: callers gate on `m_index == I` via `DO_WITH_CURRENT_INDEX`.
        bufferAs<Type>().~Type();
    }


    ////////////////////////////////////////////////////////////
    // Destroys the active alternative. Trivially destructible alternatives
    // are skipped entirely, including their discriminator comparison.
    [[gnu::always_inline]] void destroyCurrent() noexcept
    {
        [&]<SizeT... Is> [[gnu::always_inline]] (IndexSequence<Is...>)
        {
            static_cast<void>(
                (... || (!isTriviallyDestructible<ZA_VARIANT_NTH_TYPE(Is)> && m_index == Is && (destroyAt<Is>(), true))));
        }(alternativeIndexSequence);
    }


    ////////////////////////////////////////////////////////////
    // Each of the three recursive helpers below is a single static function
    // template that perfect-forwards `self`. `bufferAs` is itself a
    // deducing-this template, so `static_cast<Self&&>(self).bufferAs<T>()`
    // propagates the cvref onto the returned reference automatically. The
    // unchecked `bufferAs` is used as the index was just compared.
    ////////////////////////////////////////////////////////////
    template <SizeT I, typename R, typename Self, typename Visitor>
    [[nodiscard, gnu::always_inline]] static R recursiveVisitImpl(Self&& self, Visitor&& visitor)
    {
        if constexpr (I < sizeof...(Alternatives) - 1)
        {
            return (self.m_index == I)
                       ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I)>())
                       : recursiveVisitImpl<I + 1, R>(static_cast<Self&&>(self), static_cast<Visitor&&>(visitor));
        }
        else
        {
            return visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I)>());
        }
    }


    ////////////////////////////////////////////////////////////
    // The chunked helpers below only unroll a chunk when at least one
    // alternative remains after it, as the recursive call in the chunk's
    // last branch is always instantiated. Otherwise, they fall back to the
    // next smaller helper.
    ////////////////////////////////////////////////////////////
    template <SizeT I, typename R, typename Self, typename Visitor>
    [[nodiscard, gnu::always_inline]] static R recursiveVisitImplOpt5(Self&& self, Visitor&& visitor)
    {
        if constexpr (I + 5 < sizeof...(Alternatives))
        {
            // clang-format off
            return (self.m_index == I + 0) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 0)>()) :
                   (self.m_index == I + 1) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 1)>()) :
                   (self.m_index == I + 2) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 2)>()) :
                   (self.m_index == I + 3) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 3)>()) :
                   (self.m_index == I + 4) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 4)>()) :
                   recursiveVisitImplOpt5<I + 5, R>(static_cast<Self&&>(self), static_cast<Visitor&&>(visitor));
            // clang-format on
        }
        else
        {
            return recursiveVisitImpl<I, R>(static_cast<Self&&>(self), static_cast<Visitor&&>(visitor));
        }
    }


    ////////////////////////////////////////////////////////////
    template <SizeT I, typename R, typename Self, typename Visitor>
    [[nodiscard, gnu::always_inline]] static R recursiveVisitImplOpt10(Self&& self, Visitor&& visitor)
    {
        if constexpr (I + 10 < sizeof...(Alternatives))
        {
            // clang-format off
            return (self.m_index == I + 0) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 0)>()) :
                   (self.m_index == I + 1) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 1)>()) :
                   (self.m_index == I + 2) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 2)>()) :
                   (self.m_index == I + 3) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 3)>()) :
                   (self.m_index == I + 4) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 4)>()) :
                   (self.m_index == I + 5) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 5)>()) :
                   (self.m_index == I + 6) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 6)>()) :
                   (self.m_index == I + 7) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 7)>()) :
                   (self.m_index == I + 8) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 8)>()) :
                   (self.m_index == I + 9) ? visitor(static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I + 9)>()) :
                   recursiveVisitImplOpt10<I + 10, R>(static_cast<Self&&>(self), static_cast<Visitor&&>(visitor));
            // clang-format on
        }
        else
        {
            return recursiveVisitImplOpt5<I, R>(static_cast<Self&&>(self), static_cast<Visitor&&>(visitor));
        }
    }


public:
    ////////////////////////////////////////////////////////////
    /// \brief Construct alternative `I` in-place from forwarded `args`
    ///
    ////////////////////////////////////////////////////////////
    template <SizeT I, typename... Args>
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    [[nodiscard, gnu::always_inline]] explicit Variant(const priv::InPlaceIndex<I>, Args&&... args) noexcept :
        m_index{static_cast<DiscriminatorType>(I)}
    {
        ZA_VARIANT_STATIC_ASSERT_INDEX_VALIDITY(I);
        // Parentheses (like `Optional`): no narrowing errors, and no initializer-list constructor hijacking
        ZA_PLACEMENT_NEW(m_buffer) ZA_VARIANT_NTH_TYPE(I)(static_cast<Args&&>(args)...);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct alternative `T` in-place from forwarded `args`
    ///
    ////////////////////////////////////////////////////////////
    template <typename T, typename... Args>
    [[nodiscard, gnu::always_inline]] explicit Variant(const priv::InPlaceType<T>, Args&&... args) noexcept :
        Variant{inPlaceIndex<indexOf<T>>, static_cast<Args&&>(args)...}
    {
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct from any value whose decayed type is one of the alternatives
    ///
    /// The alternative is chosen by exact type match (after removing
    /// cvref) -- there is no implicit conversion picking.
    ///
    ////////////////////////////////////////////////////////////
    template <typename T>
    [[nodiscard, gnu::always_inline]] explicit Variant(T&& x) noexcept
        requires(isAlternative<RemoveCVRefIndirect<T>>)
        : Variant{inPlaceType<ZA_REMOVE_CVREF(T)>, static_cast<T&&>(x)}
    {
    }


    ////////////////////////////////////////////////////////////
    /// \brief Default constructor, holds the first alternative default-constructed
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] explicit Variant() noexcept : Variant{inPlaceIndex<0>}
    {
    }


    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    [[gnu::always_inline]] Variant(const Variant& rhs)
        requires(!triviallyCopyConstructible && copyConstructible)
        : m_index{rhs.m_index}
    {
        ZA_VARIANT_DO_WITH_CURRENT_INDEX(I,
                                         ZA_PLACEMENT_NEW(m_buffer)
                                             ZA_VARIANT_NTH_TYPE(I)(rhs.template bufferAs<ZA_VARIANT_NTH_TYPE(I)>()));
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] Variant(const Variant& rhs)
        requires(triviallyCopyConstructible)
    = default;


    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    [[gnu::always_inline]] Variant(Variant&& rhs) noexcept
        requires(!triviallyMoveConstructible && moveConstructible)
        : m_index{rhs.m_index}
    {
        ZA_VARIANT_DO_WITH_CURRENT_INDEX(I,
                                         ZA_PLACEMENT_NEW(m_buffer)
                                             ZA_VARIANT_NTH_TYPE(I)(static_cast<ZA_VARIANT_NTH_TYPE(I) &&>(
                                                 rhs.template bufferAs<ZA_VARIANT_NTH_TYPE(I)>())));
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] Variant(Variant&& rhs) noexcept
        requires(triviallyMoveConstructible)
    = default;


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] ~Variant()
        requires(!triviallyDestructible)
    {
        destroyCurrent();
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] ~Variant()
        requires(triviallyDestructible)
    = default;


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] Variant& operator=(const Variant& rhs)
        requires(!triviallyCopyAssignable && copyAssignable)
    {
        if (this == &rhs)
            return *this;

        if (m_index == rhs.m_index)
        {
            // Same alternative on both sides: delegate to the alternative's copy-assignment.
            // Safer than destroy+construct (no transient destroyed state) and preserves the
            // alternative's own self-assignment and exception-safety semantics.
            ZA_VARIANT_DO_WITH_CURRENT_INDEX(I,
                                             bufferAs<ZA_VARIANT_NTH_TYPE(I)>() = rhs.template bufferAs<ZA_VARIANT_NTH_TYPE(I)>());
            return *this;
        }

        destroyCurrent();

        ZA_VARIANT_DO_WITH_CURRENT_INDEX_OBJ(rhs,
                                             I,
                                             (ZA_PLACEMENT_NEW(m_buffer) ZA_VARIANT_NTH_TYPE(I)(
                                                 rhs.template bufferAs<ZA_VARIANT_NTH_TYPE(I)>())));
        m_index = rhs.m_index;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] Variant& operator=(const Variant& rhs)
        requires(triviallyCopyAssignable)
    = default;


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] Variant& operator=(Variant&& rhs) noexcept
        requires(!triviallyMoveAssignable && moveAssignable)
    {
        if (this == &rhs)
            return *this;

        if (m_index == rhs.m_index)
        {
            // Same alternative on both sides: delegate to the alternative's move-assignment.
            ZA_VARIANT_DO_WITH_CURRENT_INDEX(I,
                                             bufferAs<ZA_VARIANT_NTH_TYPE(I)>() = static_cast<ZA_VARIANT_NTH_TYPE(I) &&>(
                                                 rhs.template bufferAs<ZA_VARIANT_NTH_TYPE(I)>()));
            return *this;
        }

        destroyCurrent();

        ZA_VARIANT_DO_WITH_CURRENT_INDEX_OBJ(rhs,
                                             I,
                                             (ZA_PLACEMENT_NEW(m_buffer)
                                                  ZA_VARIANT_NTH_TYPE(I)(static_cast<ZA_VARIANT_NTH_TYPE(I) &&>(
                                                      rhs.template bufferAs<ZA_VARIANT_NTH_TYPE(I)>()))));
        m_index = rhs.m_index;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] Variant& operator=(Variant&& rhs) noexcept
        requires(triviallyMoveAssignable)
    = default;


    ////////////////////////////////////////////////////////////
    /// \brief Assign a value whose decayed type is one of the alternatives
    ///
    /// Destroys the current value and reconstructs the variant to hold a
    /// new alternative selected by exact type match (after removing cvref).
    ///
    ////////////////////////////////////////////////////////////
    template <typename T>
    [[gnu::always_inline]] Variant& operator=(T&& x)
        requires(isAlternative<RemoveCVRefIndirect<T>>)
    {
        using Type = ZA_REMOVE_CVREF(T);

        if (m_index == indexOf<Type>)
        {
            // Same alternative already active: delegate to the alternative's assignment.
            // Also the only aliasing-safe branch when `x` binds to the currently-held value
            // (e.g. `v = v.as<Type>()`); a destroy-then-construct sequence would read from
            // destroyed storage.
            bufferAs<Type>() = static_cast<T&&>(x);
            return *this;
        }

        destroyCurrent();

        ZA_PLACEMENT_NEW(m_buffer) Type(static_cast<T&&>(x));
        m_index = indexOf<Type>;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    /// \brief `true` if the active alternative is `T`
    ///
    ////////////////////////////////////////////////////////////
    template <typename T>
    [[nodiscard, gnu::always_inline]] bool is() const noexcept
    {
        ZA_VARIANT_STATIC_ASSERT_INDEX_VALIDITY(indexOf<T>);
        return m_index == indexOf<T>;
    }


    ////////////////////////////////////////////////////////////
    /// \brief `true` if the active alternative index equals `index`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool hasIndex(const DiscriminatorType index) const noexcept
    {
        return m_index == index;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Pointer-typed access to alternative `T`, or `nullptr` if not active
    ///
    /// Constrained to lvalue variants: returning a pointer into an
    /// rvalue's storage would dangle at the end of the full expression.
    ///
    ////////////////////////////////////////////////////////////
    template <typename T, typename Self>
    [[nodiscard, gnu::always_inline]] auto* getIf(this Self&& self) noexcept
        requires(!ZA_IS_RVALUE_REFERENCE(Self &&))
    {
        ZA_VARIANT_STATIC_ASSERT_INDEX_VALIDITY(indexOf<T>);
        using Ptr = CopyCV<ZA_REMOVE_REFERENCE(Self), T>*;
        return self.m_index == indexOf<T> ? ZA_LAUNDER_CAST(Ptr, self.m_buffer) : nullptr;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Unchecked access to alternative `T` (asserts that it is active)
    ///
    /// Propagates the cv-ref qualification of `*this` onto the returned
    /// reference: `T&`, `const T&`, `T&&`, or `const T&&` as appropriate.
    ///
    ////////////////////////////////////////////////////////////
    template <typename T, typename Self>
    [[nodiscard, gnu::always_inline]] auto&& as(this Self&& self ZA_LIFETIMEBOUND) noexcept
    {
        ZA_VARIANT_STATIC_ASSERT_INDEX_VALIDITY(indexOf<T>);
        ZA_ASSERT(self.m_index == indexOf<T>);
        return static_cast<Self&&>(self).template bufferAs<T>();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Unchecked access to the alternative at index `I`
    ///
    /// Propagates the cv-ref qualification of `*this` onto the returned
    /// reference (same rules as `as<T>()`).
    ///
    ////////////////////////////////////////////////////////////
    template <SizeT I, typename Self>
    [[nodiscard, gnu::always_inline]] auto&& getByIndex(this Self&& self ZA_LIFETIMEBOUND) noexcept
    {
        ZA_VARIANT_STATIC_ASSERT_INDEX_VALIDITY(I);
        ZA_ASSERT(self.m_index == I); // not via `as<T>()`, which would fail for duplicate alternatives
        return static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I)>();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Visit the active alternative via tail-call recursive dispatch
    ///
    /// Generally faster for small alternative counts. Return type is deduced
    /// from the visitor's call on the first alternative.
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self,
              typename Visitor,
              typename R = decltype(declVal<Visitor&&>()(declVal<priv::LikeT<Self, ZA_VARIANT_NTH_TYPE(0)>>()))>
    [[nodiscard, gnu::always_inline, gnu::flatten]] R recursiveVisit(this Self&& self, Visitor&& visitor)
    {
        if constexpr (sizeof...(Alternatives) >= 10)
            return recursiveVisitImplOpt10<0, R>(static_cast<Self&&>(self), static_cast<Visitor&&>(visitor));
        else if constexpr (sizeof...(Alternatives) >= 5)
            return recursiveVisitImplOpt5<0, R>(static_cast<Self&&>(self), static_cast<Visitor&&>(visitor));
        else
            return recursiveVisitImpl<0, R>(static_cast<Self&&>(self), static_cast<Visitor&&>(visitor));
    }


    ////////////////////////////////////////////////////////////
    /// \brief Pattern-matching `recursiveVisit`: combines per-alternative lambdas via `OverloadSet`
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self, typename... Fs>
    [[nodiscard, gnu::always_inline, gnu::flatten]] decltype(auto) recursiveMatch(this Self&& self, Fs&&... fs)
    {
        return static_cast<Self&&>(self).recursiveVisit(OverloadSet{static_cast<Fs&&>(fs)...});
    }


    ////////////////////////////////////////////////////////////
    /// \brief Visit the active alternative via a single linear (fold) dispatch
    ///
    /// Often produces better codegen than `recursiveVisit` for moderate
    /// alternative counts when the visitor returns a non-void, non-reference type.
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self,
              typename Visitor,
              typename R = decltype(declVal<Visitor&&>()(declVal<priv::LikeT<Self, ZA_VARIANT_NTH_TYPE(0)>>()))>
    [[nodiscard, gnu::always_inline]] R linearVisit(this Self&& self, Visitor&& visitor)
    {
        if constexpr (ZA_IS_REFERENCE(R))
        {
            ZA_REMOVE_REFERENCE(R) * ret; // NOLINT(cppcoreguidelines-init-variables)
            ZA_VARIANT_DO_WITH_CURRENT_INDEX_OBJ(self,
                                                 I,
                                                 ret = addressOf(visitor(
                                                     static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I)>())));
            return static_cast<R>(*ret);
        }
        else if constexpr (ZA_IS_SAME(R, void))
        {
            ZA_VARIANT_DO_WITH_CURRENT_INDEX_OBJ(self,
                                                 I,
                                                 (visitor(
                                                     static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I)>())));
        }
        else
        {
            alignas(R) Byte retBuffer[sizeof(R)];

            ZA_VARIANT_DO_WITH_CURRENT_INDEX_OBJ(self,
                                                 I,
                                                 ZA_PLACEMENT_NEW(retBuffer) R(visitor(
                                                     static_cast<Self&&>(self).template bufferAs<ZA_VARIANT_NTH_TYPE(I)>())));

            // Only guard after construction, as the visitor may throw before `R` exists
            ZA_SCOPE_GUARD({
                if constexpr (!ZA_IS_TRIVIALLY_DESTRUCTIBLE(R))
                    ZA_LAUNDER_CAST(R*, retBuffer)->~R();
            });

            return static_cast<R&&>(*ZA_LAUNDER_CAST(R*, retBuffer));
        }
    }


    ////////////////////////////////////////////////////////////
    /// \brief Pattern-matching `linearVisit`: combines per-alternative lambdas via `OverloadSet`
    ///
    ////////////////////////////////////////////////////////////
    template <typename Self, typename... Fs>
    [[nodiscard, gnu::always_inline, gnu::flatten]] decltype(auto) linearMatch(this Self&& self, Fs&&... fs)
    {
        return static_cast<Self&&>(self).linearVisit(OverloadSet{static_cast<Fs&&>(fs)...});
    }
};

#undef ZA_VARIANT_DO_WITH_CURRENT_INDEX
#undef ZA_VARIANT_DO_WITH_CURRENT_INDEX_OBJ
#undef ZA_VARIANT_STATIC_ASSERT_INDEX_VALIDITY
#undef ZA_VARIANT_NTH_TYPE

} // namespace za

#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif
