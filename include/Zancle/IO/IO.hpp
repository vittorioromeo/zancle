#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Config.hpp" // IWYU pragma: keep

#include "Zancle/Vocabulary/EnumClassBitwiseOps.hpp"
#include "Zancle/Vocabulary/InPlacePImpl.hpp"
#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"

#include "Zancle/Base/FwdStdString.hpp" // TODO P1: remove?
#include "Zancle/Base/PtrDiffT.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class String;
class StringView;

template <typename TItem>
class Vector;
} // namespace za

namespace za
{
class Path;
}


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Helper function to write to a file.
///
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_SYSTEM_API bool writeToFile(za::StringView filename, za::StringView contents);
[[nodiscard]] ZA_SYSTEM_API bool writeToFile(const Path& filename, za::StringView contents);


////////////////////////////////////////////////////////////
/// \brief Helper function to read the contents of a file
///
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_SYSTEM_API bool readFromFile(za::StringView filename, std::string& target);
[[nodiscard]] ZA_SYSTEM_API bool readFromFile(za::StringView filename, za::String& target);
[[nodiscard]] ZA_SYSTEM_API bool readFromFile(za::StringView filename, za::Vector<char>& target);
[[nodiscard]] ZA_SYSTEM_API bool readFromFile(const Path& filename, std::string& target);
[[nodiscard]] ZA_SYSTEM_API bool readFromFile(const Path& filename, za::String& target);
[[nodiscard]] ZA_SYSTEM_API bool readFromFile(const Path& filename, za::Vector<char>& target);


////////////////////////////////////////////////////////////
/// \brief Append the contents of a file to a `za::Vector<char>`.
///
/// Like `readFromFile(..., za::Vector<char>&)` but preserves the existing
/// content of `target` and writes the file's bytes after it. The new range
/// after a successful call is `target.data()[oldSize, oldSize + fileSize)`.
/// Skips zero-init via `unsafeSetSize`. On failure, `target` may be in a
/// partially-grown state; existing content up to `oldSize` is unchanged.
///
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_SYSTEM_API bool appendFromFile(za::StringView filename, za::Vector<char>& target);
[[nodiscard]] ZA_SYSTEM_API bool appendFromFile(const Path& filename, za::Vector<char>& target);


////////////////////////////////////////////////////////////
/// \brief Returns a thread-local scratch `za::Vector<char>` for transient file I/O.
///
/// The buffer is recycled across calls on the same thread so its capacity
/// grows monotonically toward the largest file ever loaded; subsequent loads
/// of the same or smaller size do not allocate. Intended primarily as the
/// destination passed to `readFromFile(StringView, za::Vector<char>&)` in
/// resource-loading code paths.
///
/// \warning This API is **NOT re-entrant**. The returned reference must be
///          fully consumed and released before any function on the same
///          thread (directly or transitively) requests this buffer again --
///          otherwise the second user will overwrite the first user's data.
///          In practice: hold the reference in a single tight scope, do not
///          pass it across calls into user code or third-party libraries
///          that might also use it. Loaders that decode in place from the
///          returned bytes (e.g. `stbi_load_from_memory`) are safe; loaders
///          that themselves call `readFromFile` are not.
///
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_SYSTEM_API za::Vector<char>& getThreadLocalScratchCharBuffer();


////////////////////////////////////////////////////////////
/// \brief Flags used to specify how a file should be opened (substitute for `std::ios_base::openmode`)
///
////////////////////////////////////////////////////////////
enum class FileOpenMode
{
    none  = 0,
    app   = 1L << 0,
    ate   = 1L << 1,
    bin   = 1L << 2,
    in    = 1L << 3,
    out   = 1L << 4,
    trunc = 1L << 5,
};


////////////////////////////////////////////////////////////
ZA_DEFINE_ENUM_CLASS_BITWISE_OPS(FileOpenMode);


////////////////////////////////////////////////////////////
/// \brief Specifies the direction for seeking in a stream (substitute for `std::ios_base::seekdir`)
///
////////////////////////////////////////////////////////////
enum class SeekDir
{
    beg = 0, //!< Beginning of the stream
    cur = 1, //!< Current position in the stream
    end = 2, //!< End of the stream
};


////////////////////////////////////////////////////////////
/// \brief Output file handle (replaces `std::ofstream`).
///
/// Backed by C stdio's `FILE*` -- no iostream/streambuf machinery, no
/// locale. RAII: construction is only possible through the factory,
/// which returns `za::nullOpt` if the file can't be opened; once
/// you have an `OutFile` it represents an open file. The destructor
/// closes the underlying handle (silently -- callers cannot detect a
/// close error in this design).
///
/// Move-only. Every fallible I/O operation returns its own success
/// status via `[[nodiscard]] bool`; there is no sticky-good flag.
/// \code
///     auto opt = za::OutFile::open(path, za::FileOpenMode::bin);
///     if (!opt.hasValue()) handleOpenError();
///     auto& file = *opt;
///     if (!file.write(data, size)) handleWriteError();
///     // destructor closes
/// \endcode
///
/// Satisfies `za::AppendSink` (via `append`) for composition with
/// `za::fmtTo` / `print` / `printLn`. Errors are **not** visible
/// through the sink path because Fmt buffers internally; use `write()`
/// directly when strict error propagation is required.
///
////////////////////////////////////////////////////////////
class ZA_SYSTEM_API OutFile
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Open a file for output.
    ///
    /// \return `OutFile` on success, `za::nullOpt` on open failure.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<OutFile> open(const Path& filename, FileOpenMode mode = FileOpenMode::out);

    ~OutFile();

    OutFile(const OutFile&)            = delete;
    OutFile& operator=(const OutFile&) = delete;

    OutFile(OutFile&&) noexcept;
    OutFile& operator=(OutFile&&) noexcept;

    [[nodiscard]] bool write(const char* data, za::SizeT size);
    [[nodiscard]] bool flush();

    [[nodiscard]] bool seekPos(za::PtrDiffT absolutePos);
    [[nodiscard]] bool tellPos(za::PtrDiffT& out);

    ////////////////////////////////////////////////////////////
    /// \brief `AppendSink` adapter for `fmtTo` / `print` / `printLn`.
    /// Errors are lost in this path -- use `write()` for explicit
    /// error handling.
    ////////////////////////////////////////////////////////////
    void append(const char* data, za::SizeT n);

    ////////////////////////////////////////////////////////////
    /// \private Constructor for internal use by `open`. The
    /// `PassKey` parameter restricts who can call it.
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit OutFile(za::PassKey<OutFile>&&, void* file) noexcept;

