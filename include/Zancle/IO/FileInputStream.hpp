#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Config.hpp"

#include "Zancle/IO/InputStream.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"
#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Base/SizeT.hpp"

#include <cstdio> // TODO P2: try to remove


#ifdef ZA_SYSTEM_ANDROID
namespace za::priv
{
class ZA_SYSTEM_API ResourceStream;
}
#endif


namespace za
{
class Path;

////////////////////////////////////////////////////////////
/// \brief Implementation of input stream based on a file
///
////////////////////////////////////////////////////////////
class ZA_SYSTEM_API FileInputStream : public InputStream
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default destructor
    ///
    ////////////////////////////////////////////////////////////
    ~FileInputStream() override;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    FileInputStream(const FileInputStream&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    FileInputStream& operator=(const FileInputStream&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    FileInputStream(FileInputStream&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    FileInputStream& operator=(FileInputStream&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Open the file at `filename` for reading
    ///
    /// \return Stream on success, `za::nullOpt` on error
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<FileInputStream> open(const Path& filename);

    ////////////////////////////////////////////////////////////
    /// \brief Read up to `size` bytes into `data`, advancing the reading position
    ///
    /// \return Number of bytes actually read, or `za::nullOpt` on error
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<za::SizeT> read(void* data, za::SizeT size) override;

    ////////////////////////////////////////////////////////////
    /// \brief Move the reading position to `position` (offset from the beginning)
    ///
    /// \return Position actually sought to, or `za::nullOpt` on error
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<za::SizeT> seek(za::SizeT position) override;

    ////////////////////////////////////////////////////////////
    /// \brief Current reading position, or `za::nullOpt` on error
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<za::SizeT> tell() override;

    ////////////////////////////////////////////////////////////
    /// \brief Total number of bytes in the stream, or `za::nullOpt` on error
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<za::SizeT> getSize() override;

private:
    ////////////////////////////////////////////////////////////
    /// \brief Deleter that closes the wrapped stdio file
    ///
    ////////////////////////////////////////////////////////////
    struct ZA_SYSTEM_API FileCloser
    {
        void operator()(std::FILE* file);
    };

public:
    ////////////////////////////////////////////////////////////
    /// \private
    ////////////////////////////////////////////////////////////
    explicit FileInputStream(za::PassKey<FileInputStream>&&, za::UniquePtr<std::FILE, FileCloser>&& file);

#ifdef ZA_SYSTEM_ANDROID
    ////////////////////////////////////////////////////////////
    /// \private
    ////////////////////////////////////////////////////////////
    explicit FileInputStream(za::PassKey<FileInputStream>&&, za::UniquePtr<priv::ResourceStream>&& androidFile);
#endif

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
#ifdef ZA_SYSTEM_ANDROID
    za::UniquePtr<priv::ResourceStream> m_androidFile;
#endif

    za::UniquePtr<std::FILE, FileCloser> m_file; //!< stdio file stream
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::FileInputStream
/// \ingroup system
///
/// `InputStream` specialization that reads from a file on disk.
///
/// Zancle resource classes can usually be loaded directly from a filename,
/// so this is mostly useful when writing custom algorithms over
/// `InputStream`.
///
/// Usage example:
/// \code
/// void process(InputStream& stream);
///
/// za::Optional stream = za::FileInputStream::open("some_file.dat");
/// if (stream)
///    process(*stream);
/// \endcode
///
/// \see `InputStream`, `MemoryInputStream`
///
////////////////////////////////////////////////////////////
