#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/Export.hpp"

#include "Zancle/Lifetime/LifetimeDependee.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"
#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class ChannelMap;
class InputSoundFile;
class InputStream;
class Music;
class Path;
class Time;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Streamed sound source intended to back one or more `za::Music` instances
///
/// `MusicReader` owns a thread-safe wrapper around an
/// `za::InputSoundFile` and exposes the metadata required by
/// `za::Music` (sample rate, channel count, channel map,
/// duration), plus a single `seekAndRead` operation used by
/// the streaming thread to pull new chunks.
///
/// Splitting the reader from the playback object means that:
///   - the underlying file/stream/memory can outlive any
///     individual `za::Music` instance,
///   - and the same source can be reused (one at a time) by
///     several `za::Music` instances over its lifetime.
///
/// Use the `openFromFile`, `openFromMemory`, or
/// `openFromStream` factories to construct a reader, then pass
/// it to `za::Music`.
///
/// \see `za::Music`, `za::InputSoundFile`
///
////////////////////////////////////////////////////////////
class ZA_AUDIO_API MusicReader
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~MusicReader();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    MusicReader(const MusicReader&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    MusicReader& operator=(const MusicReader&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    MusicReader(MusicReader&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    MusicReader& operator=(MusicReader&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Open a music source from an audio file
    ///
    /// This function doesn't start playing the music (use `play`
    /// to do so).
    ///
    /// See the documentation of `za::InputSoundFile` for the list
    /// of supported formats.
    ///
    /// \warning Since the music is not loaded at once but rather
    /// streamed continuously, the file must remain accessible until
    /// the `za::Music` object loads a new music or is destroyed
    /// and until all active `za::Music` objects linked to this
    /// `za::Music` instance are destroyed.
    ///
    /// \param filename Path of the music file to open
    ///
    /// \return Music source if loading succeeded, `za::nullOpt` if it failed
    ///
    /// \see `openFromMemory`, `openFromStream`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<MusicReader> openFromFile(const Path& filename);

    ////////////////////////////////////////////////////////////
    /// \brief Open a music source from an audio file in memory
    ///
    /// This function doesn't start playing the music (use `play`
    /// to do so).
    ///
    /// See the documentation of `za::InputSoundFile` for the list
    /// of supported formats.
    ///
    /// \warning Since the music is not loaded at once but rather streamed
    /// continuously, the `data` buffer must remain accessible until
    /// the `za::Music` object loads a new music or is destroyed. That is,
    /// you can't deallocate the buffer right after calling this function.
    ///
    /// \param data        Pointer to the file data in memory
    /// \param sizeInBytes Size of the data to load, in bytes
    ///
    /// \return Music source if loading succeeded, `za::nullOpt` if it failed
    ///
    /// \see `openFromFile`, `openFromStream`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<MusicReader> openFromMemory(const void* data, za::SizeT sizeInBytes);

    ////////////////////////////////////////////////////////////
    /// \brief Open a music source from an audio file in a custom stream
    ///
    /// This function doesn't start playing the music (use `play`
    /// to do so).
    ///
    /// See the documentation of `za::InputSoundFile` for the list
    /// of supported formats.
    ///
    /// \warning Since the music is not loaded at once but rather
    /// streamed continuously, the `stream` must remain accessible
    /// until the `za::Music` object loads a new music or is destroyed.
    ///
    /// \param stream Source stream to read from
    ///
    /// \return Music source if loading succeeded, `za::nullOpt` if it failed
    ///
    /// \see `openFromFile`, `openFromMemory`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<MusicReader> openFromStream(InputStream& stream);

    ////////////////////////////////////////////////////////////
    /// \brief Get the total duration of the music source
    ///
    /// \return Music duration
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Time getDuration() const;

    ////////////////////////////////////////////////////////////
    /// \brief Return the number of channels of the music source
    ///
    /// 1 channel means a mono sound, 2 means stereo, etc.
    ///
    /// \return Number of channels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getChannelCount() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the stream sample rate of the music source
    ///
    /// The sample rate is the number of audio samples played per
    /// second. The higher, the better the quality.
    ///
    /// \return Sample rate, in number of samples per second
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getSampleRate() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the map of position in sample frame to sound channel
    ///
    /// This is used to map a sample in the sample stream to a
    /// position during spatialization.
    ///
    /// \return Map of position in sample frame to sound channel
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const ChannelMap& getChannelMap() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the total number of audio samples in the music source
    ///
    /// \return Number of samples
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::U64 getSampleCount() const;

    ////////////////////////////////////////////////////////////
    /// \brief Result of a combined `seekAndRead` operation
    ///
    ////////////////////////////////////////////////////////////
    struct [[nodiscard]] SeekAndReadResult
    {
        za::U64 sampleOffset; //!< Sample offset of the underlying file after the seek
        za::U64 samplesRead;  //!< Number of samples actually read into the output buffer
    };

    ////////////////////////////////////////////////////////////
    /// \brief Atomically seek to a sample offset and read a chunk of samples
    ///
    /// This is the operation used by `za::Music`'s streaming
    /// thread to pull new audio chunks from the reader. It is
    /// internally synchronized so that multiple `za::Music`
    /// instances cannot interleave their reads against the
    /// shared underlying `InputSoundFile`.
    ///
    /// Samples are written as interleaved 16-bit signed PCM.
    ///
    /// \param sampleOffset Sample offset to seek to before reading
    /// \param samples      Pointer to the output buffer to fill
    /// \param maxCount     Maximum number of samples to read
    ///
    /// \return Post-seek sample offset and the number of samples actually read
    ///
    /// \see `InputSoundFile::seek`, `InputSoundFile::read`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] SeekAndReadResult seekAndRead(za::U64 sampleOffset, za::I16* samples, za::U64 maxCount);

    ////////////////////////////////////////////////////////////
    /// \private
    ///
    /// \brief Initialize the internal state after loading a new music
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit MusicReader(za::PassKey<MusicReader>&&, InputSoundFile&& file);

private:
    ////////////////////////////////////////////////////////////
    friend Music;

    ////////////////////////////////////////////////////////////
    /// \brief Try opening the music source from an optional input sound file
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<MusicReader> tryOpenFromInputSoundFile(za::Optional<InputSoundFile>&& optFile,
                                                                             const char* errorContext);


    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::UniquePtr<Impl> m_impl; //!< Implementation details
    // TODO P0: can the uptr be avoided here? the problem is the mutex

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_LIFETIME_DEPENDED_ON_BY(MusicReader, Music); // `MusicReader` is depended on by `Music`
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::MusicReader
/// \ingroup audio
///
/// `za::MusicReader` is the streamed counterpart to
/// `za::SoundBuffer`: it owns the long-lived audio source
/// (file, in-memory blob, or custom stream) and exposes the
/// metadata and chunked-read interface that `za::Music` needs
/// to play it back without loading it all into memory.
///
/// Because the music is streamed on demand, the underlying
/// resource (path, memory buffer, or stream) must remain
/// valid for the entire lifetime of the reader, and the
/// reader itself must outlive every `za::Music` constructed
/// from it.
///
/// Usage example:
/// \code
/// auto reader = za::MusicReader::openFromFile("music.ogg").value();
/// za::Music   music{playbackDevice, reader};
/// music.play();
/// \endcode
///
/// \see `za::Music`, `za::InputSoundFile`
///
////////////////////////////////////////////////////////////
