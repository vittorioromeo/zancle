// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/IO/IO.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/IO/FileUtils.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/StringView.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/PtrDiffT.hpp"
#include "Zancle/Base/ScopeGuard.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsSame.hpp"

#include <string>

#include <cstdio>

// Native fast-path file reads. The C++ iostream layer has overhead that becomes noticeable for the small/medium files
// Zancle typically loads. We bypass it where we can and fall back to C stdio if no native path is available.
#if defined(ZA_SYSTEM_WINDOWS)
    #include "Zancle/Base/WindowsHeader.hpp"

    #define ZA_PRIV_IO_NATIVE_BACKEND 1 // Windows
#elif __has_include(<fcntl.h>) && __has_include(<unistd.h>) && __has_include(<sys/stat.h>)
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>

    #include <cerrno>

    #define ZA_PRIV_IO_NATIVE_BACKEND 2 // POSIX
#else
    #define ZA_PRIV_IO_NATIVE_BACKEND 0 // Fallback to C stdio
#endif

namespace
{
////////////////////////////////////////////////////////////
// Convert either a `StringView` or a `Path` into a UTF-8 `std::string` for
// use with C stdio's narrow-char `fopen`. Used by the fallback path.
[[maybe_unused, gnu::always_inline]] inline std::string toUtf8FilenameForStdio(za::StringView v)
{
    return v.toString<std::string>();
}


////////////////////////////////////////////////////////////
[[maybe_unused, gnu::always_inline]] inline std::string toUtf8FilenameForStdio(const za::Path& p)
{
    return p.to<std::string>();
}


////////////////////////////////////////////////////////////
// Fallback implementation that uses C stdio (`fopen` / `fread`). Used on
// platforms where no native fast path is available. Avoids the iostream
// streambuf/locale machinery while staying ISO C portable.
template <typename Filename, typename T>
bool readFromFileFallback(const Filename& filename, T& target, const bool isAppend)
{
    ZA_ASSERT(!isAppend || ZA_IS_SAME(T, za::Vector<char>));

    const auto fail = [&]
    {
        za::priv::errMsg("Failed to read from file '{}'\n", filename);
        return false;
    };

    // `fopen` requires a NUL-terminated path. Both `StringView` and `Path` are
    // routed through the UTF-8 narrow-char API since C stdio only supports it
    // portably (Windows has `_wfopen` but the fallback never compiles there).
    const auto path = toUtf8FilenameForStdio(filename);

    std::FILE* const file = std::fopen(path.c_str(), "rb");

    if (file == nullptr)
        return fail();

    ZA_SCOPE_GUARD({ std::fclose(file); });

    // Determine size by seeking to the end. `ftell` on a binary stream is not
    // strictly defined by C, but works on every hosted platform that would
    // ever take this fallback branch.
    if (std::fseek(file, 0, SEEK_END) != 0)
        return fail();

    const long rawSize = std::ftell(file);

    if (rawSize < 0 || std::fseek(file, 0, SEEK_SET) != 0)
        return fail();

    if (rawSize == 0)
    {
        if (!isAppend)
            target.clear();

        return true;
    }

    const auto size = static_cast<za::SizeT>(rawSize);

    za::SizeT got = 0u;

    dispatchReadFileContentsIntoBufferImpl(target,
                                           size,
                                           isAppend,
                                           [&](char* buf, za::SizeT n)
    {
        got = std::fread(buf, 1u, n, file);
        return n;
    });

    if (got != size)
        return fail();

    return true;
}


#if ZA_PRIV_IO_NATIVE_BACKEND == 1

////////////////////////////////////////////////////////////
// Common Windows post-open size/limit check used by the StringView and Path
// openers below. `handle` is closed on failure.
[[nodiscard]] bool nativeStatHandle(::HANDLE handle, za::SizeT& outSize)
{
    ::LARGE_INTEGER fileSize{};
    if (!::GetFileSizeEx(handle, &fileSize) || fileSize.QuadPart < 0)
    {
        ::CloseHandle(handle);
        return false;
    }

    // Guard against truncating a >SizeT file size on 32-bit builds.
    using ULL = unsigned long long;
    if (static_cast<ULL>(fileSize.QuadPart) > static_cast<ULL>(static_cast<za::SizeT>(-1)))
    {
        ::CloseHandle(handle);
        return false;
    }

    outSize = static_cast<za::SizeT>(fileSize.QuadPart);
    return true;
}


////////////////////////////////////////////////////////////
[[nodiscard]] inline ::HANDLE nativeCreateFileW(const wchar_t* nullTerminatedPath)
{
    return ::CreateFileW(nullTerminatedPath,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         nullptr,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                         nullptr);
}


////////////////////////////////////////////////////////////
// Windows native: `CreateFileW` + `GetFileSizeEx` + `ReadFile`.
// Goes through the wide-char API, which (unlike the iostream path that takes
// `std::string`) interprets `filename` as UTF-8 -- an improvement over the
// fallback for non-ASCII paths.
[[nodiscard]] bool nativeOpenAndStat(za::StringView filename, ::HANDLE& outHandle, za::SizeT& outSize)
{
    const int utf8Len = static_cast<int>(filename.size());

    // `MultiByteToWideChar` rejects zero-length input; treat empty filename as failure here.
    if (utf8Len <= 0)
        return false;

    const int wideLen = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename.data(), utf8Len, nullptr, 0);
    if (wideLen <= 0)
        return false;

