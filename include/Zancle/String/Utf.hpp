#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Unreachable.hpp"

#include "Zancle/Trait/IsIntegral.hpp"
#include "Zancle/Trait/MakeUnsigned.hpp"
#include "Zancle/Trait/RemoveCVRef.hpp"


////////////////////////////////////////////////////////////
// References:
//
// https://www.unicode.org/
// https://www.unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.c
// https://www.unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.h
// https://people.w3.org/rishida/scripts/uniview/conversion
//
////////////////////////////////////////////////////////////


namespace za::priv
{
////////////////////////////////////////////////////////////
template <typename In, typename Out>
[[gnu::always_inline]] inline constexpr Out copyBits(In begin, const In end, Out output)
{
    using InputType = ZA_REMOVE_CVREF(decltype(*begin));
    static_assert(ZA_IS_INTEGRAL(InputType));

    using OutputType = typename Out::container_type::value_type;
    static_assert(ZA_IS_INTEGRAL(OutputType));

    static_assert(sizeof(OutputType) >= sizeof(InputType));

    // The goal is to copy the byte representation of the input into the output type.
    // A single static_cast will try to preserve the value as opposed to the byte representation
    // which leads to issues when the input is signed and has a negative value. That will get
    // wrapped to a very large unsigned value which is incorrect. To address this, we first
    // cast the input to its unsigned equivalent then cast that to the destination type which has
    // the property of preserving the byte representation of the input. A simple memcpy seems
    // like a viable solution but copying the bytes of a type into a larger type yields different
    // results on big versus little endian machines so it's not a possibility.
    //
    // Why do this? For example take the Latin1 character é. It has a byte representation of 0xE9
    // and a signed integer value of -23. If you cast -23 to a char32_t, you get a value of
    // 4294967273 which is not a valid Unicode codepoint. What we actually wanted was a char32_t
    // with the byte representation 0x000000E9.
    while (begin != end)
        *output++ = static_cast<OutputType>(static_cast<za::MakeUnsigned<InputType>>(*begin++));

    return output;
}


////////////////////////////////////////////////////////////
/// Number of trailing continuation bytes for each possible UTF-8
/// lead byte. ASCII (`0x00`-`0x7F`) and stray continuation bytes
/// (`0x80`-`0xBF`) map to `0`; lead bytes `0xC0`-`0xDF` map to
/// `1`, `0xE0`-`0xEF` to `2`, etc.
////////////////////////////////////////////////////////////
inline constexpr za::U8 utf8TrailingBytes[256] =
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};


////////////////////////////////////////////////////////////
/// Magic-constant offsets, indexed by `utf8TrailingBytes[leadByte]`,
/// that recover the codepoint value once the input bytes have been
/// folded into a single 32-bit accumulator. Derived from the standard
/// CVTUTF algorithm.
////////////////////////////////////////////////////////////
inline constexpr za::U32 utf8DecodeOffsets[6] =
    {0x00'00'00'00, 0x00'00'30'80, 0x00'0E'20'80, 0x03'C8'20'80, 0xFA'08'20'80, 0x82'08'20'80};


////////////////////////////////////////////////////////////
/// First-byte prefix for an n-byte UTF-8 sequence (indexed by
/// the byte count, 1-4 in practice).
////////////////////////////////////////////////////////////
inline constexpr za::U8 utf8FirstBytes[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};


////////////////////////////////////////////////////////////
template <typename In, typename Facet>
[[nodiscard, gnu::always_inline]] inline char32_t decodeAnsiImpl(In input, const Facet& facet)
{
    return static_cast<char32_t>(facet.widen(input));
}


////////////////////////////////////////////////////////////
template <typename In>
[[nodiscard, gnu::always_inline]] inline char32_t decodeWideImpl(In input)
{
    // wchar_t encoding is platform-defined: UCS-2 on Windows, UCS-4 on Unix.
    // A direct cast works for both (UCS-2 is a subset of UCS-4, UCS-4 *is* UTF-32).
    return static_cast<char32_t>(input);
}


////////////////////////////////////////////////////////////
template <typename Out, typename Facet>
[[gnu::always_inline]] inline Out encodeAnsiImpl(char32_t codepoint, Out output, char replacement, const Facet& facet)
{
    *output++ = facet.narrow(static_cast<wchar_t>(codepoint), replacement);
    return output;
}


