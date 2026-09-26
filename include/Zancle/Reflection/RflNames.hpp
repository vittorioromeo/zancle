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
// Loose scratch buffer used during parsing. Tightened to exact size in
// `computeStoredFieldNames` so end users only pay for the actual bytes.
////////////////////////////////////////////////////////////
template <SizeT N>
struct RawFieldNames
{
    Array<char, 4096u>            chars{};
    Array<unsigned short, N + 1u> offsets{};
    SizeT                         total{};
};


////////////////////////////////////////////////////////////
// Walks the signature once, char-by-char, tracking `<>` depth so that commas
// inside template arguments are not mistaken for segment separators. Within
// each segment, the rightmost depth-0 occurrence of `kNameMarkerCh` (".",
// "::") marks the start of the field name.
//
// `parseRawNames` is keyed on `N` only -- it is shared across every type that
// has the same field count, so the heavy compile-time work happens once per
// distinct field count rather than once per distinct type.
////////////////////////////////////////////////////////////
template <SizeT N>
[[nodiscard]] consteval RawFieldNames<N> parseRawNames(const char* sig) noexcept
{
    RawFieldNames<N> r{};

    if constexpr (N == 0u)
    {
        return r;
    }
    else
    {
        const char* p = sig;

        // Locate the open marker at the start of the pack body.
        while (true)
        {
            bool match = true;

            for (SizeT k = 0u; kOpenMarker[k] != '\0'; ++k)
            {
                if (p[k] != kOpenMarker[k])
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                p += sizeof(kOpenMarker) - 1u;
                break;
            }

            ++p;
        }

        SizeT writeIdx = 0u;
        for (SizeT i = 0u; i < N; ++i)
        {
            r.offsets[i] = static_cast<unsigned short>(writeIdx);

            const char* nameStart = p;

            // Hot path at depth 0: find the next segment terminator while
            // tracking the rightmost name marker.
            while (true)
            {
                const char c = *p;

                if (c == ',' || c == kCloseChar)
                    break;

                if (c == kNameMarkerCh)
                {
                    nameStart = p + kNameMarkerLen;
                    p         = nameStart;
                    continue;
                }

                if (c == '<' || c == '{')
                {
                    // Depth > 0: skip to matching close. Both `<>` and `{}`
                    // are tracked as balanced pairs because GCC may emit
                    // either inside a segment (e.g. `<TmplArgs>` or
                    // `{anonymous}` for unnamed-namespace types).
                    int depth = 1;
                    ++p;

                    while (depth > 0)
                    {
                        const char d = *p;

                        if (d == '<' || d == '{')
                            ++depth;
                        else if (d == '>' || d == '}')
                            --depth;

                        ++p;
                    }

                    continue;
                }

                ++p;
            }

            for (const char* c = nameStart; c != p; ++c)
                r.chars[writeIdx++] = *c;

            if (*p == ',')
                p += 2u; // skip ", "
        }

        r.offsets[N] = static_cast<unsigned short>(writeIdx);
        r.total      = writeIdx;

        return r;
    }
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
        constexpr auto        raw       = parseRawNames<n>(sig);

        PackedFieldNames<n, raw.total> packed{};

        for (SizeT i = 0u; i < raw.total; ++i)
            packed.chars[i] = raw.chars[i];

        for (SizeT i = 0u; i <= n; ++i)
            packed.offsets[i] = raw.offsets[i];

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
/// Unions and C-style array types are explicitly rejected.
///
////////////////////////////////////////////////////////////
