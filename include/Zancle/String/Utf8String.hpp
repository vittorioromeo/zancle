#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Fmt/FmtAppendMixinFwd.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/StringView.hpp"

#include "Zancle/Base/LifetimeAttributes.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Swap.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Owning UTF-8 string with codepoint iteration
///
/// Wraps a `za::String` (raw UTF-8 byte storage) and exposes a
/// `codepoints()` range that decodes one Unicode codepoint per
/// step. Use this type wherever the API contract is explicitly
/// "human-readable text encoded as UTF-8".
///
/// For arbitrary byte data, use `za::String` directly. For
/// per-codepoint random access, decode once into a contiguous
/// `char32_t` buffer.
///
/// Inherits `za::FmtAppendMixin`, which exposes `.appendFmt(fmt, args...)`
/// for formatted append. Include `<Zancle/Fmt/FmtAppendMixin.hpp>`
/// at the call site to bring in the template body.
///
////////////////////////////////////////////////////////////
class [[nodiscard]] ZA_GSL_OWNER(char) Utf8String : public za::FmtAppendMixin
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Type of the stored UTF-8 code units (e.g. for `Utf<X>::toUtf8` via `za::BackInserter`)
    ///
    ////////////////////////////////////////////////////////////
    using value_type = char;


    ////////////////////////////////////////////////////////////
    /// \brief Replacement codepoint for invalid/truncated UTF-8 sequences
    ///
    ////////////////////////////////////////////////////////////
    static constexpr char32_t replacementCodepoint = 0xFF'FDu;


    ////////////////////////////////////////////////////////////
    /// \brief Forward iterator over Unicode codepoints
    ///
    /// Full definition lives in `Utf8StringCodepoints.hpp`
    /// to keep the codepoint-decoding template machinery out of TUs
    /// that only need to pass `Utf8String` around as a byte container.
    ///
    ////////////////////////////////////////////////////////////
    class CodepointIter;

    ////////////////////////////////////////////////////////////
    /// \brief Forward range view over the codepoints in a `Utf8String`
    ///
    /// \see `CodepointIter`
    ///
    ////////////////////////////////////////////////////////////
    class CodepointRange;


    ////////////////////////////////////////////////////////////
    /// \brief Construct an empty UTF-8 string
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Utf8String() = default;


    ////////////////////////////////////////////////////////////
    /// \brief Disallow construction from `nullptr` literal
    ///
    ////////////////////////////////////////////////////////////
    Utf8String(decltype(nullptr)) = delete;


    ////////////////////////////////////////////////////////////
    /// \brief Construct from a null-terminated UTF-8 C string
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] /* implicit */ Utf8String(const char* utf8CStr) : m_bytes{utf8CStr}
    {
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct from a pointer to UTF-8 bytes and an explicit byte count
    ///
    /// `utf8Bytes` does not need to be null-terminated; only `byteCount`
    /// bytes are read. Useful when the source already provides a length
    /// alongside a pointer (e.g. SDL queries that pre-compute `strlen`,
    /// fixed-size buffers, slices).
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit Utf8String(const char* utf8Bytes, za::SizeT byteCount) : m_bytes{utf8Bytes, byteCount}
    {
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct from a null-terminated `u8"..."` literal
    ///
    /// In C++20, `u8"..."` literals have type `const char8_t[N]`
    /// rather than `const char[N]`. `char8_t` has the same size,
    /// alignment, and representation as `char` (it's an unsigned
    /// version of `char` reserved for UTF-8 text), so reinterpreting
    /// the pointer is safe and lets call sites write
    /// `Utf8String s = u8"カタツムリ";` instead of escape sequences.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] /* implicit */ Utf8String(const char8_t* utf8CStr) : m_bytes{reinterpret_cast<const char*>(utf8CStr)}
    {
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct from a `u8` pointer + byte count
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit Utf8String(const char8_t* utf8Bytes, za::SizeT byteCount) :
        m_bytes{reinterpret_cast<const char*>(utf8Bytes), byteCount}
    {
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct from a UTF-8 byte view
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] /* implicit */ Utf8String(za::StringView utf8View) : m_bytes{utf8View}
    {
    }


    ////////////////////////////////////////////////////////////
    /// \brief Take ownership of an existing UTF-8 byte buffer
    ///
    /// The bytes are assumed to already be UTF-8 encoded; no
    /// validation is performed.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] /* implicit */ Utf8String(za::String utf8Bytes) noexcept : m_bytes{ZA_MOVE(utf8Bytes)}
    {
    }


    ////////////////////////////////////////////////////////////
    /// \brief Implicit conversion to a `za::StringView` over the UTF-8 bytes
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] /* implicit */ operator za::StringView() const noexcept ZA_LIFETIMEBOUND
    {
        return m_bytes;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Number of bytes in the underlying UTF-8 buffer
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::SizeT byteSize() const noexcept
    {
        return m_bytes.size();
    }


    ////////////////////////////////////////////////////////////
    /// \brief `true` if the underlying UTF-8 buffer holds zero bytes
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool empty() const noexcept
    {
        return m_bytes.empty();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Pointer to a null-terminated UTF-8 C string
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const char* cStr() const noexcept ZA_LIFETIMEBOUND
    {
        return m_bytes.cStr();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Pointer to the first byte of the UTF-8 buffer (not necessarily null-terminated)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const char* data() const noexcept ZA_LIFETIMEBOUND
    {
        return m_bytes.data();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Number of Unicode codepoints in the string
    ///
    /// Linear in the byte length (decodes the buffer).
    ///
    /// \note Defined in `Utf8StringCodepoints.hpp`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::SizeT codepointCount() const;


    ////////////////////////////////////////////////////////////
    /// \brief Forward range over the codepoints in this string
    ///
    /// \note Defined in `Utf8StringCodepoints.hpp`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] CodepointRange codepoints() const noexcept ZA_LIFETIMEBOUND;


    ////////////////////////////////////////////////////////////
    /// \brief Invoke `fn` for every codepoint in the string
    ///
    /// Tighter than `for (cp : codepoints())` for performance-
    /// sensitive loops: no iterator object, no cached state, one
    /// `Utf<8>::decode` call per codepoint, single `ptr < end`
    /// loop condition.
    ///
    /// \note Defined in `Utf8StringCodepoints.hpp`;
    ///       that header must be included at the call site.
    ///
    ////////////////////////////////////////////////////////////
    template <typename F>
    void forCodepoints(F&& fn) const;


    ////////////////////////////////////////////////////////////
    /// \brief Reserve byte capacity to avoid reallocations
    ///
    /// Capacity is in bytes (not codepoints). For a worst-case
    /// upper bound, reserve `4 * expectedCodepointCount`.
    ///
    ////////////////////////////////////////////////////////////
    void reserve(za::SizeT byteCapacity)
    {
        m_bytes.reserve(byteCapacity);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Append a single codepoint, encoded as UTF-8
    ///
    /// \return `true` if the codepoint was a valid Unicode scalar
    ///         value and was appended; `false` if it was rejected
    ///         (lone surrogate `U+D800..U+DFFF`, or above `U+10FFFF`)
    ///         and the buffer is unchanged.
    ///
    /// Use `operator+=(char32_t)` if you want chaining and don't
    /// care about validity reporting.
    ///
    /// \note Defined in `Utf8StringCodepoints.hpp`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool appendCodepoint(char32_t codepoint);


    ////////////////////////////////////////////////////////////
    /// \brief Append a single raw UTF-8 byte
    ///
    /// `pushBack` is **byte-oriented**, primarily provided so
    /// `Utf8String` can act as a sink for `Utf<X>::toUtf8(...)` via
    /// `za::BackInserter`. Pushing only the leading byte of a
    /// multi-byte sequence (or, conversely, a continuation byte
    /// without its leader) leaves the buffer in an invariant-
    /// violating state that the codepoint iterator will treat as
    /// truncated input. Callers are responsible for completing
    /// every multi-byte sequence before the next `codepoints()`
    /// traversal.
    ///
    ////////////////////////////////////////////////////////////
    void pushBack(char byte)
    {
        m_bytes.pushBack(byte);
    }


    ////////////////////////////////////////////////////////////
    Utf8String& append(const Utf8String& other)
    {
        m_bytes.append(other.m_bytes);
        return *this;
    }


    ////////////////////////////////////////////////////////////
    Utf8String& append(za::StringView utf8View)
    {
        m_bytes.append(utf8View);
        return *this;
    }


    ////////////////////////////////////////////////////////////
    Utf8String& append(const char* utf8CStr)
    {
        m_bytes.append(utf8CStr);
        return *this;
    }


    ////////////////////////////////////////////////////////////
    Utf8String& append(const char* utf8CStr, za::SizeT byteCount)
    {
        m_bytes.append(utf8CStr, byteCount);
        return *this;
    }


    ////////////////////////////////////////////////////////////
    Utf8String& operator+=(const Utf8String& rhs)
    {
        m_bytes += rhs.m_bytes;
        return *this;
    }


    ////////////////////////////////////////////////////////////
    Utf8String& operator+=(za::StringView utf8View)
    {
        m_bytes += utf8View;
        return *this;
    }


    ////////////////////////////////////////////////////////////
    Utf8String& operator+=(const char* utf8CStr)
    {
        m_bytes += utf8CStr;
        return *this;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Append a single codepoint, encoded as UTF-8
    ///
    /// Equivalent to `appendCodepoint(cp)` but discards the validity
    /// result so `text += someCodepoint` reads naturally and chains.
    /// Invalid codepoints are silently dropped; use `appendCodepoint`
    /// if you need to detect that case.
    ///
    /// \note Defined in `Utf8StringCodepoints.hpp`.
    ///
    ////////////////////////////////////////////////////////////
    Utf8String& operator+=(char32_t codepoint);


    ////////////////////////////////////////////////////////////
    void clear() noexcept
    {
        m_bytes.clear();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Find the first byte offset of the substring `needle`
    ///
    /// UTF-8 has the property that no codepoint encoding is a
    /// substring of another, so byte-level search returns
    /// codepoint-aligned matches.
    ///
    /// \return Byte offset of the match, or `za::StringView::nPos`
    ///         if not found.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::SizeT find(za::StringView needle, za::SizeT startByte = 0u) const noexcept
    {
        return m_bytes.find(needle, startByte);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool startsWith(za::StringView prefix) const noexcept
    {
        return m_bytes.startsWith(prefix);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool endsWith(za::StringView suffix) const noexcept
    {
        return m_bytes.endsWith(suffix);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool contains(za::StringView needle) const noexcept
    {
        return m_bytes.contains(needle);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Replace the first occurrence of `needle` with `replacement`
    ///
    /// Byte-level substitution. See `replaceAllOccurrences` for the
    /// UTF-8 self-synchronization guarantee.
    ///
    /// \return `true` if a replacement occurred, `false` if `needle`
    ///         was not found or was empty.
    ///
    ////////////////////////////////////////////////////////////
    bool replaceFirstOccurrence(za::StringView needle, za::StringView replacement)
    {
        return m_bytes.replaceFirstOccurrence(needle, replacement);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Replace every non-overlapping occurrence of `needle`
    ///        with `replacement`
    ///
    /// Byte-level substitution. UTF-8 is self-synchronizing (no
    /// codepoint encoding is a substring of another), so passing a
    /// well-formed UTF-8 `needle` always matches at codepoint
    /// boundaries.
    ///
    /// \return Number of replacements performed.
    ///
    ////////////////////////////////////////////////////////////
    za::SizeT replaceAllOccurrences(za::StringView needle, za::StringView replacement)
    {
        return m_bytes.replaceAllOccurrences(needle, replacement);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Read-only access to the underlying byte storage
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const za::String& asBytes() const& noexcept ZA_LIFETIMEBOUND
    {
        return m_bytes;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Move the underlying byte storage out of an rvalue `Utf8String`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::String asBytes() && noexcept
    {
        return ZA_MOVE(m_bytes);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Swap contents with another `Utf8String` in O(1)
    ///
    ////////////////////////////////////////////////////////////
    void swap(Utf8String& other) noexcept
    {
        za::genericSwap(m_bytes, other.m_bytes);
    }


    ////////////////////////////////////////////////////////////
    /// \brief ADL hook so `za::genericSwap` (and `std::swap`-style
    ///        generic code) finds the optimized swap.
    ///
    ////////////////////////////////////////////////////////////
    friend void swap(Utf8String& lhs, Utf8String& rhs) noexcept
    {
        lhs.swap(rhs);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] friend bool operator==(const Utf8String& lhs, const Utf8String& rhs) noexcept
    {
        return lhs.m_bytes == rhs.m_bytes;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] friend bool operator==(const Utf8String& lhs, za::StringView rhs) noexcept
    {
        return lhs.m_bytes == rhs;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] friend bool operator==(const Utf8String& lhs, const char* rhs) noexcept
    {
        return lhs.m_bytes == rhs;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Equality against a `u8"..."` literal
    ///
    /// Avoids forcing callers to `reinterpret_cast` or construct a
    /// temporary `Utf8String` just to compare against a UTF-8 literal.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] friend bool operator==(const Utf8String& lhs, const char8_t* rhs) noexcept
    {
        return lhs.m_bytes == reinterpret_cast<const char*>(rhs);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Lexicographic ordering on the underlying UTF-8 bytes
    ///
    /// UTF-8 is designed so that byte-lex ordering coincides with
    /// Unicode codepoint ordering -- comparing the raw bytes is
    /// equivalent to comparing codepoints, without decoding.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] friend bool operator<(const Utf8String& lhs, const Utf8String& rhs) noexcept
    {
        return lhs.m_bytes < rhs.m_bytes;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] friend bool operator>(const Utf8String& lhs, const Utf8String& rhs) noexcept
    {
        return lhs.m_bytes > rhs.m_bytes;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] friend bool operator<=(const Utf8String& lhs, const Utf8String& rhs) noexcept
    {
        return lhs.m_bytes <= rhs.m_bytes;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] friend bool operator>=(const Utf8String& lhs, const Utf8String& rhs) noexcept
    {
        return lhs.m_bytes >= rhs.m_bytes;
    }


private:
    ////////////////////////////////////////////////////////////
    za::String m_bytes; //!< UTF-8 encoded byte storage
};


////////////////////////////////////////////////////////////
[[nodiscard]] inline Utf8String operator+(const Utf8String& lhs, const Utf8String& rhs)
{
    Utf8String result = lhs;
    result += rhs;
    return result;
}


////////////////////////////////////////////////////////////
/// \brief Concatenation that reuses an rvalue lhs's buffer
///
/// Avoids the copy of `lhs` performed by the lvalue overload --
/// useful for chained expressions like `a + b + c + d`.
///
////////////////////////////////////////////////////////////
[[nodiscard]] inline Utf8String operator+(Utf8String&& lhs, const Utf8String& rhs)
{
    lhs += rhs;
    return ZA_MOVE(lhs);
}

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::Utf8String
/// \ingroup system
///
/// `za::Utf8String` is the canonical type for human-readable text
/// stored as UTF-8. It wraps `za::String` (UTF-8 bytes) and
/// adds:
///
/// - A `codepoints()` range that yields one `char32_t` per Unicode
///   codepoint, decoded on the fly. ASCII iterates at near
///   pointer-walk speed; multi-byte sequences cost a few extra
///   cycles per step.
/// - Construction from `const char*`, `za::StringView`, and
///   `za::String` -- the source bytes are taken to already be
///   UTF-8.
/// - An implicit conversion to `za::StringView` so that any
///   byte-oriented sink (`SDL_*`, file I/O, network) accepts a
///   `Utf8String` directly.
///
/// Use `za::String` if your bytes are not UTF-8 (binary data,
/// HTTP body, certificate, etc.). Use `Utf8String` when the
/// bytes are *meant* to be human-readable text.
///
/// \code
/// za::Utf8String s = "Hello, カタツムリ!";
/// for (char32_t cp : s.codepoints())
///     // ... per-codepoint work
/// \endcode
///
////////////////////////////////////////////////////////////
