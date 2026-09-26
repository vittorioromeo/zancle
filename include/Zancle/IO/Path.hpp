#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Config.hpp"

#include "Zancle/String/StringView.hpp"

#include "Zancle/Vocabulary/FunctionRef.hpp"
#include "Zancle/Vocabulary/InPlacePImpl.hpp"
#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/IntTypes.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Cross-platform filesystem path
/// \ingroup system
///
/// PImpl wrapper around `std::filesystem::path` so that `<filesystem>` does not leak.
///
/// All filesystem operations are non-throwing: `bool` results signal success or
/// failure, and operations that produce a new `Path` (`absolute`, `tempDirectoryPath`)
/// return `za::Optional<Path>` so OS errors can be propagated without exceptions.
///
/// For stream insertion (`std::ostream << Path`), include `Zancle/IO/PathStreamOp.hpp`.
///
////////////////////////////////////////////////////////////
class [[nodiscard]] ZA_SYSTEM_API Path
{
public:
#if defined(ZA_SYSTEM_EMSCRIPTEN) || defined(ZA_SYSTEM_LINUX_OR_BSD)
    using value_type = char;
#else
    using value_type = wchar_t;
#endif

    ////////////////////////////////////////////////////////////
    /// \brief Path to the system's temporary directory; empty `Optional` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Path> getTempDirectory();

    ////////////////////////////////////////////////////////////
    /// \brief Process current working directory; empty `Optional` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Path> getCurrentDirectory();

    ////////////////////////////////////////////////////////////
    /// \brief Set the process current working directory
    ///
    /// \return `true` on success, `false` on any OS error
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static bool setCurrentDirectory(const Path& path);

    ////////////////////////////////////////////////////////////
    /// \brief User's home directory (`$HOME` on POSIX, `%USERPROFILE%` on Windows)
    ///
    /// Returns empty `Optional` if the environment variable is unset.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Path> getHomeDirectory();

    ////////////////////////////////////////////////////////////
    /// \brief Default-construct an empty path
    ///
    ////////////////////////////////////////////////////////////
    /* implicit */ Path();

    ////////////////////////////////////////////////////////////
    /// \brief Construct from a string-like or `std::filesystem::path` source
    ///
    ////////////////////////////////////////////////////////////
    template <typename T>
    /* implicit */ Path(const T& source);

    ////////////////////////////////////////////////////////////
    /// \brief Construct from a null-terminated `T*` (e.g. `const char*`, `const wchar_t*`)
    ///
    ////////////////////////////////////////////////////////////
    template <typename T>
    /* implicit */ Path(const T* source);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~Path();

    ////////////////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    ////////////////////////////////////////////////////////////
    Path(const Path&);

