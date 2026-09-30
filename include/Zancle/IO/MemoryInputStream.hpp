#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////


#include "Zancle/Config.hpp"

#include "Zancle/IO/InputStream.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/SizeT.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Implementation of input stream based on a memory chunk
///
////////////////////////////////////////////////////////////
class ZA_SYSTEM_API MemoryInputStream : public InputStream
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Construct from a pointer to `sizeInBytes` bytes of data
    ///
    /// `data` may be null if `sizeInBytes` is zero (e.g. an empty vector's `data()`).
    ///
    ////////////////////////////////////////////////////////////
    MemoryInputStream(const void* data, za::SizeT sizeInBytes);

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
    // Member data
    ////////////////////////////////////////////////////////////
    const unsigned char* m_data{};   //!< Pointer to the data in memory
    za::SizeT            m_size{};   //!< Total size of the data
    za::SizeT            m_offset{}; //!< Current reading position
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::MemoryInputStream
/// \ingroup system
///
/// `InputStream` specialization that wraps a chunk of in-memory data,
/// letting code written against `InputStream` operate on it.
///
/// Zancle resource classes can usually be loaded directly from memory,
/// so this is mostly useful when writing custom algorithms over
/// `InputStream`.
///
/// Usage example:
/// \code
/// void process(InputStream& stream);
///
/// MemoryInputStream stream(thePtr, theSize);
/// process(stream);
/// \endcode
///
/// \see `InputStream`, `FileInputStream`
///
////////////////////////////////////////////////////////////
