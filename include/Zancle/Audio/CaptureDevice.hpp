#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/Export.hpp"

#include "Zancle/Audio/ChannelMap.hpp"

#include "Zancle/Lifetime/LifetimeDependee.hpp"

#include "Zancle/Vocabulary/InPlacePImpl.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class CaptureDeviceHandle;
class SoundRecorder;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Opened audio capture device used to record sound
///
/// A `CaptureDevice` represents a live, opened audio input
/// (microphone, line-in, virtual source, etc.). It owns the
/// underlying miniaudio capture device and is the entity that
/// `za::SoundRecorder` instances pull recorded samples from.
///
/// A capture device is created from a `CaptureDeviceHandle`
/// obtained via `za::AudioContext`:
/// \code
/// auto audioContext  = za::AudioContext::create().value();
/// auto handle        = za::AudioContext::getDefaultCaptureDeviceHandle().value();
/// za::CaptureDevice  captureDevice{handle};
/// \endcode
///
/// `CaptureDevice` is non-copyable and non-movable: any
/// `za::SoundRecorder` started on it holds a non-owning
/// reference, so the device must outlive every recorder bound
/// to it.
///
/// Recordings are always 16-bit signed PCM. Currently only mono
/// (1 channel) and stereo (2 channels) recording layouts are
/// supported.
///
/// \see `za::AudioContext`, `za::CaptureDeviceHandle`, `za::SoundRecorder`, `za::SoundBufferRecorder`
///
////////////////////////////////////////////////////////////
class ZA_AUDIO_API CaptureDevice
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Open a capture device from a device handle
    ///
    /// Initializes the underlying miniaudio capture device. The
    /// handle is typically obtained from `za::AudioContext`.
    ///
    /// On failure to initialize, an error is logged via `za::err`
    /// and the device is left in an uninitialized state.
    ///
    /// \param deviceHandle Handle identifying the device to open
    ///
    ////////////////////////////////////////////////////////////
    explicit CaptureDevice(const CaptureDeviceHandle& deviceHandle);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Destroys the underlying device. The device must already
    /// be stopped (i.e. no `za::SoundRecorder` may still be
    /// recording on it).
    ///
    ////////////////////////////////////////////////////////////
    ~CaptureDevice();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    CaptureDevice(const CaptureDevice&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    CaptureDevice& operator=(const CaptureDevice&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted move constructor
    ///
    ////////////////////////////////////////////////////////////
    CaptureDevice(CaptureDevice&&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted move assignment
    ///
    ////////////////////////////////////////////////////////////
    CaptureDevice& operator=(CaptureDevice&&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Get the handle of the device that was opened
    ///
    /// \return Reference to the device handle this device was
    ///         constructed from
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const CaptureDeviceHandle& getDeviceHandle() const;

    ////////////////////////////////////////////////////////////
    /// \brief Set the sample rate of the capture device
    ///
    /// Changing the sample rate requires re-initializing the
    /// underlying miniaudio device, so this should not be called
    /// while a `za::SoundRecorder` is actively recording on this
    /// device.
    ///
    /// \param sampleRate Desired number of samples per second (e.g. 44100 for CD quality)
    ///
    /// \return `true` on success, `false` on failure
    ///
    /// \see `getSampleRate`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool setSampleRate(unsigned int sampleRate);

    ////////////////////////////////////////////////////////////
    /// \brief Get the current sample rate of the capture device
    ///
    /// The sample rate defines the number of audio samples
    /// captured per second. The higher, the better the quality
    /// (for example, 44100 samples/sec is CD quality). The
    /// default is 44100 Hz.
    ///
    /// \return Sample rate, in samples per second
    ///
    /// \see `setSampleRate`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getSampleRate() const;

    ////////////////////////////////////////////////////////////
    /// \brief Set the channel count of the audio capture device
    ///
    /// This method allows you to specify the number of channels
    /// used for recording. Currently only mono (1) and stereo
    /// (2) are supported. Any other value will fail.
    ///
    /// Changing the channel count requires re-initializing the
    /// underlying miniaudio device, so this should not be called
    /// while a `za::SoundRecorder` is actively recording on this
    /// device.
    ///
    /// The channel map is updated automatically: 1 channel maps
    /// to `Mono`, 2 channels maps to `{FrontLeft, FrontRight}`.
    ///
    /// \param channelCount Number of channels (1 for mono, 2 for stereo)
    ///
    /// \return `true` on success, `false` on failure (unsupported value or device error)
    ///
    /// \see `getChannelCount`, `getChannelMap`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool setChannelCount(unsigned int channelCount);

    ////////////////////////////////////////////////////////////
    /// \brief Get the number of channels used by this capture device
    ///
    /// Currently only mono and stereo are supported, so the
    /// value is either 1 (for mono) or 2 (for stereo). The
    /// default is 1 (mono).
    ///
    /// \return Number of channels
    ///
    /// \see `setChannelCount`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getChannelCount() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the map of position in sample frame to sound channel
    ///
    /// This is used to map a sample in the sample stream to a
    /// position during spatialization. It is updated automatically
    /// by `setChannelCount`.
    ///
    /// \return Map of position in sample frame to sound channel
    ///
    ////////////////////////////////////////////////////////////
    const ChannelMap& getChannelMap() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the human-readable name of the opened device
    ///
    /// \return Null-terminated UTF-8 device name
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const char* getName() const;

    ////////////////////////////////////////////////////////////
    /// \brief Check whether the opened device is the system default
    ///
    /// \return `true` if this device is the system's default
    ///         capture device, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isDefault() const;

private:
    friend SoundRecorder;

    ////////////////////////////////////////////////////////////
    /// \brief Check whether the underlying miniaudio device is initialized
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isDeviceInitialized() const;

    ////////////////////////////////////////////////////////////
    /// \brief Check whether the underlying miniaudio device is currently capturing
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isDeviceStarted() const;

    ////////////////////////////////////////////////////////////
    /// \brief Try to start the underlying miniaudio capture device
    ///
    /// \return `true` on success, `false` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool startDevice();

    ////////////////////////////////////////////////////////////
    /// \brief Try to stop the underlying miniaudio capture device
    ///
    /// \return `true` on success, `false` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool stopDevice();

    ////////////////////////////////////////////////////////////
    /// \brief Register the per-chunk processing callback used by miniaudio
    ///
    /// Used by `za::SoundRecorder` to wire its `onProcessSamples`
    /// hook into the device's data callback.
    ///
    ////////////////////////////////////////////////////////////
    using ProcessSamplesFunc = bool (*)(void* userData, const za::I16* samples, za::SizeT sampleCount);
    void setProcessSamplesFunc(SoundRecorder* soundRecorder, ProcessSamplesFunc processSamplesFunc);

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 5728> m_impl; //!< Implementation details

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_LIFETIME_DEPENDED_ON_BY(CaptureDevice, SoundRecorder); // `CaptureDevice` is depended on by `SoundRecorder`
};

} // namespace za