////////////////////////////////////////////////////////////
template <typename Out>
[[gnu::always_inline]] inline Out encodeWideImpl(char32_t codepoint, Out output, wchar_t replacement)
{
    // For UCS-4 platforms (Unix), every valid codepoint fits in `wchar_t`.
    // For UCS-2 platforms (Windows), only the BMP fits; non-BMP codepoints
    // (and surrogates themselves) are replaced or dropped.
    if constexpr (sizeof(wchar_t) == 4)
    {
        *output++ = static_cast<wchar_t>(codepoint);
    }
    else
    {
        if ((codepoint <= 0xFF'FF) && ((codepoint < 0xD8'00) || (codepoint > 0xDF'FF)))
            *output++ = static_cast<wchar_t>(codepoint);
        else if (replacement)
            *output++ = replacement;
    }

    return output;
}


////////////////////////////////////////////////////////////
/// \brief Encode a single codepoint into a raw 4-byte buffer
///
/// Returns the number of bytes actually written (1-4), or `0` if
/// the codepoint is invalid (outside Unicode range or a high
/// surrogate). The buffer must have room for at least 4 bytes;
/// only the first `result` bytes are written, the remainder is
/// untouched.
///
/// Used directly by `Utf8String::appendCodepoint` to skip the
/// `BackInserter` per-byte function-call chain when the caller
/// knows the destination is a contiguous byte buffer.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline za::SizeT encodeCodepointToBuffer(char32_t input, char out[4]) noexcept
{
    if ((input > 0x00'10'FF'FF) || ((input >= 0xD8'00) && (input <= 0xDB'FF))) [[unlikely]]
        return 0u; // Invalid codepoint: outside Unicode range or a high surrogate.

    // Compute byte count from codepoint magnitude.
    za::SizeT bytestoWrite = 1;
    if (input < 0x80)
        bytestoWrite = 1;
    else if (input < 0x8'00)
        bytestoWrite = 2;
    else if (input < 0x1'00'00)
        bytestoWrite = 3;
    else if (input <= 0x00'10'FF'FF)
        bytestoWrite = 4;

    // Lay down the bytes from least-significant onwards, then prefix the
    // lead byte. Each continuation byte takes 6 bits of payload.
    // `utf8FirstBytes` lives in `za::priv` (one `.rodata` copy across
    // all instantiations).
    switch (bytestoWrite)
    {
        case 4:
            out[3] = static_cast<char>((input | 0x80) & 0xBF);
            input >>= 6;
            [[fallthrough]];
        case 3:
            out[2] = static_cast<char>((input | 0x80) & 0xBF);
            input >>= 6;
            [[fallthrough]];
        case 2:
            out[1] = static_cast<char>((input | 0x80) & 0xBF);
            input >>= 6;
            [[fallthrough]];
        case 1:
            out[0] = static_cast<char>(input | utf8FirstBytes[bytestoWrite]);
    }

    return bytestoWrite;
}


////////////////////////////////////////////////////////////
/// \brief Raw UTF-8 encoder (used by `za::Utf<8>::encode` and by
///        the UTF-16/UTF-32 → UTF-8 conversion bodies that need
///        to reach across class boundaries).
///
////////////////////////////////////////////////////////////
template <typename Out>
[[gnu::always_inline]] inline Out encodeUtf8Impl(char32_t input, Out output, za::U8 replacement)
{
    char       buf[4];
    const auto bytestoWrite = encodeCodepointToBuffer(input, buf);

    if (bytestoWrite == 0u) [[unlikely]]
    {
        // Invalid codepoint: emit replacement byte if requested, otherwise drop.
        if (replacement)
            *output++ = static_cast<typename Out::container_type::value_type>(replacement);

        return output;
    }

    return copyBits(buf, buf + bytestoWrite, output);
}


////////////////////////////////////////////////////////////
/// \brief Raw UTF-16 encoder (used by `za::Utf<16>::encode` and by
///        UTF-8 → UTF-16 / UTF-32 → UTF-16 conversion bodies).
///
////////////////////////////////////////////////////////////
template <typename Out>
[[gnu::always_inline]] inline Out encodeUtf16Impl(char32_t input, Out output, char16_t replacement)
{
    if (input <= 0xFF'FF)
    {
        if ((input >= 0xD8'00) && (input <= 0xDF'FF))
        {
            // Reserved surrogate range -- not a valid codepoint.
            if (replacement)
                *output++ = replacement;
        }
        else
        {
            *output++ = static_cast<char16_t>(input);
        }
    }
    else if (input > 0x00'10'FF'FF)
    {
        // Above the Unicode maximum.
        if (replacement)
            *output++ = replacement;
    }
    else
    {
        // Encode as a surrogate pair.
        input -= 0x0'01'00'00;
        *output++ = static_cast<char16_t>((input >> 10) + 0xD8'00);
        *output++ = static_cast<char16_t>((input & 0x3'FFUL) + 0xDC'00);
    }

    return output;
}


} // namespace za::priv


namespace za
{
template <unsigned int N>
class Utf;

////////////////////////////////////////////////////////////
/// \brief Specialization of the Utf template for UTF-8
///
////////////////////////////////////////////////////////////
template <>
class Utf<8>
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Decode a single UTF-8 character into its Unicode codepoint
    ///
    /// On an invalid or incomplete sequence, `output` is set to `replacement`.
    ///
    /// **Precondition:** `begin != end`. Callers loop on `begin != end`
    /// (or `begin < end`) before invoking `decode`, so this is always
    /// true at the call site; the existing implementation already
    /// dereferences `*begin` before checking truncation.
    ///
    /// **ASCII fast path:** when the lead byte has the high bit clear,
    /// the codepoint equals the byte value and the iterator advances
    /// by one. UI text is overwhelmingly ASCII (whitespace, digits,
    /// punctuation, Latin letters) even in localized strings, so we
    /// hint the branch and skip the `trailing[]`/`offsets[]` table
    /// loads entirely.
    ///
    /// \return Iterator past the last consumed input element
    ///
    ////////////////////////////////////////////////////////////
    template <typename In>
    [[nodiscard, gnu::always_inline]] static In decode(In begin, In end, char32_t& output, char32_t replacement)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        // ASCII fast path: a leading byte with the high bit clear is a
        // single-byte codepoint that equals the byte value (U+0000..U+007F).
        // No table touch, no truncation check (none possible for 1 byte).
        const auto firstByte = static_cast<za::U8>(*begin);
        if (firstByte < 0x80u) [[likely]]
        {
            output = firstByte;
            return ++begin;
        }

        // Slow path: multi-byte sequence (or stray continuation byte).
        // Tables live in `za::priv` so they're shared across all
        // template instantiations of `decode` (one `.rodata` copy
        // instead of one per `In` type).
        const auto trailingBytes = priv::utf8TrailingBytes[firstByte];

        // The `trailing[]` table only produces values in [0, 5]; assert the
        // invariant so GCC's value-range analysis can prove that the later
        // `offsets[trailingBytes]` access stays in bounds (otherwise
        // `-Warray-bounds` fires after the inline fast path makes the
        // dataflow harder to track across instantiations).
        if (trailingBytes > 5u)
            ZA_UNREACHABLE();

        if (trailingBytes < (end - begin)) [[likely]]
        {
            // Already consumed `firstByte` mentally; fold it in and shift,
            // then add the trailing bytes one at a time. The switch falls
            // through to accumulate (trailingBytes + 1) bytes total.
            output = 0;

            // clang-format off
            switch (trailingBytes)
            {
                case 5: output += static_cast<za::U8>(*begin++); output <<= 6; [[fallthrough]];
                case 4: output += static_cast<za::U8>(*begin++); output <<= 6; [[fallthrough]];
                case 3: output += static_cast<za::U8>(*begin++); output <<= 6; [[fallthrough]];
                case 2: output += static_cast<za::U8>(*begin++); output <<= 6; [[fallthrough]];
                case 1: output += static_cast<za::U8>(*begin++); output <<= 6; [[fallthrough]];
                case 0: output += static_cast<za::U8>(*begin++);
            }
            // clang-format on

            output -= priv::utf8DecodeOffsets[trailingBytes];
        }
        else
        {
            // Incomplete character at end of input -- consume the rest.
            begin  = end;
            output = replacement;
        }

        return begin;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Encode a Unicode codepoint as UTF-8, writing to `output`
    ///
    /// Codepoints not representable in UTF-8 are replaced with `replacement`,
    /// or skipped entirely if `replacement` is `0`.
    ///
    /// \return Iterator past the last written output element
    ///
    ////////////////////////////////////////////////////////////
    template <typename Out>
    [[gnu::always_inline]] static Out encode(char32_t input, Out output, za::U8 replacement)
    {
        return priv::encodeUtf8Impl(input, output, replacement);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Advance past the next UTF-8 character in `[begin, end)`
    ///
    /// A single character may span multiple storage elements.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In>
    [[nodiscard, gnu::always_inline]] static In next(In begin, In end)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        char32_t codepoint = 0;
        return decode(begin, end, codepoint, 0);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Count the number of characters in a UTF-8 sequence
    ///
    /// May differ from `end - begin` since a single character may
    /// span multiple storage elements.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In>
    [[nodiscard, gnu::flatten]] static za::SizeT count(In begin, In end)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        za::SizeT length = 0;
        while (begin != end)
        {
            begin = next(begin, end);
            ++length;
        }

        return length;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert an ANSI sequence `[begin, end)` to UTF-8
    ///
    /// Uses `facet` (typically a locale's `std::ctype<wchar_t>`) to widen
    /// each ANSI character to a Unicode codepoint before encoding as UTF-8.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out, typename Facet>
    [[gnu::always_inline, gnu::flatten]] static Out fromAnsi(In begin, In end, Out output, const Facet& facet)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        while (begin != end)
        {
            const char32_t codepoint = priv::decodeAnsiImpl(*begin++, facet);
            output                   = encode(codepoint, output, 0);
        }

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a wide-character sequence `[begin, end)` to UTF-8
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out fromWide(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(wchar_t));

        while (begin != end)
        {
            const char32_t codepoint = priv::decodeWideImpl(*begin++);
            output                   = encode(codepoint, output, 0);
        }

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a latin-1 (ISO-5589-1) sequence `[begin, end)` to UTF-8
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out fromLatin1(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        // Latin-1 is directly compatible with Unicode encodings,
        // and can thus be treated as (a sub-range of) UTF-32
        while (begin != end)
            output = encode(static_cast<za::U8>(*begin++), output, 0);

        return output;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-8 sequence `[begin, end)` to ANSI
    ///
    /// Uses `facet` to narrow each codepoint to ANSI. Codepoints not
    /// representable are substituted with `replacement`, or skipped if
    /// `replacement` is `0`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out, typename Facet>
    [[gnu::always_inline, gnu::flatten]] static Out toAnsi(In begin, In end, Out output, char replacement, const Facet& facet)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        while (begin != end)
        {
            char32_t codepoint = 0;
            begin              = decode(begin, end, codepoint, 0);
            output             = priv::encodeAnsiImpl(codepoint, output, replacement, facet);
        }

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-8 sequence `[begin, end)` to wide characters
    ///
    /// Codepoints not representable as `wchar_t` are substituted with
    /// `replacement`, or skipped if `replacement` is `0`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out toWide(In begin, In end, Out output, wchar_t replacement)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        while (begin != end)
        {
            char32_t codepoint = 0;
            begin              = decode(begin, end, codepoint, 0);
            output             = priv::encodeWideImpl(codepoint, output, replacement);
        }

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-8 sequence `[begin, end)` to latin-1 (ISO-5589-1)
    ///
    /// Codepoints outside the latin-1 range are substituted with `replacement`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out toLatin1(In begin, In end, Out output, char replacement)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        // Latin-1 is directly compatible with Unicode encodings,
        // and can thus be treated as (a sub-range of) UTF-32
        while (begin != end)
        {
            char32_t codepoint = 0;
            begin              = decode(begin, end, codepoint, 0);
            *output++          = codepoint < 256 ? static_cast<char>(codepoint) : replacement;
        }

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Copy a UTF-8 sequence `[begin, end)` to `output`
    ///
    /// Direct copy; provided so generic code can use the same interface
    /// across all `za::Utf<>` specializations.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline]] static Out toUtf8(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        return priv::copyBits(begin, end, output);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-8 sequence `[begin, end)` to UTF-16
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out toUtf16(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        while (begin != end)
        {
            char32_t codepoint = 0;
            begin              = decode(begin, end, codepoint, 0);
            output             = priv::encodeUtf16Impl(codepoint, output, char16_t{0});
        }

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-8 sequence `[begin, end)` to UTF-32
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out toUtf32(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        while (begin != end)
        {
            char32_t codepoint = 0;
            begin              = decode(begin, end, codepoint, 0);
            *output++          = codepoint;
        }

        return output;
    }
};

////////////////////////////////////////////////////////////
/// \brief Specialization of the Utf template for UTF-16
///
////////////////////////////////////////////////////////////
template <>
class Utf<16>
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Decode a single UTF-16 character into its Unicode codepoint
    ///
    /// Handles surrogate pairs. On an invalid or incomplete sequence,
    /// `output` is set to `replacement`.
    ///
    /// \return Iterator past the last consumed input element
    ///
    ////////////////////////////////////////////////////////////
    template <typename In>
    [[nodiscard, gnu::always_inline]] static In decode(In begin, In end, char32_t& output, char32_t replacement)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char16_t));

        const char16_t first = *begin++;

        // If it's a surrogate pair, first convert to a single UTF-32 character
        if ((first >= 0xD8'00) && (first <= 0xDB'FF))
        {
            if (begin != end)
            {
                const za::U32 second = *begin++;
                if ((second >= 0xDC'00) && (second <= 0xDF'FF))
                {
                    // The second element is valid: convert the two elements to a UTF-32 character
                    output = ((first - 0xD8'00u) << 10) + (second - 0xDC'00) + 0x0'01'00'00;
                }
                else
                {
                    // Invalid character
                    output = replacement;
                }
            }
            else
            {
                // Invalid character
                begin  = end;
                output = replacement;
            }
        }
        else
        {
            // We can make a direct copy
            output = static_cast<char32_t>(first);
        }

        return begin;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Encode a Unicode codepoint as UTF-16, writing to `output`
    ///
    /// Codepoints in the surrogate range or above U+10FFFF are replaced
    /// with `replacement`, or skipped if `replacement` is `0`. Codepoints
    /// above the BMP are emitted as a surrogate pair.
    ///
    /// \return Iterator past the last written output element
    ///
    ////////////////////////////////////////////////////////////
    template <typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out encode(char32_t input, Out output, char16_t replacement)
    {
        return priv::encodeUtf16Impl(input, output, replacement);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Advance past the next UTF-16 character in `[begin, end)`
    ///
    /// A single character may span two storage elements (surrogate pair).
    ///
    ////////////////////////////////////////////////////////////
    template <typename In>
    [[nodiscard, gnu::always_inline]] static In next(In begin, In end)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char16_t));

        char32_t codepoint = 0;
        return decode(begin, end, codepoint, 0);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Count the number of characters in a UTF-16 sequence
    ///
    /// May differ from `end - begin` since surrogate pairs encode a
    /// single character in two storage elements.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In>
    [[nodiscard, gnu::flatten]] static za::SizeT count(In begin, In end)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char16_t));

        za::SizeT length = 0;
        while (begin != end)
        {
            begin = next(begin, end);
            ++length;
        }

        return length;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert an ANSI sequence `[begin, end)` to UTF-16
    ///
    /// Uses `facet` to widen each ANSI character to a Unicode codepoint
    /// before encoding as UTF-16.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out, typename Facet>
    [[gnu::always_inline, gnu::flatten]] static Out fromAnsi(In begin, In end, Out output, const Facet& facet)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        while (begin != end)
        {
            const char32_t codepoint = priv::decodeAnsiImpl(*begin++, facet);
            output                   = encode(codepoint, output, 0);
        }

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a wide-character sequence `[begin, end)` to UTF-16
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out fromWide(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(wchar_t));

        while (begin != end)
        {
            const char32_t codepoint = priv::decodeWideImpl(*begin++);
            output                   = encode(codepoint, output, 0);
        }

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a latin-1 (ISO-5589-1) sequence `[begin, end)` to UTF-16
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out fromLatin1(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        // Latin-1 is directly compatible with Unicode encodings,
        // and can thus be treated as (a sub-range of) UTF-32
        return priv::copyBits(begin, end, output);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-16 sequence `[begin, end)` to ANSI
    ///
    /// Uses `facet` to narrow each codepoint to ANSI. Codepoints not
    /// representable are substituted with `replacement`, or skipped if
    /// `replacement` is `0`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out, typename Facet>
    [[gnu::always_inline, gnu::flatten]] static Out toAnsi(In begin, In end, Out output, char replacement, const Facet& facet)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char16_t));

        while (begin != end)
        {
            char32_t codepoint = 0;
            begin              = decode(begin, end, codepoint, 0);
            output             = priv::encodeAnsiImpl(codepoint, output, replacement, facet);
        }

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-16 sequence `[begin, end)` to wide characters
    ///
    /// Codepoints not representable as `wchar_t` are substituted with
    /// `replacement`, or skipped if `replacement` is `0`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out toWide(In begin, In end, Out output, wchar_t replacement)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char16_t));

        while (begin != end)
        {
            char32_t codepoint = 0;
            begin              = decode(begin, end, codepoint, 0);
            output             = priv::encodeWideImpl(codepoint, output, replacement);
        }

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-16 sequence `[begin, end)` to latin-1 (ISO-5589-1)
    ///
    /// Codepoints outside the latin-1 range are substituted with `replacement`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out toLatin1(In begin, In end, Out output, char replacement)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char16_t));

        // Latin-1 is directly compatible with Unicode encodings,
        // and can thus be treated as (a sub-range of) UTF-32
        while (begin != end)
        {
            *output++ = *begin < 256 ? static_cast<char>(*begin) : replacement;
            ++begin;
        }

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-16 sequence `[begin, end)` to UTF-8
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out toUtf8(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char16_t));

        while (begin != end)
        {
            char32_t codepoint = 0;
            begin              = decode(begin, end, codepoint, 0);
            output             = Utf<8>::encode(codepoint, output, 0);
        }

        return output;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Copy a UTF-16 sequence `[begin, end)` to `output`
    ///
    /// Direct copy; provided so generic code can use the same interface
    /// across all `za::Utf<>` specializations.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline]] static Out toUtf16(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char16_t));

        return priv::copyBits(begin, end, output);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-16 sequence `[begin, end)` to UTF-32
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out toUtf32(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char16_t));

        while (begin != end)
        {
            char32_t codepoint = 0;
            begin              = decode(begin, end, codepoint, 0);
            *output++          = codepoint;
        }

        return output;
    }
};

////////////////////////////////////////////////////////////
/// \brief Specialization of the Utf template for UTF-32
///
////////////////////////////////////////////////////////////
template <>
class Utf<32>
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Decode a single UTF-32 character into its Unicode codepoint
    ///
    /// For UTF-32 the character value is the codepoint, so this is a
    /// direct copy. `replacement` is unused, kept for API consistency.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In>
    [[nodiscard, gnu::always_inline]] static In decode(In                        begin,
                                                       [[maybe_unused]] In       end,
                                                       char32_t&                 output,
                                                       [[maybe_unused]] char32_t replacement)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char32_t));

        output = *begin++;
        return begin;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Encode a Unicode codepoint as UTF-32, writing to `output`
    ///
    /// For UTF-32 the codepoint is the character value, so this is a
    /// direct write. `replacement` is unused, kept for API consistency.
    ///
    ////////////////////////////////////////////////////////////
    template <typename Out>
    [[gnu::always_inline]] static Out encode(char32_t input, Out output, [[maybe_unused]] char32_t replacement)
    {
        *output++ = input;
        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Advance past the next UTF-32 character
    ///
    /// Trivial for UTF-32: one storage element per character.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In>
    [[nodiscard, gnu::always_inline]] static In next(In begin, [[maybe_unused]] In end)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char32_t));

        return ++begin;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Count the number of characters in a UTF-32 sequence
    ///
    /// Trivial for UTF-32: one storage element per character, so this
    /// returns `end - begin`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In>
    [[nodiscard, gnu::always_inline]] static za::SizeT count(In begin, In end)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char32_t));

        return static_cast<za::SizeT>(end - begin);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert an ANSI sequence `[begin, end)` to UTF-32
    ///
    /// Uses `facet` to widen each ANSI character into a Unicode codepoint.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out, typename Facet>
    [[gnu::always_inline, gnu::flatten]] static Out fromAnsi(In begin, In end, Out output, const Facet& facet)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        while (begin != end)
            *output++ = decodeAnsi(*begin++, facet);

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a wide-character sequence `[begin, end)` to UTF-32
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out fromWide(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(wchar_t));

        while (begin != end)
            *output++ = decodeWide(*begin++);

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a latin-1 (ISO-5589-1) sequence `[begin, end)` to UTF-32
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out fromLatin1(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char));

        // Latin-1 is directly compatible with Unicode encodings,
        // and can thus be treated as (a sub-range of) UTF-32
        return priv::copyBits(begin, end, output);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-32 sequence `[begin, end)` to ANSI
    ///
    /// Uses `facet` to narrow each codepoint to ANSI. Codepoints not
    /// representable are substituted with `replacement`, or skipped if
    /// `replacement` is `0`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out, typename Facet>
    [[gnu::always_inline, gnu::flatten]] static Out toAnsi(In begin, In end, Out output, char replacement, const Facet& facet)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char32_t));

        while (begin != end)
            output = encodeAnsi(*begin++, output, replacement, facet);

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-32 sequence `[begin, end)` to wide characters
    ///
    /// Codepoints not representable as `wchar_t` are substituted with
    /// `replacement`, or skipped if `replacement` is `0`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out toWide(In begin, In end, Out output, wchar_t replacement)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char32_t));

        while (begin != end)
            output = encodeWide(*begin++, output, replacement);

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-32 sequence `[begin, end)` to latin-1 (ISO-5589-1)
    ///
    /// Codepoints outside the latin-1 range are substituted with `replacement`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out toLatin1(In begin, In end, Out output, char replacement)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char32_t));

        // Latin-1 is directly compatible with Unicode encodings,
        // and can thus be treated as (a sub-range of) UTF-32
        while (begin != end)
        {
            *output++ = *begin < 256 ? static_cast<char>(*begin) : replacement;
            ++begin;
        }

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-32 sequence `[begin, end)` to UTF-8
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out toUtf8(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char32_t));

        while (begin != end)
            output = Utf<8>::encode(*begin++, output, 0);

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Convert a UTF-32 sequence `[begin, end)` to UTF-16
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out toUtf16(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char32_t));

        while (begin != end)
            output = Utf<16>::encode(*begin++, output, 0);

        return output;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Copy a UTF-32 sequence `[begin, end)` to `output`
    ///
    /// Direct copy; provided so generic code can use the same interface
    /// across all `za::Utf<>` specializations.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Out>
    [[gnu::always_inline]] static Out toUtf32(In begin, In end, Out output)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char32_t));

        return priv::copyBits(begin, end, output);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Decode a single ANSI character to a UTF-32 codepoint
    ///
    /// Thin forwarder around `priv::decodeAnsiImpl` kept for API
    /// compatibility.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In, typename Facet>
    [[nodiscard, gnu::always_inline]] static char32_t decodeAnsi(In input, const Facet& facet)
    {
        return priv::decodeAnsiImpl(input, facet);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Decode a single wide character to a UTF-32 codepoint
    ///
    /// Thin forwarder around `priv::decodeWideImpl` kept for API
    /// compatibility.
    ///
    ////////////////////////////////////////////////////////////
    template <typename In>
    [[nodiscard, gnu::always_inline]] static char32_t decodeWide(In input)
    {
        return priv::decodeWideImpl(input);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Encode a single UTF-32 codepoint as ANSI
    ///
    /// Thin forwarder around `priv::encodeAnsiImpl` kept for API
    /// compatibility.
    ///
    ////////////////////////////////////////////////////////////
    template <typename Out, typename Facet>
    [[gnu::always_inline, gnu::flatten]] static Out encodeAnsi(char32_t codepoint, Out output, char replacement, const Facet& facet)
    {
        return priv::encodeAnsiImpl(codepoint, output, replacement, facet);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Encode a single UTF-32 codepoint as a wide character
    ///
    /// Thin forwarder around `priv::encodeWideImpl` kept for API
    /// compatibility.
    ///
    ////////////////////////////////////////////////////////////
    template <typename Out>
    [[gnu::always_inline, gnu::flatten]] static Out encodeWide(char32_t codepoint, Out output, wchar_t replacement)
    {
        return priv::encodeWideImpl(codepoint, output, replacement);
    }
};


////////////////////////////////////////////////////////////
// Make type aliases to get rid of the template syntax
using Utf8  = Utf<8>;
using Utf16 = Utf<16>;
using Utf32 = Utf<32>;

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::Utf
/// \ingroup system
///
/// Low-level generic interface for counting, iterating, encoding and
/// decoding Unicode characters across ANSI, wide, latin-1, UTF-8,
/// UTF-16 and UTF-32. All members are static templates and the class
/// is not meant to be instantiated.
///
/// Specializations: `za::Utf<8>` / `za::Utf8`, `za::Utf<16>` / `za::Utf16`,
/// `za::Utf<32>` / `za::Utf32`.
///
////////////////////////////////////////////////////////////
