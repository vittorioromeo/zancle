#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/String/String.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] constexpr bool isWhitespace(const char c) noexcept
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == '\f';
}

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Stream-insertion operator for `za::String`
///
/// Writes the entire string to any stream-like type that exposes a
/// `write(const char*, long)` member, mirroring the behavior of
/// `std::ostream << std::string`.
///
////////////////////////////////////////////////////////////
template <typename StreamLike>
StreamLike& operator<<(StreamLike& stream, const String& s)
    requires(requires { stream.write(s.data(), static_cast<long>(s.size())); })
{
    stream.write(s.data(), static_cast<long>(s.size()));
    return stream;
}


////////////////////////////////////////////////////////////
/// \brief Extract one whitespace-delimited word into `s`, mimicking `std::istream >> std::string`
///
/// If no characters are extracted (only whitespace remained), `s` is left
/// empty and, for streams with `setstate`/`failbit`, the fail bit is set.
///
////////////////////////////////////////////////////////////
template <typename StreamLike>
StreamLike& operator>>(StreamLike& stream, String& s)
    requires(requires(StreamLike& str) {
        { str.peek() };
        { str.get() };
    })
{
    s.clear();

    // 1. Skip leading whitespace
    int c = stream.peek();
    while (c != -1 && priv::isWhitespace(static_cast<char>(c)))
    {
        stream.get(); // Consume the whitespace character
        c = stream.peek();
    }

    // 2. Read non-whitespace characters until the next whitespace or EOF
    while (c != -1 && !priv::isWhitespace(static_cast<char>(c)))
    {
        s.pushBack(static_cast<char>(stream.get())); // Consume and append the character
        c = stream.peek();
    }

    // 3. Like `std::istream >> std::string`, fail if nothing was extracted, so that
    //    `while (in >> s)` stops instead of yielding a final empty word
    if constexpr (requires { stream.setstate(StreamLike::failbit); })
    {
        if (s.empty())
            stream.setstate(StreamLike::failbit);
    }

    return stream;
}

} // namespace za
