#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/String/Utf.hpp"
#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Base/PtrDiffT.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Forward iterator over Unicode codepoints in a `Utf8String`
///
/// Decodes one codepoint per `operator++`. Invalid sequences yield
/// `Utf8String::replacementCodepoint` and consume only their invalid
/// prefix (see `Utf<8>::decode`), so decoding resumes at the next byte
/// that can start a character.
///
////////////////////////////////////////////////////////////
class [[nodiscard]] Utf8String::CodepointIter
{
public:
    ////////////////////////////////////////////////////////////
    // Standard iterator typedefs (excluding `iterator_category`,
    // which would require pulling in `<iterator>`).
    ////////////////////////////////////////////////////////////
    using value_type      = char32_t;
    using reference       = char32_t; //!< Dereference returns by value (proxy iterator)
    using pointer         = const char32_t*;
    using difference_type = za::PtrDiffT;


private:
    ////////////////////////////////////////////////////////////
    const char* m_ptr{};        //!< Start of current codepoint
    const char* m_end{};        //!< One past the last byte
    char32_t    m_current{};    //!< Cached decoded codepoint
    za::SizeT   m_currentLen{}; //!< Byte width of current codepoint


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void decodeAtPtr() noexcept
    {
        if (m_ptr >= m_end)
        {
            m_currentLen = 0u;
            return;
        }

        const char* const next = Utf<8>::decode(m_ptr, m_end, m_current, replacementCodepoint);
        m_currentLen           = static_cast<za::SizeT>(next - m_ptr);
    }


public:
    ////////////////////////////////////////////////////////////
    /// \brief Singular iterator (required by the standard iterator concepts, e.g. for `std::ranges`)
    [[nodiscard]] CodepointIter() noexcept = default;


    ////////////////////////////////////////////////////////////
    CodepointIter(const char* ptr, const char* end) noexcept : m_ptr{ptr}, m_end{end}
    {
        decodeAtPtr();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] char32_t operator*() const noexcept
    {
        return m_current;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] CodepointIter& operator++() noexcept
    {
        m_ptr += m_currentLen;
        decodeAtPtr();
        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] CodepointIter operator++(int) noexcept
    {
        CodepointIter tmp = *this;
        ++*this;
        return tmp;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool operator==(const CodepointIter& rhs) const noexcept
    {
        return m_ptr == rhs.m_ptr;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Pointer to the first byte of the current codepoint
    ///
    /// Useful for slicing: `[i.bytePtr(), j.bytePtr())` is a
    /// well-formed UTF-8 substring of the source buffer.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] const char* bytePtr() const noexcept
    {
        return m_ptr;
    }
};


////////////////////////////////////////////////////////////
/// \brief Forward range view over the codepoints in a `Utf8String`
///
////////////////////////////////////////////////////////////
class [[nodiscard]] Utf8String::CodepointRange
{
private:
    const char* m_begin;
    const char* m_end;


public:
    ////////////////////////////////////////////////////////////
    [[nodiscard]] CodepointRange(const char* b, const char* e) noexcept : m_begin{b}, m_end{e}
    {
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] CodepointIter begin() const noexcept
    {
        return {m_begin, m_end};
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] CodepointIter end() const noexcept
    {
        return {m_end, m_end};
    }
};


////////////////////////////////////////////////////////////
inline za::SizeT Utf8String::codepointCount() const
{
    return Utf<8>::count(m_bytes.data(), m_bytes.data() + m_bytes.size());
}


////////////////////////////////////////////////////////////
inline Utf8String::CodepointRange Utf8String::codepoints() const noexcept
{
    return {m_bytes.data(), m_bytes.data() + m_bytes.size()};
}


////////////////////////////////////////////////////////////
template <typename F>
[[gnu::always_inline]] inline void Utf8String::forCodepoints(F&& fn) const
{
    const char*       ptr    = m_bytes.data();
    const char* const endPtr = ptr + m_bytes.size();

    while (ptr < endPtr)
    {
        char32_t cp{};
        ptr = Utf<8>::decode(ptr, endPtr, cp, replacementCodepoint);
        fn(cp);
    }
}


////////////////////////////////////////////////////////////
inline bool Utf8String::appendCodepoint(char32_t codepoint)
{
    char       buf[4];
    const auto byteCount = priv::encodeCodepointToBuffer(codepoint, buf);

    if (byteCount == 0u)
        return false;

    m_bytes.append(buf, byteCount);
    return true;
}


////////////////////////////////////////////////////////////
inline Utf8String& Utf8String::operator+=(char32_t codepoint)
{
    (void)appendCodepoint(codepoint);
    return *this;
}


////////////////////////////////////////////////////////////
inline Utf8String& Utf8String::operator+=(const char latin1Char)
{
    // Latin-1 characters are the codepoints U+0000..U+00FF
    (void)appendCodepoint(static_cast<unsigned char>(latin1Char));
    return *this;
}

} // namespace za
