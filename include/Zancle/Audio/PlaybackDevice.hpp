#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/Export.hpp"

#include "Zancle/Lifetime/LifetimeDependee.hpp"

#include "Zancle/Vocabulary/InPlacePImpl.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za::priv::MiniaudioUtils
{
struct SoundBase;
} // namespace za::priv::MiniaudioUtils

namespace za
{
class PlaybackDeviceHandle;
class Sound;
struct Listener;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Opened audio playback device used to play sound sources
///
/// A `PlaybackDevice` represents a live, opened audio output
/// (speakers, headphones, virtual sink, etc.). It owns the
/// underlying miniaudio engine and is the entity that
/// `za::Sound` and `za::Music` instances render through.
///
/// A playback device is created from a `PlaybackDeviceHandle`
/// obtained via `za::AudioContext`:
/// \code
/// auto audioContext   = za::AudioContext::create().value();
/// auto handle         = za::AudioContext::getDefaultPlaybackDeviceHandle().value();
/// za::PlaybackDevice  playbackDevice{handle};
/// \endcode
///
/// `PlaybackDevice` is non-copyable and non-movable: any
/// `za::Sound` or `za::SoundStream` constructed from it holds
/// a non-owning reference, so the device must outlive every
/// sound source bound to it.
///
/// \see `za::AudioContext`, `za::PlaybackDeviceHandle`, `za::Sound`, `za::SoundStream`, `za::Music`
///
////////////////////////////////////////////////////////////
class ZA_AUDIO_API PlaybackDevice
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Open a playback device from a device handle
    ///
    /// Initializes the underlying miniaudio engine bound to the
    /// device described by `deviceHandle`. The handle is
    /// typically obtained from `za::AudioContext`.
    ///
    /// \param deviceHandle Handle identifying the device to open
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit PlaybackDevice(const PlaybackDeviceHandle& deviceHandle);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Closes the underlying device. All `za::Sound`,
    /// `za::SoundStream`, and `za::Music` instances bound to
    /// this device must already have been destroyed.
    ///
    ////////////////////////////////////////////////////////////
    ~PlaybackDevice();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    PlaybackDevice(const PlaybackDevice&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    PlaybackDevice& operator=(const PlaybackDevice&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted move constructor
    ///
    ////////////////////////////////////////////////////////////
    PlaybackDevice(PlaybackDevice&&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted move assignment
    ///
    ////////////////////////////////////////////////////////////
    PlaybackDevice& operator=(PlaybackDevice&&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Get the handle of the device that was opened
    ///
    /// \return Reference to the device handle this device was
    ///         constructed from
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const PlaybackDeviceHandle& getDeviceHandle() const;

    ////////////////////////////////////////////////////////////
    /// \brief Push the listener parameters to the underlying engine
    ///
    /// `za::Listener` is a plain data structure: changes made to
    /// a `Listener` value have no effect until they are committed
    /// to a playback device through this function. Each playback
    /// device keeps its own listener state.
    ///
    /// \param listener Listener configuration to apply
    ///
    /// \return `true` on success, `false` on failure
    ///
    /// \see `za::Listener`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool applyListener(const Listener& listener);

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
    ///         playback device, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isDefault() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the sample rate of the underlying audio engine
    ///
    /// \return Sample rate (number of audio samples to play per second)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getSampleRate() const;

private:
    // Friends
    using SoundBase = priv::MiniaudioUtils::SoundBase;
    friend SoundBase;
    friend Sound;

    ////////////////////////////////////////////////////////////
    /// \brief Get the internal miniaudio engine pointer
    ///
    /// Internal accessor used by `za::Sound` / `za::SoundStream`
    /// to attach themselves to the engine. Not part of the
    /// public API.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] void* getMAEngine();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 7680> m_impl; //!< Implementation details

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_LIFETIME_DEPENDED_ON_BY(PlaybackDevice, SoundBase); // `PlaybackDevice` is depended on by `SoundBase`
};

} // namespace za
