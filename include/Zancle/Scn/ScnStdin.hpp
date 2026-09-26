#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include "Zancle/Config.hpp"

#include "Zancle/Scn/Scn.hpp" // IWYU pragma: export -- scn<T> / scnInto

#include "Zancle/Vocabulary/Optional.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class String;
}


namespace za
{
////////////////////////////////////////////////////////////
/// \brief `ScnSource` wrapping libc stdin (`std::getchar` / `std::ungetc`).
///
/// Holds a 1-byte read-ahead cache so `peek()` is non-destructive
/// when followed by another `peek()`. On destruction, an un-consumed
/// cached byte is `ungetc`'d back to stdin so the next reader sees it.
///
/// Move-only; cheap to construct (no heap allocations, no buffers
/// other than the cached byte).
///
////////////////////////////////////////////////////////////
class ZA_SYSTEM_API ScnStdinSource
{
public:
    ////////////////////////////////////////////////////////////
    ScnStdinSource() noexcept;
    ~ScnStdinSource();

    ////////////////////////////////////////////////////////////
    ScnStdinSource(const ScnStdinSource&)            = delete;
    ScnStdinSource& operator=(const ScnStdinSource&) = delete;

    ////////////////////////////////////////////////////////////
    ScnStdinSource(ScnStdinSource&&) noexcept;
    ScnStdinSource& operator=(ScnStdinSource&&) noexcept;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Optional<char> peek();
    void                         consume() noexcept;

private:
    int m_peeked; //!< libc int (EOF or a byte cast through `unsigned char`); -2 means "cache empty".
};


////////////////////////////////////////////////////////////
/// \brief Read one `T` from stdin via a fresh `ScnStdinSource`.
///
/// Returns `nullOpt` on parse failure / EOF, matching `scn<T>` for
/// other sources. Equivalent to `int v; if (!(cin >> v)) error()`,
/// but with an explicit `Optional` return.
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] Optional<T> scnStdin()
{
    ScnStdinSource src;
    return scn<T>(src);
}


////////////////////////////////////////////////////////////
/// \brief Read into `out` from stdin via a fresh `ScnStdinSource`.
/// Returns `true` on success, `false` on parse failure / EOF.
///
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] bool scnStdinInto(T& out)
{
    ScnStdinSource src;
    return scnInto(src, out);
}


////////////////////////////////////////////////////////////
/// \brief Drop bytes from stdin until (and including) the next `'\n'`,
/// or until EOF if no newline arrives first. Direct replacement for
/// `std::cin.ignore(N, '\n')`.
///
////////////////////////////////////////////////////////////
ZA_SYSTEM_API void scnStdinIgnoreLine();


////////////////////////////////////////////////////////////
/// \brief Read a line from stdin into `out` (excluding the `'\n'`).
/// Returns `false` only if stdin was already at EOF on entry.
///
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_SYSTEM_API bool scnStdinReadLine(String& out);

} // namespace za


////////////////////////////////////////////////////////////
/// \file
/// `ScnStdinSource`: an `ScnSource` that pulls bytes from stdin via
/// libc's `getchar` / `ungetc` pair.
///
/// Not contiguous -- terminal input is line-buffered and arbitrarily
/// large, so handing out a `StringView` of "remaining bytes" doesn't
/// make sense. Built-in parsers therefore fall back to the
/// peek/consume slow path against this source, which is the right
/// thing: stdin's actual cost lives in the OS syscall, dwarfing any
/// per-call overhead in user code.
///
/// Two helpers wrap the source for the common "read one value from
/// stdin" case:
///   - `scnStdin<T>() -> Optional<T>`
///   - `scnStdinInto(T&) -> bool`
///
/// Both construct a fresh source per call, so there is no hidden
/// persistent state -- consecutive `scnStdin<int>()` reads work the
/// same way `cin >> a; cin >> b;` does (the source's one-byte cache
/// is `ungetc`'d on destruction).
///
////////////////////////////////////////////////////////////
