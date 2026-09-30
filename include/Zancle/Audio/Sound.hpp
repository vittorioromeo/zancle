#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/Export.hpp"

#include "Zancle/Audio/Priv/MiniaudioSoundSource.hpp"

#include "Zancle/Lifetime/LifetimeDependant.hpp"

#include "Zancle/Vocabulary/InPlacePImpl.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class PlaybackDevice;
class SoundBuffer;
class Time;
struct AudioSettings;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Regular sound that can be played in the audio environment
///
////////////////////////////////////////////////////////////
class ZA_AUDIO_API Sound : public priv::MiniaudioSoundSource
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Construct the sound with a buffer and a full settings snapshot
    ///
    /// The sound is bound to `playbackDevice` (which must
    /// outlive it) and reads its samples from `buffer` (which
    /// must also outlive it). Every property in `audioSettings`
    /// is applied immediately.
    ///
    /// \param playbackDevice Playback device to render through
    /// \param buffer         Sound buffer containing the audio data to play
    /// \param audioSettings  Initial audio settings to apply (volume, pitch, position, etc.)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit Sound(PlaybackDevice& playbackDevice, const SoundBuffer& buffer, const AudioSettings& audioSettings);

    ////////////////////////////////////////////////////////////
    /// \brief Construct the sound with a buffer and default settings
    ///
    /// Equivalent to passing a default-constructed
    /// `AudioSettings` to the other constructor.
    ///
    /// \param playbackDevice Playback device to render through
    /// \param buffer         Sound buffer containing the audio data to play
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit Sound(PlaybackDevice& playbackDevice, const SoundBuffer& buffer);

    ////////////////////////////////////////////////////////////
    /// \brief Disallow construction from a temporary sound buffer
    ///
    /// `Sound` only stores a reference to the buffer, so
    /// constructing one from a temporary would immediately
    /// dangle.
    ///
    ////////////////////////////////////////////////////////////
    Sound(PlaybackDevice&, const SoundBuffer&& buffer, const AudioSettings& audioSettings) = delete;
    Sound(PlaybackDevice&, const SoundBuffer&& buffer)                                     = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~Sound() override;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    Sound(const Sound& rhs) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    Sound& operator=(const Sound& rhs) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted move constructor
    ///
    ////////////////////////////////////////////////////////////
    Sound(Sound&& rhs) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted move assignment
    ///
    ////////////////////////////////////////////////////////////
    Sound& operator=(Sound&& rhs) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Change the current playing position of the sound
    ///
    /// The playing position can be changed when the sound is
    /// either paused or playing. Changing the playing position
    /// when the sound is stopped has no effect, since playing
    /// the sound will reset its position.
    ///
    /// \param playingOffset New playing position, from the beginning of the sound
    ///
    /// \see `getPlayingOffset`
    ///
    ////////////////////////////////////////////////////////////
    void setPlayingOffset(Time playingOffset) override;

    ////////////////////////////////////////////////////////////
    /// \brief Get the audio buffer attached to the sound
    ///
    /// \return Sound buffer this sound is reading samples from
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const SoundBuffer& getBuffer() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the playback device this sound is rendered through
    ///
    /// \return Reference to the playback device this sound was constructed with
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] PlaybackDevice& getPlaybackDevice() const;

private:
    friend class SoundBuffer;

    ////////////////////////////////////////////////////////////
    /// \brief Get the sound object
    ///
    /// \return The sound object
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] priv::MiniaudioUtils::SoundBase& getSoundBase() const override;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 1896> m_impl; //!< Implementation details

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_LIFETIME_DEPENDS_ON(SoundBuffer); // `Sound` depends on `SoundBuffer`
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::Sound
/// \ingroup audio
///
/// `za::Sound` is the class to use to play sounds.
/// It provides:
/// \li Control (play, pause, stop)
/// \li Ability to modify output parameters in real-time (pitch, volume, etc...)
/// \li 3D spatial features (position, attenuation, etc...).
///
/// `za::Sound` is perfect for playing short sounds that can
/// fit in memory and require no latency, like foot steps or
/// gun shots. For longer sounds, like background musics
/// or long speeches, rather see `za::Music` (which is based
/// on streaming).
///
/// In order to work, a sound must be given a buffer of audio
/// data to play. Audio data (samples) is stored in `za::SoundBuffer,`
/// and attached to a sound when it is created.
/// The buffer object attached to a sound must remain alive
/// as long as the sound uses it. Note that multiple sounds
/// can use the same sound buffer at the same time.
///
/// Usage example:
/// \code
/// // Assumes `playbackDevice` is an initialized za::PlaybackDevice
/// const auto buffer = za::SoundBuffer::loadFromFile("sound.wav").value();
/// za::Sound sound(playbackDevice, buffer);
/// sound.play();
/// \endcode
///
/// \see `za::SoundBuffer`, `za::Music`
///
////////////////////////////////////////////////////////////
