#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/Export.hpp"

#include "Zancle/Lifetime/LifetimeDependee.hpp"

#include "Zancle/Vocabulary/InPlacePImpl.hpp"
#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class ChannelMap;
class InputStream;
class Path;
class Sound;
class Time;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Storage for audio samples defining a sound
///
/// `SoundBuffer` owns a contiguous array of 16-bit signed PCM
/// samples plus the metadata needed to interpret them
/// (sample rate, channel map). It is the heavy resource that
/// `za::Sound` reads from at playback time.
///
/// Buffers can be loaded from a file, an in-memory blob, a
/// custom stream, or directly from raw samples, and saved
/// back to a file. They can be freely copied and moved.
///
/// \see `za::Sound`, `za::SoundBufferRecorder`, `za::InputSoundFile`
///
////////////////////////////////////////////////////////////
class ZA_AUDIO_API SoundBuffer
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    /// Performs a deep copy of the sample data.
    ///
    ////////////////////////////////////////////////////////////
    SoundBuffer(const SoundBuffer& rhs);

    ////////////////////////////////////////////////////////////
    /// \brief Copy assignment
    ///
    ////////////////////////////////////////////////////////////
    SoundBuffer& operator=(const SoundBuffer& rhs);

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    SoundBuffer(SoundBuffer&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    SoundBuffer& operator=(SoundBuffer&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~SoundBuffer();

    ////////////////////////////////////////////////////////////
    /// \brief Load the sound buffer from a file
    ///
    /// See the documentation of `za::InputSoundFile` for the list
    /// of supported formats.
    ///
    /// \param filename Path of the sound file to load
    ///
    /// \return Sound buffer on success, `za::nullOpt` otherwise
    ///
    /// \see `loadFromMemory`, `loadFromStream`, `loadFromSamples`, `saveToFile`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<SoundBuffer> loadFromFile(const Path& filename);

    ////////////////////////////////////////////////////////////
    /// \brief Load the sound buffer from a file in memory
    ///
    /// See the documentation of `za::InputSoundFile` for the list
    /// of supported formats.
    ///
    /// \param data        Pointer to the file data in memory
    /// \param sizeInBytes Size of the data to load, in bytes
    ///
    /// \return Sound buffer on success, `za::nullOpt` otherwise
    ///
    /// \see `loadFromFile`, `loadFromStream`, `loadFromSamples`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<SoundBuffer> loadFromMemory(const void* data, za::SizeT sizeInBytes);

    ////////////////////////////////////////////////////////////
    /// \brief Load the sound buffer from a custom stream
    ///
    /// See the documentation of `za::InputSoundFile` for the list
    /// of supported formats.
    ///
    /// \param stream Source stream to read from
    ///
    /// \return Sound buffer on success, `za::nullOpt` otherwise
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `loadFromSamples`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<SoundBuffer> loadFromStream(InputStream& stream);

    ////////////////////////////////////////////////////////////
    /// \brief Load the sound buffer from an array of audio samples
    ///
    /// The assumed format of the audio samples is 16 bit signed integer.
    ///
    /// \param samples      Pointer to the array of samples in memory
    /// \param sampleCount  Number of samples in the array
    /// \param channelMap   Map of position in sample frame to sound channel
    /// \param sampleRate   Sample rate (number of samples to play per second)
    ///
    /// \return Sound buffer on success, `za::nullOpt` otherwise
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `saveToFile`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<SoundBuffer> loadFromSamples(
        const za::I16*    samples,
        za::SizeT         sampleCount,
        const ChannelMap& channelMap,
        unsigned int      sampleRate);

    ////////////////////////////////////////////////////////////
    /// \brief Save the sound buffer to an audio file
    ///
    /// See the documentation of `za::OutputSoundFile` for the list
    /// of supported formats.
    ///
    /// \param filename Path of the sound file to write
    ///
    /// \return `true` if saving succeeded, `false` if it failed
    ///
    /// \see loadFromFile, loadFromMemory, loadFromSamples
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool saveToFile(const Path& filename) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the array of audio samples stored in the buffer
    ///
    /// The format of the returned samples is 16 bit signed integer.
    /// The total number of samples in this array is given by the
    /// `getSampleCount()` function.
    ///
    /// \return Read-only pointer to the array of sound samples
    ///
    /// \see `getSampleCount`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const za::I16* getSamples() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the number of samples stored in the buffer
    ///
    /// The array of samples can be accessed with the `getSamples()`
    /// function.
    ///
    /// \return Number of samples
    ///
    /// \see `getSamples`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::U64 getSampleCount() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the sample rate of the sound
    ///
    /// The sample rate is the number of samples played per second.
    /// The higher, the better the quality (for example, 44100
    /// samples/s is CD quality).
    ///
    /// \return Sample rate (number of samples per second)
    ///
    /// \see `getChannelCount`, `getChannelMap`, `getDuration`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getSampleRate() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the number of channels used by the sound
    ///
    /// If the sound is mono then the number of channels will
    /// be 1, 2 for stereo, etc.
    ///
    /// \return Number of channels
    ///
    /// \see `getSampleRate`, `getChannelMap`, `getDuration`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getChannelCount() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the map of position in sample frame to sound channel
    ///
    /// This is used to map a sample in the sample stream to a
    /// position during spatialization.
    ///
    /// \return Map of position in sample frame to sound channel
    ///
    /// \see `getSampleRate`, `getChannelCount`, `getDuration`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const ChannelMap& getChannelMap() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the total duration of the sound
    ///
    /// \return Sound duration
    ///
    /// \see `getSampleRate`, `getChannelCount`, `getChannelMap`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Time getDuration() const;

private:
    friend Sound;

public:
    ////////////////////////////////////////////////////////////
    /// \private
    ///
    /// \brief Construct an uninitialized sound buffer with capacity for
    ///        `sampleCount` PCM samples.
    ///
    /// The samples buffer is sized to `sampleCount` but its contents are NOT
    /// zero-initialized. The caller (typically `loadFromStream`) is expected
    /// to fill the buffer immediately via direct access. Computing duration
    /// happens during construction based on `sampleCount`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit SoundBuffer(za::PassKey<SoundBuffer>&&,
                                       za::SizeT         sampleCount,
                                       const ChannelMap& channelMap,
                                       unsigned int      sampleRate);

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 128> m_impl; //!< Implementation details

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_LIFETIME_DEPENDED_ON_BY(SoundBuffer, Sound); // `SoundBuffer` is depended on by `Sound`
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::SoundBuffer
/// \ingroup audio
///
/// A sound buffer holds the data of a sound, which is
/// an array of audio samples. A sample is a 16 bit signed integer
/// that defines the amplitude of the sound at a given time.
/// The sound is then reconstituted by playing these samples at
/// a high rate (for example, 44100 samples per second is the
/// standard rate used for playing CDs). In short, audio samples
/// are like texture pixels, and a `za::SoundBuffer` is similar to
/// a `za::Texture`.
///
/// A sound buffer can be loaded from a file (see loadFromFile()
/// for the complete list of supported formats), from memory, from
/// a custom stream (see za::InputStream) or directly from an array
/// of samples. It can also be saved back to a file.
///
/// Sound buffers alone are not very useful: they hold the audio data
/// but cannot be played. To do so, you need to use the `za::Sound` class,
/// which provides functions to play/pause/stop the sound as well as
/// changing the way it is outputted (volume, pitch, 3D position, etc...).
/// This separation allows more flexibility and better performances:
/// indeed a `za::SoundBuffer` is a heavy resource, and any operation on it
/// is slow (often too slow for real-time applications). On the other
/// side, a `za::Sound` is a lightweight object, which can use the audio data
/// of a sound buffer and change the way it is played without actually
/// modifying that data. Note that it is also possible to bind
/// several `za::Sound` instances to the same `za::SoundBuffer`.
///
/// It is important to note that the `za::Sound` instance doesn't
/// copy the buffer that it uses, it only keeps a reference to it.
/// Thus, a `za::SoundBuffer` must not be destructed while it is
/// used by a `za::Sound` (i.e. never write a function that
/// uses a local `za::SoundBuffer` instance for loading a sound).
///
/// When loading sound samples from an array, a channel map needs to be
/// provided, which specifies the mapping of the position in the sample frame
/// to the sound channel. For example when you have six samples in a frame and
/// a 5.1 sound system, the channel map defines how each of those samples map
/// to which speaker channel.
///
/// Usage example:
/// \code
/// // Load a new sound buffer from a file
/// const auto buffer = za::SoundBuffer::loadFromFile("sound.wav").value();
///
/// // Assumes `playbackDevice` is an initialized za::PlaybackDevice
///
/// // Create a sound source bound to the buffer
/// za::Sound sound1(playbackDevice, buffer);
///
/// // Play the sound
/// sound1.play();
///
/// // Create another sound source bound to the same buffer
/// za::Sound sound2(playbackDevice, buffer);
///
/// // Play it with a higher pitch -- the first sound remains unchanged
/// sound2.setPitch(2);
/// sound2.play();
///
/// // Load samples with a channel map
/// auto samples = std::vector<std::int16_t>();
/// // ...
/// auto channelMap = za::ChannelMap{
///     za::SoundChannel::FrontLeft,
///     za::SoundChannel::FrontCenter,
///     za::SoundChannel::FrontRight,
///     za::SoundChannel::BackRight,
///     za::SoundChannel::BackLeft,
///     za::SoundChannel::LowFrequencyEffects
/// };
/// auto soundBuffer = za::SoundBuffer::loadFromSamples(
///     samples.data(), samples.size(), channelMap, 44100).value();
/// za::Sound sound(playbackDevice, soundBuffer);
/// \endcode
///
/// \see `za::Sound`, `za::SoundBufferRecorder`
///
////////////////////////////////////////////////////////////
