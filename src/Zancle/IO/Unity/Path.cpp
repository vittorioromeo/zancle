// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/IO/Path.hpp"

#include "Zancle/Err/FmtPath.hpp"

#include "Zancle/Fmt/FmtResult.hpp"
#include "Zancle/Fmt/FmtSink.hpp"
#include "Zancle/Fmt/FmtSpec.hpp"

#include "Zancle/IO/PathUtils.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/StringView.hpp"

#include "Zancle/Chrono/StdChrono.hpp"

#include "Zancle/Vocabulary/FunctionRef.hpp"
#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Strlen.hpp"

#include "Zancle/Trait/IsSame.hpp"

#include <filesystem>
#include <string>
#include <system_error>

#include <cstdlib>

#ifdef ZA_SYSTEM_WINDOWS
    #include "Zancle/Base/WindowsHeader.hpp"
#endif


namespace
{
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] constexpr char asciiToLower(const char c) noexcept
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c;
}


////////////////////////////////////////////////////////////
/// \brief `std::filesystem::path` from `size` bytes of UTF-8 at `data`
///
/// `std::filesystem::path`'s own `char` constructors use the "native narrow
/// encoding", which is the ANSI code page on Windows with MSVC and libc++
/// (but UTF-8 with libstdc++). Going through `char8_t` means UTF-8 everywhere.
/// Elsewhere the native narrow encoding is used verbatim (bytes, UTF-8 in
/// practice), so that non-UTF-8 file names still round-trip.
///
////////////////////////////////////////////////////////////
[[nodiscard]] std::filesystem::path fsPathFromUtf8(const char* const data, const za::SizeT size)
{
#ifdef ZA_SYSTEM_WINDOWS
    const auto* const begin = reinterpret_cast<const char8_t*>(data);
    return std::filesystem::path(begin, begin + size);
#else
    return std::filesystem::path(data, data + size);
#endif
}


////////////////////////////////////////////////////////////
template <typename Char>
[[nodiscard]] za::SizeT nullTerminatedLength(const Char* str) noexcept
{
    za::SizeT length = 0u;

    while (str[length] != Char{})
        ++length;

    return length;
}


////////////////////////////////////////////////////////////
/// \brief Run `operation(ec) -> bool` (a filesystem modification), retrying briefly on transient Windows errors
///
/// On Windows, other processes (e.g. antivirus scanners, search indexers) commonly hold freshly
/// written or deleted files open for a few milliseconds. Meanwhile, replacing such a file fails with
/// an access or sharing error, and a deleted file lingers ("delete pending"), so that removing its
/// directory fails as not empty, and re-creating a just-removed directory fails as access denied.
/// Measured on Windows 11 with Defender: ~7% of renames over a freshly written file, ~3% of directory
/// removals right after removing their last file, and ~33% of directory re-creations fail this way.
///
/// Elsewhere (and on genuine errors) `operation` runs exactly once.
///
////////////////////////////////////////////////////////////
template <typename Operation>
[[nodiscard]] bool retryTransientErrors(Operation&& operation)
{
#ifdef ZA_SYSTEM_WINDOWS
    constexpr int maxAttempts = 10; // at most ~45ms of waiting

    for (int attempt = 1;; ++attempt)
    {
        std::error_code ec;
        if (operation(ec))
            return true;

        const bool transient = ec == std::errc::permission_denied || ec == std::errc::device_or_resource_busy ||
                               ec == std::errc::directory_not_empty;

        if (!transient || attempt == maxAttempts)
            return false;

        ::Sleep(static_cast<::DWORD>(attempt));
    }
#else
    std::error_code ec;
    return operation(ec);
#endif
}

} // namespace


