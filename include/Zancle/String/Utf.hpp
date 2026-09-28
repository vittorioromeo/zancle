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
/// \brief Codepoint used internally to mark invalid input
///
/// Above U+10FFFF, so every encoder rejects it: conversions taking a
/// `replacement` output that replacement instead (or nothing, if it is
/// `0`), exactly as for valid but unrepresentable codepoints.
///
////////////////////////////////////////////////////////////
inline constexpr char32_t invalidCodepoint = 0xFF'FF'FF'FFu;


////////////////////////////////////////////////////////////
/// \brief U+FFFD REPLACEMENT CHARACTER
///
/// Output for invalid input by conversions to UTF-8/16/32, which have
/// no `replacement` parameter.
///
////////////////////////////////////////////////////////////
inline constexpr char32_t replacementCharacter = 0xFF'FDu;


////////////////////////////////////////////////////////////
/// \brief Whether `codepoint` is a Unicode scalar value: at most U+10FFFF, and not a surrogate
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr bool isValidCodepoint(const char32_t codepoint) noexcept
{
    return codepoint <= 0x10'FF'FFu && (codepoint & 0xFF'FF'F8'00u) != 0xD8'00u;
}


////////////////////////////////////////////////////////////
/// \brief Element type written through the output iterator `Out`
///
/// Inserters (e.g. `std::back_insert_iterator`) expose it through
/// `container_type`, raw pointers through their pointee type.
///
////////////////////////////////////////////////////////////
template <typename Out>
struct UtfOutputElement
{
    using type = typename Out::container_type::value_type;
};


////////////////////////////////////////////////////////////
template <typename T>
struct UtfOutputElement<T*>
{
    using type = T;
};


////////////////////////////////////////////////////////////
template <typename Out>
using UtfOutputElementType = typename UtfOutputElement<Out>::type;


////////////////////////////////////////////////////////////
template <typename In, typename Out>
[[gnu::always_inline]] inline constexpr Out copyBits(In begin, const In end, Out output)
{
    using InputType = ZA_REMOVE_CVREF(decltype(*begin));
    static_assert(ZA_IS_INTEGRAL(InputType));

    using OutputType = UtfOutputElementType<Out>;
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
/// First-byte prefix for an n-byte UTF-8 sequence (indexed by the byte count, 1-4).
///
////////////////////////////////////////////////////////////
inline constexpr za::U8 utf8FirstBytes[5] = {0x00, 0x00, 0xC0, 0xE0, 0xF0};


////////////////////////////////////////////////////////////
/// \brief Decode one UTF-16 character (possibly a surrogate pair) from a non-empty `[begin, end)`
///
/// Unpaired surrogates yield `replacement`. A high surrogate followed by
/// anything but a low surrogate consumes only itself, so that the next
/// unit is decoded on its own (instead of being swallowed).
///
/// Also used for `wchar_t` sequences on platforms where `wchar_t` is 16-bit (UTF-16).
///
////////////////////////////////////////////////////////////
template <typename In>
[[nodiscard, gnu::always_inline]] inline In decodeUtf16Impl(In begin, const In end, char32_t& output, const char32_t replacement)
{
    const auto first = static_cast<za::U32>(static_cast<char16_t>(*begin++));

    if ((first & 0xFC'00u) == 0xD8'00u) // High surrogate: must be followed by a low surrogate
    {
        if (begin != end)
        {
            const auto second = static_cast<za::U32>(static_cast<char16_t>(*begin));

            if ((second & 0xFC'00u) == 0xDC'00u)
            {
                ++begin;
                output = ((first - 0xD8'00u) << 10) + (second - 0xDC'00u) + 0x1'00'00u;
                return begin;
            }
        }

        output = replacement;
        return begin;
    }

    // A low surrogate cannot come first
    output = (first & 0xFC'00u) == 0xDC'00u ? replacement : static_cast<char32_t>(first);
    return begin;
}


////////////////////////////////////////////////////////////
template <typename In, typename Facet>
[[nodiscard, gnu::always_inline]] inline char32_t decodeAnsiImpl(In input, const Facet& facet)
{
    return static_cast<char32_t>(facet.widen(input));
}


////////////////////////////////////////////////////////////
/// \brief Decode a single `wchar_t` unit, without combining UTF-16 surrogate pairs
///
////////////////////////////////////////////////////////////
template <typename In>
[[nodiscard, gnu::always_inline]] inline char32_t decodeWideUnitImpl(In input)
{
    return static_cast<char32_t>(input);
}


////////////////////////////////////////////////////////////
/// \brief Decode one character from a non-empty `wchar_t` sequence
///
/// `wchar_t` holds UTF-16 on platforms where it is 16-bit (Windows), and
/// UTF-32 where it is 32-bit. Invalid input yields `replacement`.
///
////////////////////////////////////////////////////////////
template <typename In>
[[nodiscard, gnu::always_inline]] inline In decodeWideImpl(In begin, const In end, char32_t& output, const char32_t replacement)
{
    if constexpr (sizeof(wchar_t) == 2)
    {
        return decodeUtf16Impl(begin, end, output, replacement);
    }
    else
    {
        const auto codepoint = static_cast<char32_t>(*begin++);
        output               = isValidCodepoint(codepoint) ? codepoint : replacement;
        return begin;
    }
}


////////////////////////////////////////////////////////////
/// \brief Narrow a codepoint to ANSI through `facet`
///
/// Codepoints that are invalid, or that `wchar_t` cannot hold (above
/// U+FFFF where it is 16-bit), are substituted with `replacement` rather
/// than truncated into another character. Unrepresentable codepoints are
/// skipped if `replacement` is `0`.
///
////////////////////////////////////////////////////////////
template <typename Out, typename Facet>
[[gnu::always_inline]] inline Out encodeAnsiImpl(const char32_t codepoint, Out output, const char replacement, const Facet& facet)
{
    const bool fitsInWchar = isValidCodepoint(codepoint) && (sizeof(wchar_t) == 4 || codepoint <= 0xFF'FFu);
    const char narrowed    = fitsInWchar ? facet.narrow(static_cast<wchar_t>(codepoint), replacement) : replacement;

    // `narrow` returns `replacement` for unrepresentable characters (a genuine U+0000 is kept)
    if (narrowed != '\0' || codepoint == 0u)
        *output++ = narrowed;

    return output;
}


////////////////////////////////////////////////////////////
/// \brief Encode a single codepoint into a raw 4-byte buffer
///
/// Returns the number of bytes actually written (1-4), or `0` if
/// the codepoint is invalid (outside Unicode range or a surrogate).
/// The buffer must have room for at least 4 bytes; only the first
/// `result` bytes are written, the remainder is untouched.
///
/// Used directly by `Utf8String::appendCodepoint` to skip the
/// `BackInserter` per-byte function-call chain when the caller
/// knows the destination is a contiguous byte buffer.
///
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline za::SizeT encodeCodepointToBuffer(char32_t input, char out[4]) noexcept
{
    if (!isValidCodepoint(input)) [[unlikely]]
        return 0u;

    const za::SizeT bytesToWrite = input < 0x80u ? 1u : input < 0x8'00u ? 2u : input < 0x1'00'00u ? 3u : 4u;

    // Lay down the bytes from least-significant onwards, then prefix the
    // lead byte. Each continuation byte takes 6 bits of payload.
    switch (bytesToWrite)
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
            out[0] = static_cast<char>(input | utf8FirstBytes[bytesToWrite]);
    }

    return bytesToWrite;
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
    const auto bytesToWrite = encodeCodepointToBuffer(input, buf);

    if (bytesToWrite == 0u) [[unlikely]]
    {
        // Invalid codepoint: emit replacement byte if requested, otherwise drop.
        if (replacement)
            *output++ = static_cast<UtfOutputElementType<Out>>(replacement);

        return output;
    }

    return copyBits(buf, buf + bytesToWrite, output);
}


////////////////////////////////////////////////////////////
/// \brief Raw UTF-16 encoder (used by `za::Utf<16>::encode`, by
///        UTF-8 → UTF-16 / UTF-32 → UTF-16 conversion bodies, and for
///        16-bit `wchar_t`).
///
////////////////////////////////////////////////////////////
template <typename Out>
[[gnu::always_inline]] inline Out encodeUtf16Impl(char32_t input, Out output, char16_t replacement)
{
    if (!isValidCodepoint(input))
    {
        // Surrogate, or above the Unicode maximum.
        if (replacement)
            *output++ = replacement;
    }
    else if (input <= 0xFF'FF)
    {
        *output++ = static_cast<char16_t>(input);
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


////////////////////////////////////////////////////////////
/// \brief Encode a codepoint as `wchar_t`: UTF-16 where it is 16-bit (Windows), UTF-32 where it is 32-bit
///
/// Invalid codepoints are replaced with `replacement`, or dropped if it is `0`.
///
////////////////////////////////////////////////////////////
template <typename Out>
[[gnu::always_inline]] inline Out encodeWideImpl(char32_t codepoint, Out output, wchar_t replacement)
{
    if constexpr (sizeof(wchar_t) == 2)
    {
        return encodeUtf16Impl(codepoint, output, static_cast<char16_t>(replacement));
    }
    else
    {
        if (isValidCodepoint(codepoint))
            *output++ = static_cast<wchar_t>(codepoint);
        else if (replacement)
            *output++ = replacement;

        return output;
    }
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
    /// Only well-formed UTF-8 is accepted: overlong encodings, encoded
    /// surrogates, codepoints above U+10FFFF, stray continuation bytes, and
    /// truncated sequences all set `output` to `replacement`. Following the
    /// Unicode "maximal subpart" practice, an invalid sequence consumes only
    /// its valid prefix (at least one byte), so a valid character that
    /// follows it (e.g. the `'A'` in `C3 41`) is decoded on its own.
    ///
    /// **Precondition:** `begin != end`. Callers loop on `begin != end`
    /// (or `begin < end`) before invoking `decode`, so this is always
    /// true at the call site; the existing implementation already
    /// dereferences `*begin` before checking truncation.
    ///
    /// **ASCII fast path:** when the lead byte has the high bit clear,
    /// the codepoint equals the byte value and the iterator advances
    /// by one. UI text is overwhelmingly ASCII (whitespace, digits,
    /// punctuation, Latin letters) even in localized strings, so the
    /// branch is hinted as likely.
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
        const auto firstByte = static_cast<za::U8>(*begin++);
        if (firstByte < 0x80u) [[likely]]
        {
            output = firstByte;
            return begin;
        }

        // Well-formed multi-byte sequences (Unicode Table 3-7). The valid range of the
        // second byte depends on the lead byte: this rules out overlong encodings,
        // surrogates (U+D800..U+DFFF), and codepoints above U+10FFFF.
        za::SizeT length;
        za::U8    low  = 0x80u; // valid range of the next continuation byte
        za::U8    high = 0xBFu;

        if (firstByte >= 0xC2u && firstByte <= 0xDFu)
        {
            length = 2u;
            output = firstByte & 0x1Fu;
        }
        else if (firstByte >= 0xE0u && firstByte <= 0xEFu)
        {
            length = 3u;
            output = firstByte & 0x0Fu;

            if (firstByte == 0xE0u)
                low = 0xA0u; // overlong
            else if (firstByte == 0xEDu)
                high = 0x9Fu; // surrogates
        }
        else if (firstByte >= 0xF0u && firstByte <= 0xF4u)
        {
            length = 4u;
            output = firstByte & 0x07u;

            if (firstByte == 0xF0u)
                low = 0x90u; // overlong
            else if (firstByte == 0xF4u)
                high = 0x8Fu; // above U+10FFFF
        }
        else
        {
            // Stray continuation byte (0x80..0xBF), overlong lead (0xC0, 0xC1), or no longer valid lead (0xF5..0xFF)
            output = replacement;
            return begin;
        }

        for (za::SizeT i = 1u; i < length; ++i)
        {
            // Truncated or broken sequence: replace the maximal invalid subpart consumed so far,
            // without consuming the offending byte (which may start the next valid character)
            if (begin == end)
            {
                output = replacement;
                return begin;
            }

            const auto byte = static_cast<za::U8>(*begin);
            if (byte < low || byte > high)
            {
                output = replacement;
                return begin;
            }

            output = (output << 6) | (byte & 0x3Fu);
            ++begin;

            low  = 0x80u;
            high = 0xBFu;
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
            char32_t codepoint = 0;
            begin              = priv::decodeWideImpl(begin, end, codepoint, priv::replacementCharacter);
            output             = encode(codepoint, output, 0);
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
            begin              = decode(begin, end, codepoint, priv::invalidCodepoint);
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
            begin              = decode(begin, end, codepoint, priv::invalidCodepoint);
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
            begin              = decode(begin, end, codepoint, priv::invalidCodepoint);
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
            begin              = decode(begin, end, codepoint, priv::replacementCharacter);
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
            begin              = decode(begin, end, codepoint, priv::replacementCharacter);
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
    /// Handles surrogate pairs. Unpaired surrogates set `output` to
    /// `replacement`; a high surrogate that is not followed by a low
    /// surrogate consumes only itself.
    ///
    /// \return Iterator past the last consumed input element
    ///
    ////////////////////////////////////////////////////////////
    template <typename In>
    [[nodiscard, gnu::always_inline]] static In decode(In begin, In end, char32_t& output, char32_t replacement)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char16_t));

        return priv::decodeUtf16Impl(begin, end, output, replacement);
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
            char32_t codepoint = 0;
            begin              = priv::decodeWideImpl(begin, end, codepoint, priv::replacementCharacter);
            output             = encode(codepoint, output, 0);
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
            begin              = decode(begin, end, codepoint, priv::invalidCodepoint);
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
            begin              = decode(begin, end, codepoint, priv::invalidCodepoint);
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

        // Latin-1 is directly compatible with Unicode encodings, and can thus be treated
        // as (a sub-range of) UTF-32. Decode whole codepoints, so that a surrogate pair
        // yields a single replacement.
        while (begin != end)
        {
            char32_t codepoint = 0;
            begin              = decode(begin, end, codepoint, priv::invalidCodepoint);
            *output++          = codepoint < 256 ? static_cast<char>(codepoint) : replacement;
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
            begin              = decode(begin, end, codepoint, priv::replacementCharacter);
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
            begin              = decode(begin, end, codepoint, priv::replacementCharacter);
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
    [[nodiscard, gnu::always_inline]] static In decode(In begin, [[maybe_unused]] In end, char32_t& output, char32_t replacement)
    {
        static_assert(sizeof(decltype(*begin)) == sizeof(char32_t));

        const auto codepoint = static_cast<char32_t>(*begin++);
        output               = priv::isValidCodepoint(codepoint) ? codepoint : replacement;
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
    [[gnu::always_inline]] static Out encode(char32_t input, Out output, char32_t replacement)
    {
        if (priv::isValidCodepoint(input))
            *output++ = input;
        else if (replacement)
            *output++ = replacement;

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
        {
            char32_t codepoint = 0;
            begin              = priv::decodeWideImpl(begin, end, codepoint, priv::replacementCharacter);
            *output++          = codepoint;
        }

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
        {
            char32_t codepoint = 0;
            begin              = decode(begin, end, codepoint, priv::replacementCharacter);
            output             = Utf<8>::encode(codepoint, output, 0);
        }

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
        {
            char32_t codepoint = 0;
            begin              = decode(begin, end, codepoint, priv::replacementCharacter);
            output             = Utf<16>::encode(codepoint, output, 0);
        }

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
        return priv::decodeWideUnitImpl(input);
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
