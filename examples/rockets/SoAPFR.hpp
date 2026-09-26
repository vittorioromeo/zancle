#pragma once

#include "Zancle/Reflection/Rfl.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IndexSequence.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/MakeIndexSequence.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/TypePackElement.hpp"

#include "Zancle/Trait/IsMemberPointer.hpp"


namespace priv
{
////////////////////////////////////////////////////////////
template <za::SizeT I, typename T>
struct SoABase
{
    za::Vector<T> data;
};

} // namespace priv

////////////////////////////////////////////////////////////
template <typename, typename, typename...>
class SoA;

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define SOA_AS_BASE(I)       static_cast<priv::SoABase<I, ZA_TYPE_PACK_ELEMENT(I, Ts...)>&>(*this)
#define SOA_AS_CONST_BASE(I) static_cast<const priv::SoABase<I, ZA_TYPE_PACK_ELEMENT(I, Ts...)>&>(*this)
#define SOA_ALL_BASES()      static_cast<priv::SoABase<Is, Ts>&>(*this)

////////////////////////////////////////////////////////////
template <typename T, za::SizeT... Is, typename... Ts>
class SoA<T, za::IndexSequence<Is...>, Ts...> : private priv::SoABase<Is, Ts>...
{
private:
    ////////////////////////////////////////////////////////////
    template <auto PM>
    [[nodiscard]] static consteval za::SizeT fieldIndexFromPM()
    {
        // const auto& obj = za::rfl::priv::getFakeObject<T>();
        constexpr T obj{};

        auto result = static_cast<za::SizeT>(-1);

        (...,
         ((static_cast<const void*>(&(za::rfl::getField<Is>(obj))) == static_cast<const void*>(&(obj.*PM)))
              ? (result = Is)
              : 0));

        return result;
    }

public:
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void clear()
    {
        (..., SOA_ALL_BASES().data.clear());
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void reserve(const za::SizeT capacity)
    {
        (..., SOA_ALL_BASES().data.reserve(capacity));
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void resize(const za::SizeT size)
    {
        (..., SOA_ALL_BASES().data.resize(size));
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void pushBack(const T& aos)
    {
        emplaceBack(za::rfl::getField<Is>(aos)...);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void emplaceBack(auto&&... values)
    {
        (..., SOA_ALL_BASES().data.emplaceBack(ZA_FORWARD(values)));
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] za::SizeT getSize() const
    {
        return SOA_AS_CONST_BASE(0).data.size();
    }

    ////////////////////////////////////////////////////////////
    template <za::SizeT I>
    [[nodiscard, gnu::always_inline]] auto& get() noexcept
    {
        return SOA_AS_BASE(I).data;
    }

    ////////////////////////////////////////////////////////////
    template <za::SizeT I>
    [[nodiscard, gnu::always_inline]] const auto& get() const noexcept
    {
        return SOA_AS_CONST_BASE(I).data;
    }

    ////////////////////////////////////////////////////////////
    template <auto PM>
    [[nodiscard, gnu::always_inline]] auto& get() noexcept
        requires za::isMemberPointer<decltype(PM)>
    {
        return get<fieldIndexFromPM<PM>()>();
    }

    ////////////////////////////////////////////////////////////
    template <auto PM>
    [[nodiscard, gnu::always_inline]] const auto& get() const noexcept
        requires za::isMemberPointer<decltype(PM)>
    {
        return get<fieldIndexFromPM<PM>()>();
    }

    ////////////////////////////////////////////////////////////
    template <za::SizeT... Js>
    [[gnu::always_inline]] void withNth(const za::SizeT i, auto&& f)
    {
        f(SOA_AS_BASE(Js).data[i]...);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void withAllNth(const za::SizeT i, auto&& f)
    {
        f(SOA_ALL_BASES().data[i]...);
    }

    ////////////////////////////////////////////////////////////
    template <za::SizeT... Js>
    [[gnu::always_inline]] void withSubRange(const za::SizeT start, const za::SizeT end, auto&& f)
    {
        ZA_ASSERT(end <= getSize());
        ZA_ASSERT(start <= end);

        for (za::SizeT i = start; i < end; ++i)
            f(SOA_AS_BASE(Js).data[i]...);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void withAllSubRange(const za::SizeT start, const za::SizeT end, auto&& f)
    {
        ZA_ASSERT(end <= getSize());
        ZA_ASSERT(start <= end);

        for (za::SizeT i = start; i < end; ++i)
            f(SOA_ALL_BASES().data[i]...);
    }

    ////////////////////////////////////////////////////////////
    template <za::SizeT... Js>
    [[gnu::always_inline]] void with(auto&& f)
    {
        const za::SizeT size = getSize();

        for (za::SizeT i = 0u; i < size; ++i)
            f(SOA_AS_BASE(Js).data[i]...);
    }

    ////////////////////////////////////////////////////////////
    template <auto... PMs>
    [[gnu::always_inline]] void with(auto&& f)
        requires(... && za::isMemberPointer<decltype(PMs)>)
    {
        with<fieldIndexFromPM<PMs>()...>(ZA_FORWARD(f));
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void withAll(auto&& f)
    {
        const za::SizeT size = getSize();

        for (za::SizeT i = 0u; i < size; ++i)
            f(SOA_ALL_BASES().data[i]...);
    }

    ////////////////////////////////////////////////////////////
    template <za::SizeT... Js>
    void eraseIfByShifting(auto&& f)
    {
        const za::SizeT n = getSize();

        // Find the first element to remove.
        za::SizeT i = 0u;
        while (i < n && !f(SOA_AS_BASE(Js).data[i]...))
            ++i;

        // For the remaining elements, shift over those that must be kept.
        za::SizeT newSize = i;

        for (; i < n; ++i)
        {
            if (f(SOA_AS_BASE(Js).data[i]...))
                continue;

            if (newSize != i)
                (..., (SOA_ALL_BASES().data[newSize] = ZA_MOVE(SOA_ALL_BASES().data[i])));

            ++newSize;
        }

        // Resize all columns to the new size.
        (..., SOA_ALL_BASES().data.resize(newSize));
    }

    ////////////////////////////////////////////////////////////
    template <za::SizeT... Js>
    void eraseIfBySwapping(auto&& f)
    {
        za::SizeT currentSize = getSize();

        for (za::SizeT i = currentSize; i-- > 0u;)
        {
            if (!f(SOA_AS_BASE(Js).data[i]...))
                continue;

            --currentSize;
            (..., (SOA_ALL_BASES().data[i] = ZA_MOVE(SOA_ALL_BASES().data[currentSize])));
        }

        (..., SOA_ALL_BASES().data.resize(currentSize));
    }

    ////////////////////////////////////////////////////////////
    template <auto... PMs>
    [[gnu::always_inline]] void eraseIfBySwapping(auto&& f)
        requires(... && za::isMemberPointer<decltype(PMs)>)
    {
        eraseIfBySwapping<fieldIndexFromPM<PMs>()...>(ZA_FORWARD(f));
    }
};

////////////////////////////////////////////////////////////
#undef SOA_ALL_BASES
#undef SOA_AS_CONST_BASE
#undef SOA_AS_BASE

////////////////////////////////////////////////////////////
template <typename, typename T>
class SoAHelper;

////////////////////////////////////////////////////////////
template <za::SizeT... Is, typename T>
class SoAHelper<za::IndexSequence<Is...>, T>
{
public:
    using Type = SoA<T, za::IndexSequence<Is...>, za::rfl::FieldType<Is, T>...>;
};

////////////////////////////////////////////////////////////
template <typename T>
using SoAFor = typename SoAHelper<ZA_MAKE_INDEX_SEQUENCE(za::rfl::numFields<T>), T>::Type;