private:
    struct Impl;
    za::InPlacePImpl<Impl, sizeof(void*)> m_impl; //!< Holds a `FILE*`.
};


////////////////////////////////////////////////////////////
/// \brief Input file handle (replaces `std::ifstream`).
///
/// Backed by C stdio's `FILE*` -- no iostream/streambuf machinery, no
/// locale. RAII: construction is only possible through the factory,
/// which returns `za::nullOpt` if the file can't be opened; once
/// you have an `InFile` it represents an open file. The destructor
/// closes the underlying handle (silently).
///
/// Move-only. Every fallible I/O operation returns its own success
/// status via `[[nodiscard]] bool`; there is no sticky-good flag.
///
/// EOF is distinct from I/O failure: `read()` returns `true` with a
/// short `bytesRead` (possibly 0) when EOF is reached without error.
/// Use `isEOF()` (matching `feof`) to detect EOF after such a read.
///
/// For full-file loads, prefer the `za::readFromFile` helpers in this
/// header -- they take native fast paths on Windows/POSIX. Use `InFile`
/// when you need streaming reads, seeking, or per-byte parsing via
/// `<Zancle/Scn/...>`.
///
/// `InFile` directly satisfies `za::ScnSource` (via `peek` /
/// `consume`), so it can be handed to `za::scn<T>` /
/// `za::scnInto` / friends without an external adapter.
///
////////////////////////////////////////////////////////////
class ZA_SYSTEM_API InFile
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Open a file for input.
    ///
    /// \return `InFile` on success, `za::nullOpt` on open failure.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<InFile> open(const Path& filename, FileOpenMode mode = FileOpenMode::in);

    ~InFile();

    InFile(const InFile&)            = delete;
    InFile& operator=(const InFile&) = delete;

    InFile(InFile&&) noexcept;
    InFile& operator=(InFile&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Read up to `size` bytes into `data`. On success returns
    /// `true` and writes the actual count to `bytesRead` -- this can
    /// be less than `size` at EOF (use `isEOF()` to distinguish from
    /// "no more available" mid-read). Returns `false` on I/O error;
    /// `bytesRead` is left untouched.
    ///
    /// A pending peek-cached byte (set by `peek()` without a matching
    /// `consume()`) is delivered as the first byte of the output and
    /// the cache is cleared.
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool read(char* data, za::SizeT size, za::SizeT& bytesRead);

    [[nodiscard]] bool seekPos(za::PtrDiffT absolutePos);
    [[nodiscard]] bool seekPos(za::PtrDiffT offset, SeekDir dir);

    [[nodiscard]] bool tellPos(za::PtrDiffT& out);

    [[nodiscard]] bool isEOF() const noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief `ScnSource`: return the next byte without consuming it,
    /// or `za::nullOpt` at EOF / on I/O error. Holds a 1-byte
    /// read-ahead cache internally so repeated `peek()` is idempotent.
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<char> peek();

    ////////////////////////////////////////////////////////////
    /// \brief `ScnSource`: drop the byte most recently returned by
    /// `peek()` -- subsequent reads start at the byte after.
    /// Undefined if not preceded by a successful `peek()`.
    ////////////////////////////////////////////////////////////
    void consume() noexcept;

    ////////////////////////////////////////////////////////////
    /// \private Constructor for internal use by `open`. The
    /// `PassKey` parameter restricts who can call it.
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit InFile(za::PassKey<InFile>&&, void* file) noexcept;

private:
    struct Impl;
    za::InPlacePImpl<Impl, sizeof(void*) * 2> m_impl; //!< `FILE*` + 1-byte peek cache (int-sized).
};


} // namespace za


////////////////////////////////////////////////////////////
/// \file Zancle/IO/IO.hpp
/// \ingroup system
///
/// File I/O: `OutFile` / `InFile`, plus the `readFromFile` /
/// `writeToFile` helpers. For formatted output to stdout/stderr, see
/// `za::print` / `printLn` / `printErr` / `printErrLn` in
/// `<Zancle/Fmt/Fmt.hpp>`. For input parsing -- in-memory or from
/// stdin -- see `<Zancle/Scn/Scn.hpp>` and
/// `<Zancle/Scn/ScnStdin.hpp>`. `InFile` itself satisfies
/// `za::ScnSource`, so it can be handed to `scn<T>` / `scnInto`
/// directly.
///
/// Implementation details are hidden behind PImpl to keep expensive
/// standard library headers out of the public API.
///
////////////////////////////////////////////////////////////
