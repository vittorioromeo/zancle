#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Reflection/Rfl.hpp"

#include "Zancle/String/StringView.hpp"

#include "Zancle/Container/Array.hpp"

#include "Zancle/Base/IndexSequence.hpp"
#include "Zancle/Base/MakeIndexSequence.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsArray.hpp"
#include "Zancle/Trait/IsUnion.hpp"


namespace za::rfl::priv
{
////////////////////////////////////////////////////////////
// All field names of `T` are extracted from a single `__PRETTY_FUNCTION__`,
// shaped like:
//
//   Clang: `... Refs = <wImpl.value.f0, wImpl.value.f1, ...>]`
//   GCC:   `... Refs = {wImpl<T>.Wrap<T>::value.T::f0, wImpl<T>.Wrap<T>::value.T::f1, ...}]`
//
// On GCC the type T is rendered inline (e.g. `Members<0, int>`, or
// `{anonymous}::Foo` for types in an unnamed namespace) so segments may
// contain `,`, `<`, `>`, `{`, and `}` at depth > 0. The parser counts both
// `<>` and `{}` as balanced bracket pairs and only treats those characters
// as separators or close markers at depth 0.
//
// On Clang the type and the `<...>` template arguments are elided from the NTTP printout, so
// segments are simple identifier chains -- the bracket tracking is harmless there.
////////////////////////////////////////////////////////////
#if defined(__clang__)

inline constexpr char kOpenMarker[] = "Refs = <";

enum : char
{
    kCloseChar    = '>',
    kNameMarkerCh = '.'
};

enum : SizeT
{
    kNameMarkerLen = 1u
};

#elif defined(__GNUC__)

inline constexpr char kOpenMarker[] = "Refs = {";

enum : char
{
    kCloseChar    = '}',
    kNameMarkerCh = ':'
};

enum : SizeT
{
    kNameMarkerLen = 2u
};

#else
    #error "Zancle reflection field-name extraction is only supported on Clang and GCC"
#endif


////////////////////////////////////////////////////////////
#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wundefined-var-template"
#endif


////////////////////////////////////////////////////////////
// The single NTTP-heavy template. Carrying all field references into one
// `__PRETTY_FUNCTION__` is the whole point; downstream parsing happens in
// non-NTTP code so we do not pay mangling cost for the same pack three times.
////////////////////////////////////////////////////////////
template <auto&... Refs>
[[nodiscard]] consteval const char* getRawSignature() noexcept
{
    return __PRETTY_FUNCTION__;
}


////////////////////////////////////////////////////////////
// The parsing functions below are plain (non-template) `consteval`
// functions: they are compiled once, no matter how many types are
// reflected. Only `computeStoredFieldNames` is instantiated per type.
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// \brief Skip a character or string literal (e.g. in `Tmpl<'{'>`), returning the position past it
///
////////////////////////////////////////////////////////////
[[nodiscard]] consteval const char* skipQuoted(const char* p) noexcept
{
    const char quote = *p++;

    while (*p != quote)
        p += (*p == '\\') ? 2 : 1; // skip escaped characters, including quotes

    return p + 1;
}


////////////////////////////////////////////////////////////
/// \brief Skip a balanced `<...>` or `{...}` group, returning the position past it
///
/// Both pairs are tracked because GCC may emit either inside a segment
/// (e.g. `<TmplArgs>` or `{anonymous}` for unnamed-namespace types).
/// Quoted literals are skipped as a whole, as they may contain brackets.
///
////////////////////////////////////////////////////////////
[[nodiscard]] consteval const char* skipBracketed(const char* p) noexcept
{
    int depth = 0;

    do
    {
        const char c = *p;

        if (c == '\'' || c == '"')
        {
            p = skipQuoted(p);
            continue;
        }

        if (c == '<' || c == '{')
            ++depth;
        else if (c == '>' || c == '}')
            --depth;

        ++p;
    } while (depth > 0);

    return p;
}


////////////////////////////////////////////////////////////
/// \brief Position of the first segment of the reference pack in `sig`
///
////////////////////////////////////////////////////////////
[[nodiscard]] consteval const char* findFirstSegment(const char* p) noexcept
{
    while (true)
    {
        SizeT k = 0u;

        while (kOpenMarker[k] != '\0' && p[k] == kOpenMarker[k])
            ++k;

        if (kOpenMarker[k] == '\0')
            return p + k;

        ++p;
    }
}


////////////////////////////////////////////////////////////
struct NameRange
{
    const char* begin;
    const char* end;
};


////////////////////////////////////////////////////////////
/// \brief Extract the field name of the segment starting at `p`, and advance `p` to the next segment
///
/// The name starts after the rightmost depth-0 `kNameMarkerCh` (".", "::").
///
////////////////////////////////////////////////////////////
[[nodiscard]] consteval NameRange parseNextName(const char*& p) noexcept
{
    const char* nameStart = p;

    while (true)
    {
        const char c = *p;

        if (c == ',' || c == kCloseChar)
            break;

        if (c == kNameMarkerCh)
        {
            nameStart = p + kNameMarkerLen;
            p         = nameStart;
        }
        else if (c == '<' || c == '{')
        {
            p = skipBracketed(p);
        }
        else if (c == '\'' || c == '"')
        {
            p = skipQuoted(p);
        }
        else
        {
            ++p;
        }
    }

    const NameRange result{nameStart, p};

    if (*p == ',')
        p += 2u; // skip ", "

    return result;
}


////////////////////////////////////////////////////////////
/// \brief Locations of the `N` field names within a signature, and their total length
///
////////////////////////////////////////////////////////////
template <SizeT N>
struct FieldNameRanges
{
    NameRange ranges[N];
    SizeT     totalLength;
};


////////////////////////////////////////////////////////////
/// \brief Find the `N` field names in `sig` in a single pass
///
/// Keyed on `N` only: shared by every type with the same field count.
///
////////////////////////////////////////////////////////////
template <SizeT N>
[[nodiscard]] consteval FieldNameRanges<N> findFieldNameRanges(const char* sig) noexcept
{
    FieldNameRanges<N> result{};

    const char* p = findFirstSegment(sig);

    for (SizeT i = 0u; i < N; ++i)
    {
        result.ranges[i] = parseNextName(p);
        result.totalLength += static_cast<SizeT>(result.ranges[i].end - result.ranges[i].begin);
    }

    return result;
}


////////////////////////////////////////////////////////////
template <SizeT N, SizeT TotalChars>
struct PackedFieldNames
{
    // The branch that returns `PackedFieldNames<0, 0>` for empty aggregates is
    // discarded by `if constexpr`, but the type is still instantiated. Clamp
    // the chars array size so `Array<char, 0>` (unsupported) never appears.
    Array<char, (TotalChars == 0u ? 1u : TotalChars)> chars{};
    Array<unsigned short, N + 1u>                     offsets{};
};


////////////////////////////////////////////////////////////
template <typename T, SizeT... Is>
[[nodiscard]] consteval auto computeStoredFieldNames(IndexSequence<Is...>) noexcept
{
    constexpr SizeT n = sizeof...(Is);

    if constexpr (n == 0u)
    {
        return PackedFieldNames<0u, 0u>{};
    }
    else
    {
        // A constexpr lvalue tuple is required so its `.get<I>()` lvalue refs
        // are valid as `auto&` non-type template arguments.
        constexpr auto        fakeTuple = tieAsTuple(getFakeObject<T>());
        constexpr const char* sig       = getRawSignature<fakeTuple.template get<Is>()...>();
        constexpr auto        found     = findFieldNameRanges<n>(sig);

        static_assert(found.totalLength <= 0xFF'FFu, "field names are too long");

        // Exactly sized: only the name characters are stored
        PackedFieldNames<n, found.totalLength> packed{};

        SizeT writeIdx = 0u;

        for (SizeT i = 0u; i < n; ++i)
        {
            packed.offsets[i] = static_cast<unsigned short>(writeIdx);

            for (const char* c = found.ranges[i].begin; c != found.ranges[i].end; ++c)
                packed.chars[writeIdx++] = *c;
        }

        packed.offsets[n] = static_cast<unsigned short>(writeIdx);
        return packed;
    }
}


////////////////////////////////////////////////////////////
template <typename T>
inline constexpr auto storedFieldNames = computeStoredFieldNames<T>(MakeIndexSequence<numFields<T>>{});


////////////////////////////////////////////////////////////
#ifdef __clang__
    #pragma clang diagnostic pop
#endif


////////////////////////////////////////////////////////////
template <SizeT>
using AlwaysStringView = StringView;

} // namespace za::rfl::priv


namespace za::rfl
{
////////////////////////////////////////////////////////////
template <typename T, SizeT I>
constexpr StringView getFieldName() noexcept
{
    static_assert(!ZA_IS_UNION(T), "union reflection is forbidden");
    static_assert(!ZA_IS_ARRAY(T), "impossible to extract name from C-style array");
    static_assert(I < numFields<T>, "field index out of range");

    constexpr const auto& packed = priv::storedFieldNames<T>;

    return StringView{packed.chars.data() + packed.offsets[I],
                      static_cast<SizeT>(packed.offsets[I + 1u] - packed.offsets[I])};
}


////////////////////////////////////////////////////////////
template <typename T>
constexpr auto tieAsFieldNamesTuple() noexcept
{
    static_assert(!ZA_IS_UNION(T), "union reflection is forbidden");
    static_assert(!ZA_IS_ARRAY(T), "impossible to extract name from C-style array");

    return []<SizeT... Is>(IndexSequence<Is...>)
    { return priv::Tuple<priv::AlwaysStringView<Is>...>{getFieldName<T, Is>()...}; }(MakeIndexSequence<numFields<T>>{});
}

} // namespace za::rfl


////////////////////////////////////////////////////////////
namespace za::rfl::priv
{
////////////////////////////////////////////////////////////
// Sanity-check on include.
struct FieldNameSelfCheck
{
    int  alpha;
    char beta;
};

static_assert(getFieldName<FieldNameSelfCheck, 0u>() == StringView{"alpha"});
static_assert(getFieldName<FieldNameSelfCheck, 1u>() == StringView{"beta"});

} // namespace za::rfl::priv


////////////////////////////////////////////////////////////
/// \file
///
/// \brief Compile-time field-name extraction for aggregate types
///
/// Adds `getFieldName<T, I>()` and `tieAsFieldNamesTuple<T>()` to the
/// `za::rfl` core (see `Zancle/Reflection/Rfl.hpp`). Field names are
/// recovered by parsing the compiler's `__PRETTY_FUNCTION__` of a
/// single NTTP-heavy template, with results memoized per type.
///
/// Supports the same types as the core, except:
///
/// - Types with reference members fail to compile: extracting names
///   requires constant references to the fields of a never-created
///   object, and a reference member has nothing to refer to.
/// - C-style arrays as the reflected type are rejected, as their
///   elements have no names.
///
/// Only Clang and GCC are supported.
///
////////////////////////////////////////////////////////////
