#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Config.hpp" // IWYU pragma: keep

#include "Zancle/IO/Path.hpp"

#include "Zancle/String/String.hpp"

#include <iosfwd>


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Stream-insertion operator for `za::Path`
///
/// Writes the path as UTF-8, avoiding locale-dependent filesystem streaming
/// (streaming `std::filesystem::path` uses the C locale, which throws on
/// MinGW for some non-ASCII paths).
///
/// A template, so that `<ostream>` (which pulls in `<format>`) is only
/// needed where a path is actually streamed, rather than in the library.
///
////////////////////////////////////////////////////////////
template <typename Traits>
std::basic_ostream<char, Traits>& operator<<(std::basic_ostream<char, Traits>& os, const Path& path)
{
    const auto utf8 = path.to<String>();
    // `int` (paths are far shorter than `INT_MAX`) converts implicitly to `std::streamsize`, which
    // `<iosfwd>` does not necessarily declare
    return os.write(utf8.data(), static_cast<int>(utf8.size()));
}

} // namespace za