namespace za
{
////////////////////////////////////////////////////////////
struct Path::Impl
{
    std::filesystem::path fsPath;
};


////////////////////////////////////////////////////////////
za::Optional<Path> Path::getTempDirectory()
{
    std::error_code ec;
    auto            tmp = std::filesystem::temp_directory_path(ec);

    if (ec)
        return za::nullOpt;

    return za::makeOptional(Path{0, &tmp});
}


////////////////////////////////////////////////////////////
za::Optional<Path> Path::getCurrentDirectory()
{
    std::error_code ec;
    auto            cwd = std::filesystem::current_path(ec);

    if (ec)
        return za::nullOpt;

    return za::makeOptional(Path{0, &cwd});
}


////////////////////////////////////////////////////////////
bool Path::setCurrentDirectory(const Path& path)
{
    std::error_code ec;
    std::filesystem::current_path(path.m_impl->fsPath, ec);
    return !ec;
}


////////////////////////////////////////////////////////////
za::Optional<Path> Path::getHomeDirectory()
{
#ifdef ZA_SYSTEM_WINDOWS
    // Wide: the narrow environment uses the ANSI code page, which cannot represent every user name
    if (const wchar_t* const userProfile = ::_wgetenv(L"USERPROFILE"))
        return za::makeOptional(Path{userProfile});
#else
    if (const char* const home = std::getenv("HOME"))
        return za::makeOptional(Path{home});
#endif

    return za::nullOpt;
}


////////////////////////////////////////////////////////////
Path::Path() = default;


////////////////////////////////////////////////////////////
Path::Path(const char* source) : Path(0, source, ZA_STRLEN(source))
{
}


////////////////////////////////////////////////////////////
Path::Path(const wchar_t* source) : Path(0, source, nullTerminatedLength(source))
{
}


////////////////////////////////////////////////////////////
Path::Path(const char32_t* source) : Path(0, source, nullTerminatedLength(source))
{
}


////////////////////////////////////////////////////////////
Path::Path(int, const void* fsPath) : m_impl{*static_cast<const std::filesystem::path*>(fsPath)}
{
}


////////////////////////////////////////////////////////////
Path::Path(int, const char* data, const za::SizeT size) : m_impl{fsPathFromUtf8(data, size)}
{
}


////////////////////////////////////////////////////////////
Path::Path(int, const wchar_t* data, const za::SizeT size) : m_impl{std::filesystem::path(data, data + size)}
{
}


////////////////////////////////////////////////////////////
Path::Path(int, const char32_t* data, const za::SizeT size) : m_impl{std::filesystem::path(data, data + size)}
{
}


////////////////////////////////////////////////////////////
Path::~Path()                          = default;
Path::Path(const Path&)                = default;
Path& Path::operator=(const Path&)     = default;
Path::Path(Path&&) noexcept            = default;
Path& Path::operator=(Path&&) noexcept = default;


////////////////////////////////////////////////////////////
Path Path::getFilename() const
{
    const auto fn = m_impl->fsPath.filename();
    return Path{0, &fn};
}


////////////////////////////////////////////////////////////
Path Path::getStem() const
{
    const auto s = m_impl->fsPath.stem();
    return Path{0, &s};
}


////////////////////////////////////////////////////////////
Path Path::getExtension() const
{
    const auto ext = m_impl->fsPath.extension();
    return Path{0, &ext};
}


////////////////////////////////////////////////////////////
za::Optional<Path> Path::getAbsolute() const
{
    std::error_code ec;
    const auto      abs = std::filesystem::absolute(m_impl->fsPath, ec);

    if (ec)
        return za::nullOpt;

    return za::makeOptional(Path{0, &abs});
}


////////////////////////////////////////////////////////////
Path Path::getParent() const
{
    const auto p = m_impl->fsPath.parent_path();
    return Path{0, &p};
}


////////////////////////////////////////////////////////////
const Path::value_type* Path::c_str() const
{
    return m_impl->fsPath.c_str();
}


////////////////////////////////////////////////////////////
bool Path::empty() const
{
    return m_impl->fsPath.empty();
}


////////////////////////////////////////////////////////////
bool Path::exists() const
{
    std::error_code ec;
    return std::filesystem::exists(m_impl->fsPath, ec);
}


////////////////////////////////////////////////////////////
bool Path::isDirectory() const
{
    std::error_code ec;
    return std::filesystem::is_directory(m_impl->fsPath, ec);
}


////////////////////////////////////////////////////////////
bool Path::isRegularFile() const
{
    std::error_code ec;
    return std::filesystem::is_regular_file(m_impl->fsPath, ec);
}


////////////////////////////////////////////////////////////
bool Path::isSymlink() const
{
    std::error_code ec;
    return std::filesystem::is_symlink(m_impl->fsPath, ec);
}


////////////////////////////////////////////////////////////
za::Optional<za::U64> Path::getFileSize() const
{
    std::error_code ec;
    const auto      size = std::filesystem::file_size(m_impl->fsPath, ec);

    if (ec)
        return za::nullOpt;

    return za::makeOptional(static_cast<za::U64>(size));
}


////////////////////////////////////////////////////////////
za::Optional<za::I64> Path::getLastWriteTimeSecondsSinceEpoch() const
{
    std::error_code ec;
    const auto      ftime = std::filesystem::last_write_time(m_impl->fsPath, ec);

    if (ec)
        return za::nullOpt;

    const auto sysTime = std::chrono::file_clock::to_sys(ftime);
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(sysTime.time_since_epoch()).count();
    return za::makeOptional(static_cast<za::I64>(seconds));
}


////////////////////////////////////////////////////////////
bool Path::extensionIs(const za::StringView str) const
{
    // Delegate the "what is the extension substring" decision to
    // `std::filesystem::path::extension()` so we always match its
    // semantics for `.`, `..`, leading-dot stems, etc. Compare in UTF-8,
    // like `str`: on Windows, narrowing the native UTF-16 code units would
    // make non-ASCII characters alias ASCII ones (e.g. U+0170 as 'p').
    // Typical extensions like `.png` fit in the small-string buffer.
    const auto ext = m_impl->fsPath.extension().u8string();

    if (ext.size() != str.size())
        return false;

    for (za::SizeT i = 0u; i < ext.size(); ++i)
        if (asciiToLower(static_cast<char>(ext[i])) != asciiToLower(str[i]))
            return false;

    return true;
}


////////////////////////////////////////////////////////////
bool Path::hasParent() const
{
    return m_impl->fsPath.has_parent_path();
}


////////////////////////////////////////////////////////////
bool Path::removeFromDisk() const
{
    // A missing path yields `false` without an error, and is not retried
    return retryTransientErrors([&](std::error_code& ec) { return std::filesystem::remove(m_impl->fsPath, ec) && !ec; });
}


////////////////////////////////////////////////////////////
bool Path::copyFileTo(const Path& path) const
{
    std::error_code ec;
    return std::filesystem::copy_file(m_impl->fsPath, path.m_impl->fsPath, ec) && !ec;
}


////////////////////////////////////////////////////////////
bool Path::createLeafDirectory() const
{
    // An existing directory yields `false` without an error, and is not retried
    return retryTransientErrors([&](std::error_code& ec)
    { return std::filesystem::create_directory(m_impl->fsPath, ec) && !ec; });
}


////////////////////////////////////////////////////////////
bool Path::createDirectoryTree() const
{
    return retryTransientErrors([&](std::error_code& ec)
    {
        const bool created = std::filesystem::create_directories(m_impl->fsPath, ec);

        // `create_directories` returns false when the path already exists; that's not an error.
        return !ec && (created || std::filesystem::is_directory(m_impl->fsPath, ec));
    });
}


////////////////////////////////////////////////////////////
bool Path::renameTo(const Path& target) const
{
    return retryTransientErrors([&](std::error_code& ec)
    {
        std::filesystem::rename(m_impl->fsPath, target.m_impl->fsPath, ec);
        return !ec;
    });
}


////////////////////////////////////////////////////////////
bool Path::forEachEntry(za::FunctionRef<void(const Path&)> callback) const
{
    std::error_code                     ec;
    std::filesystem::directory_iterator it(m_impl->fsPath, ec);

    if (ec)
        return false;

    // Check `ec` after every `increment`: on error, the iterator becomes the end
    // iterator, which would otherwise end the loop as if the listing were complete
    for (const std::filesystem::directory_iterator end; it != end;)
    {
        const auto& entryPath = it->path();
        callback(Path{0, &entryPath});

        it.increment(ec);
        if (ec)
            return false;
    }

    return true;
}

////////////////////////////////////////////////////////////
Path& Path::operator/=(const Path& rhs)
{
    m_impl->fsPath /= rhs.m_impl->fsPath;
    return *this;
}


////////////////////////////////////////////////////////////
Path operator/(const Path& lhs, const Path& rhs)
{
    const auto joined = lhs.m_impl->fsPath / rhs.m_impl->fsPath;
    return Path{0, &joined};
}


////////////////////////////////////////////////////////////
Path operator/(Path&& lhs, const Path& rhs)
{
    lhs /= rhs;
    return ZA_MOVE(lhs);
}


////////////////////////////////////////////////////////////
Path& Path::operator+=(const Path& rhs)
{
    m_impl->fsPath += rhs.m_impl->fsPath;
    return *this;
}


////////////////////////////////////////////////////////////
Path operator+(const Path& lhs, const Path& rhs)
{
    auto result = lhs.m_impl->fsPath;
    result += rhs.m_impl->fsPath;
    return Path{0, &result};
}


////////////////////////////////////////////////////////////
template <typename T>
T Path::to() const
{
    if constexpr (ZA_IS_SAME(T, std::filesystem::path))
        return m_impl->fsPath;
    else if constexpr (ZA_IS_SAME(T, za::String))
    {
        // `u8string()` is locale-independent; `string()` throws on MinGW/Clang64 when the
        // path contains characters outside the current codepage.
        const auto res = m_impl->fsPath.u8string();
        return za::String{reinterpret_cast<const char*>(res.data()), res.size()};
    }
    else if constexpr (ZA_IS_SAME(T, std::string))
    {
        const auto res = m_impl->fsPath.u8string();
        return std::string{reinterpret_cast<const char*>(res.data()), res.size()};
    }
    else if constexpr (ZA_IS_SAME(T, std::u8string))
        return m_impl->fsPath.u8string();
    else if constexpr (ZA_IS_SAME(T, std::u32string))
        return m_impl->fsPath.u32string();
    else if constexpr (ZA_IS_SAME(T, std::wstring))
        return m_impl->fsPath.wstring();
    else
        static_assert(false,
                      "za::Path::to<T>(): unsupported target type. Supported: std::filesystem::path, "
                      "za::String, std::string, std::u8string, std::u32string, std::wstring.");
}


////////////////////////////////////////////////////////////
bool Path::operator==(const Path& rhs) const
{
    return m_impl->fsPath == rhs.m_impl->fsPath;
}


////////////////////////////////////////////////////////////
bool Path::operator==(const char* str) const
{
    return *this == Path{str};
}


////////////////////////////////////////////////////////////
bool Path::operator==(const wchar_t* str) const
{
    return *this == Path{str};
}


////////////////////////////////////////////////////////////
bool Path::operator==(const char32_t* str) const
{
    return *this == Path{str};
}


////////////////////////////////////////////////////////////
template ZA_SYSTEM_API std::filesystem::path Path::to<std::filesystem::path>() const;
template ZA_SYSTEM_API std::string Path::to<std::string>() const;
template ZA_SYSTEM_API za::String Path::to<za::String>() const;
template ZA_SYSTEM_API std::u8string Path::to<std::u8string>() const;
template ZA_SYSTEM_API std::u32string Path::to<std::u32string>() const;
template ZA_SYSTEM_API std::wstring Path::to<std::wstring>() const;


// `operator<<(std::ostream&, const Path&)` lives in `PathStreamOp.cpp`
// -- see the `<ostream>` comment near the top of this file.


////////////////////////////////////////////////////////////
za::FmtResult fmtArg(za::FmtSink& sink, const Path& path, const za::FmtSpec&)
{
    // Same rationale as the stream-insertion operator above: emit UTF-8 to avoid
    // locale-dependent encoding of `std::filesystem::path` on Windows.
    const auto u8 = path.to<std::string>();
    return sink.append(u8.data(), static_cast<za::SizeT>(u8.size()));
}

} // namespace za


////////////////////////////////////////////////////////////
namespace za::priv
{
////////////////////////////////////////////////////////////
za::FmtResult fmtArg(za::FmtSink& sink, const PathDebugFormatter& dbg, const za::FmtSpec&)
{
    // Two debug lines: input path + resolved absolute path (or sentinel).
    ZA_FMT_TRY(sink.append("    Provided path: ", 19u));
    ZA_FMT_TRY(fmtArg(sink, dbg.path, za::FmtSpec{}));
    ZA_FMT_TRY(sink.appendChar('\n'));

    ZA_FMT_TRY(sink.append("    Absolute path: ", 19u));

    if (const auto abs = dbg.path.getAbsolute(); abs.hasValue())
        return fmtArg(sink, *abs, za::FmtSpec{});

    return sink.append("<unavailable>", 13u);
}

} // namespace za::priv