    ////////////////////////////////////////////////////////////
    /// \brief Copy assignment operator
    ///
    ////////////////////////////////////////////////////////////
    Path& operator=(const Path&);

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    Path(Path&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment operator
    ///
    ////////////////////////////////////////////////////////////
    Path& operator=(Path&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Filename component (e.g. `/foo/bar.txt` -> `bar.txt`)
    ///
    /// Returns an empty path when this path ends with a separator
    /// (e.g. `/foo/`) or when this path is empty.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Path getFilename() const;

    ////////////////////////////////////////////////////////////
    /// \brief Filename without final extension (e.g. `/foo/bar.tar.gz` -> `bar.tar`)
    ///
    /// For dot-files like `.bashrc` the leading dot is part of the stem
    /// (so `.bashrc`'s stem is `.bashrc`, not the empty path).
    /// `.` and `..` filenames return themselves.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Path getStem() const;

    ////////////////////////////////////////////////////////////
    /// \brief Extension of the filename component (e.g. `/foo/bar.txt` -> `.txt`); empty path if none
    ///
    /// Follows `std::filesystem::path::extension` rules:
    /// - filename `.` or `..` -> empty path
    /// - filename starting with `.` and containing no other `.` -> empty path (it's a stem)
    /// - otherwise the extension begins at the rightmost `.` in the filename
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Path getExtension() const;

    ////////////////////////////////////////////////////////////
    /// \brief Absolute version of this path; empty `Optional` on OS failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<Path> getAbsolute() const;

    ////////////////////////////////////////////////////////////
    /// \brief Parent directory component (e.g. `/foo/bar.txt` -> `/foo`)
    ///
    /// Returns an empty path for paths with no separator (e.g. `bar.txt`)
    /// or for the empty path.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Path getParent() const;

    ////////////////////////////////////////////////////////////
    /// \brief Pointer to a null-terminated OS-native string (`wchar_t` on Windows, `char` on POSIX)
    ///
    /// Lifetime is tied to this `Path` object: the pointer is invalidated by
    /// destruction, by mutation (`operator/=`, `operator+=`, copy/move-assignment),
    /// or by moving from `*this`.
    ///
    /// \warning The encoding is platform-dependent. Pass this only to OS APIs
    ///          (CreateFileW, open, ...). For cross-platform code or non-OS APIs,
    ///          prefer `to<std::string>()` (UTF-8) or `to<std::wstring>()`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const value_type* c_str() const; // NOLINT(readability-identifier-naming)

    ////////////////////////////////////////////////////////////
    /// \brief Convert to `T` (`std::string`, `std::wstring`, `std::u8string`, `std::u32string`, `std::filesystem::path`)
    ///
    ////////////////////////////////////////////////////////////
    template <typename T>
    [[nodiscard]] T to() const;

    ////////////////////////////////////////////////////////////
    /// \brief `true` if the path is empty
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool empty() const;

    ////////////////////////////////////////////////////////////
    /// \brief `true` if the path refers to an existing file or directory
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool exists() const;

    ////////////////////////////////////////////////////////////
    /// \brief `true` if the path refers to an existing directory (after symlink resolution)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isDirectory() const;

    ////////////////////////////////////////////////////////////
    /// \brief `true` if the path refers to an existing regular file (after symlink resolution)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isRegularFile() const;

    ////////////////////////////////////////////////////////////
    /// \brief `true` if the path itself is a symbolic link (no resolution)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isSymlink() const;

    ////////////////////////////////////////////////////////////
    /// \brief Size of the regular file at this path, in bytes; empty `Optional` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<za::U64> getFileSize() const;

    ////////////////////////////////////////////////////////////
    /// \brief Last write time, as seconds since the Unix epoch; empty `Optional` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<za::I64> getLastWriteTimeSecondsSinceEpoch() const;

    ////////////////////////////////////////////////////////////
    /// \brief `true` if the path's extension matches `str` (ASCII case-insensitive)
    ///
    /// Uses the same rules as `getExtension()` to identify the extension,
    /// then compares it byte-for-byte against `str` with ASCII-only
    /// case folding (locale-independent).
    /// An empty `str` matches paths with no extension.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool extensionIs(za::StringView str) const;

    ////////////////////////////////////////////////////////////
    /// \brief `true` if this path has a parent directory component
    ///
    /// Mirrors `std::filesystem::path::has_parent_path`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool hasParent() const;

    ////////////////////////////////////////////////////////////
    /// \brief Remove the file or empty (directory) at this path
    ///
    /// Non-throwing: returns `false` on any failure (file does not exist,
    /// permission denied, directory not empty, ...).
    ///
    /// \return `true` if a filesystem entry was removed, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool removeFromDisk() const;

    ////////////////////////////////////////////////////////////
    /// \brief Copy the file at this path to `path`
    ///
    /// Non-throwing. Fails (returns `false`) if the source does not exist,
    /// the destination already exists, or any OS-level error occurs.
    ///
    /// \return `true` on success, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool copyFileTo(const Path& path) const;

    ////////////////////////////////////////////////////////////
    /// \brief Create the directory at this path (parent must already exist)
    ///
    /// Non-throwing. Returns `false` if the parent directory is missing or any
    /// OS error occurs. If the directory already exists, returns `false` (no-op).
    /// For idempotent creation, use `createDirectoryTree()` instead.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool createLeafDirectory() const;

    ////////////////////////////////////////////////////////////
    /// \brief Create the directory at this path together with all missing parents
    ///
    /// Non-throwing and idempotent: returns `true` whether the directory was
    /// freshly created or already existed. Returns `false` only on OS error.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool createDirectoryTree() const;

    ////////////////////////////////////////////////////////////
    /// \brief Rename or move the file/directory at this path to `target`
    ///
    /// Non-throwing. Returns `false` if the source does not exist, the target
    /// is on a different filesystem with no fallback, or any OS-level error occurs.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool renameTo(const Path& target) const;

    ////////////////////////////////////////////////////////////
    /// \brief Iterate over the entries of the directory at this path
    ///
    /// Calls `callback` once per entry (non-recursive, no order guarantees).
    /// `callback` receives the absolute or relative path of each entry depending
    /// on this path's form. Symlinks are not followed for iteration.
    ///
    /// \return `true` if the directory was successfully opened and iterated,
    ///         `false` on OS error (including: this path is not a directory).
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool forEachEntry(za::FunctionRef<void(const Path&)> callback) const;

    ////////////////////////////////////////////////////////////
    /// \brief Append `rhs` to this path, inserting a directory separator if needed
    ///
    ////////////////////////////////////////////////////////////
    Path& operator/=(const Path& rhs);

    ////////////////////////////////////////////////////////////
    /// \brief Join two paths with a directory separator into a new `Path`
    ///
    ////////////////////////////////////////////////////////////
    friend ZA_SYSTEM_API Path operator/(const Path& lhs, const Path& rhs);

    ////////////////////////////////////////////////////////////
    /// \brief Join two paths with a directory separator into a new `Path` (rvalue)
    ///
    ////////////////////////////////////////////////////////////
    friend ZA_SYSTEM_API Path operator/(Path&& lhs, const Path& rhs);

    ////////////////////////////////////////////////////////////
    /// \brief Concatenate `rhs` to this path *without* inserting a separator
    ///
    /// Use this to append an extension or suffix (e.g. `Path("foo") += ".bak"`).
    /// For directory joining, use `operator/=` instead.
    ///
    ////////////////////////////////////////////////////////////
    Path& operator+=(const Path& rhs);

    ////////////////////////////////////////////////////////////
    /// \brief Concatenate two paths *without* inserting a separator
    ///
    ////////////////////////////////////////////////////////////
    friend ZA_SYSTEM_API Path operator+(const Path& lhs, const Path& rhs);

    ////////////////////////////////////////////////////////////
    /// \brief Compare two paths for equality (`!=` is synthesized by the compiler)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool operator==(const Path& rhs) const;

    ////////////////////////////////////////////////////////////
    /// \brief Constructs a `Path` from `str` and compares it for equality with the current path
    ///
    ////////////////////////////////////////////////////////////
    template <typename T>
    [[nodiscard]] bool operator==(const T* str) const;

private:
    ////////////////////////////////////////////////////////////
    /// \brief Private constructor for internal use
    ///
    /// \param fsPath The filesystem path to wrap.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit Path(int, const void* fsPath);

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 64> m_impl;
};


} // namespace za