    // `resize_and_overwrite` (C++23) skips the zero-init pass that `wstring(n, L'\0')` would do.
    // `c_str()` is auto-null-terminated past `size()` regardless.
    std::wstring widePath;
    widePath.resize_and_overwrite(static_cast<za::SizeT>(wideLen),
                                  [&](wchar_t* buf, za::SizeT n) -> za::SizeT
    {
        return static_cast<za::SizeT>(
            ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename.data(), utf8Len, buf, static_cast<int>(n)));
    });

    const ::HANDLE handle = nativeCreateFileW(widePath.c_str());
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    if (!nativeStatHandle(handle, outSize))
        return false;

    outHandle = handle;
    return true;
}


////////////////////////////////////////////////////////////
// Path-native overload: `Path::c_str()` already returns `const wchar_t*` on
// Windows, so we can hand it straight to `CreateFileW` and skip the UTF-8
// → UTF-16 round-trip the StringView opener performs.
[[nodiscard]] bool nativeOpenAndStat(const za::Path& filename, ::HANDLE& outHandle, za::SizeT& outSize)
{
    const ::HANDLE handle = nativeCreateFileW(filename.c_str());
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    if (!nativeStatHandle(handle, outSize))
        return false;

    outHandle = handle;
    return true;
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool nativeReadFully(::HANDLE handle, char* buffer, za::SizeT size)
{
    while (size > 0u)
    {
        // `ReadFile` takes a DWORD; cap each call at a safe chunk size.
        constexpr za::SizeT maxChunk = static_cast<za::SizeT>(0x7f'ff'ff'ffu);
        const ::DWORD       chunk    = static_cast<::DWORD>(size > maxChunk ? maxChunk : size);

        ::DWORD bytesRead = 0u;
        if (!::ReadFile(handle, buffer, chunk, &bytesRead, nullptr) || bytesRead == 0u)
            return false;

        buffer += bytesRead;
        size -= static_cast<za::SizeT>(bytesRead);
    }
    return true;
}


////////////////////////////////////////////////////////////
inline void nativeClose(::HANDLE handle)
{
    ::CloseHandle(handle);
}

#elif ZA_PRIV_IO_NATIVE_BACKEND == 2

////////////////////////////////////////////////////////////
// Common POSIX post-open size/limit check used by the StringView and Path
// openers below. `fd` is closed on failure.
[[nodiscard]] bool nativeStatFd(int fd, za::SizeT& outSize)
{
    struct ::stat st{};
    if (::fstat(fd, &st) != 0 || st.st_size < 0)
    {
        ::close(fd);
        return false;
    }

    using ULL = unsigned long long;
    if (static_cast<ULL>(st.st_size) > static_cast<ULL>(static_cast<za::SizeT>(-1)))
    {
        ::close(fd);
        return false;
    }

    outSize = static_cast<za::SizeT>(st.st_size);
    return true;
}


////////////////////////////////////////////////////////////
[[nodiscard]] inline int nativeOpenCStr(const char* nullTerminatedPath)
{
    int fd = -1;

    do
    {
        fd = ::open(nullTerminatedPath, O_RDONLY | O_CLOEXEC);
    } while (fd < 0 && errno == EINTR);

    return fd;
}


////////////////////////////////////////////////////////////
// POSIX native: `open` + `fstat` + `read`.
[[nodiscard]] bool nativeOpenAndStat(za::StringView filename, int& outFd, za::SizeT& outSize)
{
    // `open` requires a NUL-terminated path; `StringView` is not guaranteed to be one.
    const auto path = filename.toString<std::string>();

    const int fd = nativeOpenCStr(path.c_str());
    if (fd < 0)
        return false;

    if (!nativeStatFd(fd, outSize))
        return false;

    outFd = fd;
    return true;
}


////////////////////////////////////////////////////////////
// Path-native overload: on Linux/BSD/Emscripten `Path::c_str()` is already a
// UTF-8 `const char*` (per `Zancle/IO/Path.hpp`'s value_type macro), so it
// can be handed straight to `::open` with no per-call allocation. On other
// POSIX platforms (macOS/iOS/Android) Path stores `wchar_t`, so we go through
// a single `to<std::string>()` UTF-8 conversion -- then call `::open` directly
// rather than delegating to the StringView opener (which would re-convert).
[[nodiscard]] bool nativeOpenAndStat(const za::Path& filename, int& outFd, za::SizeT& outSize)
{
    #if defined(ZA_SYSTEM_LINUX_OR_BSD) || defined(ZA_SYSTEM_EMSCRIPTEN)
    const int fd = nativeOpenCStr(filename.c_str());
    #else
    const auto utf8 = filename.to<std::string>();
    const int  fd   = nativeOpenCStr(utf8.c_str());
    #endif

    if (fd < 0)
        return false;

    if (!nativeStatFd(fd, outSize))
        return false;

    outFd = fd;
    return true;
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool nativeReadFully(int fd, char* buffer, za::SizeT size)
{
    while (size > 0u)
    {
        const ::ssize_t got = ::read(fd, buffer, size);

        if (got < 0)
        {
            if (errno == EINTR)
                continue;

            return false;
        }

        if (got == 0)
            return false; // unexpected EOF: file shrank between fstat and read

        buffer += got;
        size -= static_cast<za::SizeT>(got);
    }

    return true;
}


////////////////////////////////////////////////////////////
inline void nativeClose(int fd)
{
    ::close(fd);
}

#endif


template <typename T>
void dispatchReadFileContentsIntoBufferImpl(T& target, const za::SizeT size, const bool isAppend, auto&& fFillBuffer)
{
    if constexpr (ZA_IS_SAME(T, std::string))
    {
        target.resize_and_overwrite(size, fFillBuffer);
    }
    else if constexpr (ZA_IS_SAME(T, za::String))
    {
        target.resizeAndOverwrite(size, fFillBuffer);
    }
    else
    {
        static_assert(ZA_IS_SAME(T, za::Vector<char>));

        const za::SizeT origin    = isAppend ? target.size() : 0u;
        const za::SizeT finalSize = origin + size;

        target.reserve(finalSize);
        fFillBuffer(target.data() + origin, size);
        target.unsafeSetSize(finalSize);
    }
}

////////////////////////////////////////////////////////////
// `Filename` is either `za::StringView` or `const za::Path&`; both
// resolve to a matching `nativeOpenAndStat` overload above. The Path-taking
// version skips the UTF-8 → UTF-16 conversion on Windows and skips the
// `to<std::string>()` allocation on Linux/BSD/Emscripten.
// `isAppend` is meaningful only when `T == za::Vector<char>`: when true, the
// file's bytes are appended to `target` (preserving existing content); when
// false, `target` is replaced. For string types it must be `false` and the
// behavior is replace.
template <typename Filename, typename T>
bool readFromFileImpl(const Filename& filename, T& target, const bool isAppend)
{
    ZA_ASSERT(!isAppend || ZA_IS_SAME(T, za::Vector<char>));

#if ZA_PRIV_IO_NATIVE_BACKEND == 0 // Fallback
    return readFromFileFallback(filename, target, isAppend);
#else
    #if ZA_PRIV_IO_NATIVE_BACKEND == 1 // Windows
    ::HANDLE handle = INVALID_HANDLE_VALUE;
    #else                              // POSIX
    int handle = -1;
    #endif

    za::SizeT size = 0u;
    if (!nativeOpenAndStat(filename, handle, size))
    {
        za::priv::errMsg("Failed to read from file '{}'\n", filename);
        return false;
    }

    ZA_SCOPE_GUARD({ nativeClose(handle); });

    if (size == 0u)
    {
        if (!isAppend)
            target.clear();

        return true;
    }

    // `resize_and_overwrite` (std::string, C++23) and the analogous
    // `resizeAndOverwrite` on `za::String` skip the zero-init pass that
    // a plain `resize` would do.
    bool readOk = false;

    dispatchReadFileContentsIntoBufferImpl(target,
                                           size,
                                           isAppend,
                                           [&](char* buf, za::SizeT n)
    {
        readOk = nativeReadFully(handle, buf, n);
        return n;
    });

    if (!readOk)
    {
        za::priv::errMsg("Failed to read the full contents of file '{}'\n", filename);
        return false;
    }

    return true;
#endif
}

} // namespace


namespace za
{
////////////////////////////////////////////////////////////
bool writeToFile(za::StringView filename, za::StringView contents)
{
    // `Path{StringView}` is unavailable; route via `std::string` (matches the
    // pre-migration behavior, which constructed `std::ofstream` the same way).
    auto optFile = OutFile::open(Path{filename.toString<std::string>()}, FileOpenMode::bin);
    if (!optFile.hasValue())
    {
        priv::errMsg("Failed to write to file '{}'\n", filename);
        return false;
    }

    // Destructor closes; explicit close errors would be lost here, but the
    // write itself is reported and that's the meaningful failure path.
    return optFile->write(contents.data(), contents.size());
}


////////////////////////////////////////////////////////////
bool writeToFile(const Path& filename, za::StringView contents)
{
    auto optFile = OutFile::open(filename, FileOpenMode::bin);
    if (!optFile.hasValue())
    {
        priv::errMsg("Failed to write to file '{}'\n", filename);
        return false;
    }

    return optFile->write(contents.data(), contents.size());
}


////////////////////////////////////////////////////////////
bool readFromFile(za::StringView filename, std::string& target)
{
    return readFromFileImpl(filename, target, /* isAppend */ false);
}


////////////////////////////////////////////////////////////
bool readFromFile(za::StringView filename, za::String& target)
{
    return readFromFileImpl(filename, target, /* isAppend */ false);
}


////////////////////////////////////////////////////////////
bool readFromFile(za::StringView filename, za::Vector<char>& target)
{
    return readFromFileImpl(filename, target, /* isAppend */ false);
}


////////////////////////////////////////////////////////////
bool readFromFile(const Path& filename, std::string& target)
{
    return readFromFileImpl(filename, target, /* isAppend */ false);
}


////////////////////////////////////////////////////////////
bool readFromFile(const Path& filename, za::String& target)
{
    return readFromFileImpl(filename, target, /* isAppend */ false);
}


////////////////////////////////////////////////////////////
bool readFromFile(const Path& filename, za::Vector<char>& target)
{
    return readFromFileImpl(filename, target, /* isAppend */ false);
}


////////////////////////////////////////////////////////////
bool appendFromFile(za::StringView filename, za::Vector<char>& target)
{
    return readFromFileImpl(filename, target, /* isAppend */ true);
}


////////////////////////////////////////////////////////////
bool appendFromFile(const Path& filename, za::Vector<char>& target)
{
    return readFromFileImpl(filename, target, /* isAppend */ true);
}


////////////////////////////////////////////////////////////
za::Vector<char>& getThreadLocalScratchCharBuffer()
{
    static thread_local za::Vector<char> buffer;
    return buffer;
}


////////////////////////////////////////////////////////////
namespace
{
////////////////////////////////////////////////////////////
// Map our `FileOpenMode` flags to a `fopen`-style mode string.
//
// `OutFile` is always opened for writing, so the table only distinguishes
// truncate-vs-append and text-vs-binary. The `in` / `ate` flags are
// meaningless on an output handle and ignored. Returns one of:
//     "w", "wb", "a", "ab"
[[nodiscard, gnu::const]] constexpr const char* mapOutFileOpenMode(const za::FileOpenMode mode) noexcept
{
    const bool append = (mode & za::FileOpenMode::app) != za::FileOpenMode::none;
    const bool binary = (mode & za::FileOpenMode::bin) != za::FileOpenMode::none;

    if (append)
        return binary ? "ab" : "a";

    return binary ? "wb" : "w";
}


////////////////////////////////////////////////////////////
// Shared close helper for `OutFile` / `InFile` destructors and move
// assignment. Closes `file` if non-null and reports any `fclose` error
// via `errMsg`, tagged with `kindName` (e.g. "OutFile", "InFile") so the
// log line is attributable. A moved-from object has `file == nullptr`,
// which short-circuits to a no-op.
void closeAndReport(std::FILE* const file, const char* const kindName)
{
    if (file == nullptr)
        return;

    if (std::fclose(file) != 0)
        za::priv::errMsg("za::{}: `fclose` reported an error; buffered output may have been lost", kindName);
}

} // namespace


////////////////////////////////////////////////////////////
struct OutFile::Impl
{
    std::FILE* file; //!< Non-null on a live `OutFile`; null only in a moved-from object.
};


////////////////////////////////////////////////////////////
OutFile::OutFile(za::PassKey<OutFile>&&, void* file) noexcept : m_impl{static_cast<std::FILE*>(file)}
{
}


////////////////////////////////////////////////////////////
OutFile::~OutFile()
{
    closeAndReport(m_impl->file, "OutFile");
}


////////////////////////////////////////////////////////////
OutFile::OutFile(OutFile&& rhs) noexcept : m_impl{rhs.m_impl->file}
{
    rhs.m_impl->file = nullptr;
}


////////////////////////////////////////////////////////////
OutFile& OutFile::operator=(OutFile&& rhs) noexcept
{
    if (&rhs == this)
        return *this;

    closeAndReport(m_impl->file, "OutFile");

    m_impl->file     = rhs.m_impl->file;
    rhs.m_impl->file = nullptr;

    return *this;
}


////////////////////////////////////////////////////////////
za::Optional<OutFile> OutFile::open(const Path& filename, FileOpenMode mode)
{
    std::FILE* const file = openFile(filename, mapOutFileOpenMode(mode));
    if (file == nullptr)
        return za::nullOpt;

    return za::makeOptional<OutFile>(za::PassKey<OutFile>{}, file);
}


////////////////////////////////////////////////////////////
bool OutFile::write(const char* data, za::SizeT size)
{
    if (m_impl->file == nullptr)
        return false;

    return std::fwrite(data, 1u, size, m_impl->file) == size;
}


////////////////////////////////////////////////////////////
bool OutFile::flush()
{
    if (m_impl->file == nullptr)
        return false;

    return std::fflush(m_impl->file) == 0;
}


////////////////////////////////////////////////////////////
bool OutFile::seekPos(za::PtrDiffT absolutePos)
{
    if (m_impl->file == nullptr)
        return false;

    return std::fseek(m_impl->file, static_cast<long>(absolutePos), SEEK_SET) == 0;
}


////////////////////////////////////////////////////////////
bool OutFile::tellPos(za::PtrDiffT& out)
{
    if (m_impl->file == nullptr)
        return false;

    const long pos = std::ftell(m_impl->file);
    if (pos < 0)
        return false;

    out = static_cast<za::PtrDiffT>(pos);
    return true;
}


////////////////////////////////////////////////////////////
void OutFile::append(const char* data, za::SizeT n)
{
    // `AppendSink` adapter: best-effort, return value lost. The Fmt
    // pipeline buffers internally and only calls `append` once at the
    // end, so a discarded failure here at worst corrupts the on-disk
    // file silently. Use `write()` directly when that matters.
    (void)write(data, n);
}


////////////////////////////////////////////////////////////
namespace
{
////////////////////////////////////////////////////////////
// Map our `FileOpenMode` flags to a `fopen`-style mode string for input.
//
// `InFile` always opens for reading. The only meaningful distinction left
// is text vs. binary; `app` / `out` / `trunc` are ignored on the input
// side. `ate` (open at end) is handled separately by `open()` via a
// post-`fopen` seek -- mirroring `std::ios::ate` semantics.
[[nodiscard, gnu::const]] constexpr const char* mapInFileOpenMode(const za::FileOpenMode mode) noexcept
{
    return (mode & za::FileOpenMode::bin) != za::FileOpenMode::none ? "rb" : "r";
}


////////////////////////////////////////////////////////////
// Map `SeekDir` to a stdio `SEEK_*` constant.
[[nodiscard, gnu::const]] constexpr int mapSeekDirToStdio(const za::SeekDir dir) noexcept
{
    switch (dir)
    {
        case za::SeekDir::beg:
            return SEEK_SET;
        case za::SeekDir::cur:
            return SEEK_CUR;
        default:
            ZA_ASSERT(dir == za::SeekDir::end);
            return SEEK_END;
    }
}

} // namespace


////////////////////////////////////////////////////////////
struct InFile::Impl
{
    std::FILE* file;   //!< Non-null on a live `InFile`; null only in a moved-from object.
    int        peeked; //!< 1-byte `ScnSource` cache: byte value cast through `unsigned char`, or sentinel below.
};


////////////////////////////////////////////////////////////
namespace
{
// `InFile::peek()` returns a single non-negative byte or EOF; the cache
// uses `-2` as its "empty" sentinel because that value is reserved (real
// `fgetc` only ever returns `[0, 255]` or `EOF == -1`).
constexpr int peekCacheEmpty = -2;
} // namespace


////////////////////////////////////////////////////////////
InFile::InFile(za::PassKey<InFile>&&, void* file) noexcept : m_impl{static_cast<std::FILE*>(file), peekCacheEmpty}
{
}


////////////////////////////////////////////////////////////
InFile::~InFile()
{
    closeAndReport(m_impl->file, "InFile");
}


////////////////////////////////////////////////////////////
InFile::InFile(InFile&& rhs) noexcept : m_impl{rhs.m_impl->file, rhs.m_impl->peeked}
{
    rhs.m_impl->file   = nullptr;
    rhs.m_impl->peeked = peekCacheEmpty;
}


////////////////////////////////////////////////////////////
InFile& InFile::operator=(InFile&& rhs) noexcept
{
    if (&rhs == this)
        return *this;

    closeAndReport(m_impl->file, "InFile");

    m_impl->file       = rhs.m_impl->file;
    m_impl->peeked     = rhs.m_impl->peeked;
    rhs.m_impl->file   = nullptr;
    rhs.m_impl->peeked = peekCacheEmpty;

    return *this;
}


////////////////////////////////////////////////////////////
za::Optional<InFile> InFile::open(const Path& filename, FileOpenMode mode)
{
    std::FILE* const file = openFile(filename, mapInFileOpenMode(mode));
    if (file == nullptr)
        return za::nullOpt;

    // `FileOpenMode::ate`: seek to end on open, matching `std::ios::ate`.
    // Callers query `tellPos()` immediately afterward to learn the file size.
    if ((mode & FileOpenMode::ate) != FileOpenMode::none)
    {
        if (std::fseek(file, 0, SEEK_END) != 0)
        {
            // Open succeeded but ate-seek failed; close out and report failure
            // so the caller doesn't see a half-initialized handle.
            (void)std::fclose(file);
            return za::nullOpt;
        }
    }

    return za::makeOptional<InFile>(za::PassKey<InFile>{}, file);
}


////////////////////////////////////////////////////////////
bool InFile::read(char* data, za::SizeT size, za::SizeT& bytesRead)
{
    if (m_impl->file == nullptr)
        return false;

    if (size == 0u)
    {
        bytesRead = 0u;
        return true;
    }

    za::SizeT prefix = 0u;

    // Deliver the peek-cached byte (if any) as the first byte of the read.
    if (m_impl->peeked != peekCacheEmpty && m_impl->peeked != EOF)
    {
        *data++ = static_cast<char>(static_cast<unsigned char>(m_impl->peeked));
        ++prefix;
        --size;
    }
    m_impl->peeked = peekCacheEmpty;

    if (size == 0u)
    {
        bytesRead = prefix;
        return true;
    }

    const za::SizeT got = std::fread(data, 1u, size, m_impl->file);

    // Short read = either EOF (`feof`) or true I/O error (`ferror`). The
    // first is normal -- the caller distinguishes via `isEOF()` after a
    // short read. The second is the failure we report.
    if (got != size && std::ferror(m_impl->file) != 0)
        return false;

    bytesRead = prefix + got;
    return true;
}


////////////////////////////////////////////////////////////
bool InFile::seekPos(za::PtrDiffT absolutePos)
{
    if (m_impl->file == nullptr)
        return false;

    // Drop any peek-cached byte: after seeking, the cached byte's position
    // is no longer adjacent to the next read.
    m_impl->peeked = peekCacheEmpty;

    return std::fseek(m_impl->file, static_cast<long>(absolutePos), SEEK_SET) == 0;
}


////////////////////////////////////////////////////////////
bool InFile::seekPos(za::PtrDiffT offset, SeekDir dir)
{
    if (m_impl->file == nullptr)
        return false;

    // When a non-EOF peek cache is live, the raw FILE* cursor sits one
    // byte ahead of the logical cursor (we already pulled the cached byte
    // via `fgetc`). For `SeekDir::cur`, compensate so the seek lands at
    // the offset the caller expects. `tellPos` performs the symmetric
    // adjustment at line ~910.
    if (dir == SeekDir::cur && m_impl->peeked != peekCacheEmpty && m_impl->peeked != EOF)
        offset -= 1;

    m_impl->peeked = peekCacheEmpty;

    return std::fseek(m_impl->file, static_cast<long>(offset), mapSeekDirToStdio(dir)) == 0;
}


////////////////////////////////////////////////////////////
bool InFile::tellPos(za::PtrDiffT& out)
{
    if (m_impl->file == nullptr)
        return false;

    const long pos = std::ftell(m_impl->file);
    if (pos < 0)
        return false;

    // A peeked-but-unconsumed byte sits one position "before" the FILE's
    // logical cursor (we already pulled it via `fgetc`). Adjust so callers
    // see a stable cursor across peek calls.
    const long adj = (m_impl->peeked != peekCacheEmpty && m_impl->peeked != EOF) ? 1 : 0;

    out = static_cast<za::PtrDiffT>(pos - adj);
    return true;
}


////////////////////////////////////////////////////////////
bool InFile::isEOF() const noexcept
{
    // While a peek-cached byte exists, we have not yet "reached" EOF from
    // the caller's point of view, even if the FILE's internal EOF flag is set
    // (which can happen if `fgetc` returned a byte but the next call would EOF).
    if (m_impl->peeked != peekCacheEmpty && m_impl->peeked != EOF)
        return false;

    return m_impl->file != nullptr && std::feof(m_impl->file) != 0;
}


////////////////////////////////////////////////////////////
za::Optional<char> InFile::peek()
{
    if (m_impl->file == nullptr)
        return za::nullOpt;

    if (m_impl->peeked == peekCacheEmpty)
        m_impl->peeked = std::fgetc(m_impl->file);

    if (m_impl->peeked == EOF)
        return za::nullOpt;

    return za::makeOptional(static_cast<char>(static_cast<unsigned char>(m_impl->peeked)));
}


////////////////////////////////////////////////////////////
void InFile::consume() noexcept
{
    // Pre: a successful `peek()` preceded this call. Drop the cache so
    // the next `peek()` pulls a fresh byte.
    m_impl->peeked = peekCacheEmpty;
}


} // namespace za
