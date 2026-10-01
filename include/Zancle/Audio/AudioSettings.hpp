#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/Listener.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Vec3.hpp"

#include "Zancle/Base/Limits.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Aggregate of all the parameters that fully describe how a sound source plays
///
/// `AudioSettings` is a plain data structure that bundles together
/// every tunable property of a sound source: spatial parameters
/// (position, direction, velocity, cone, distance/gain bounds),
/// playback parameters (volume, pitch, pan), looping/spatialization
/// flags, and so on.
///
/// It is used to construct sound sources (`za::Sound`, `za::Music`)
/// in a single shot, and to snapshot or restore a source's state via
/// `MiniaudioSoundSource::applyAudioSettings`. Because it is a plain
/// aggregate, it can be freely copied, compared, serialized, and stored.
///
/// All defaults match the defaults that a freshly constructed
/// sound source would have, so default-constructing an
/// `AudioSettings` and then applying it is a no-op.
///
/// \see `za::Sound`, `za::Music`, `za::priv::MiniaudioSoundSource`, `za::Listener`
///
////////////////////////////////////////////////////////////
struct AudioSettings
{
    ////////////////////////////////////////////////////////////
    /// \brief How the sound's `position` is interpreted relative to the listener
    ///
    ////////////////////////////////////////////////////////////
    enum class Positioning : unsigned char
    {
        Absolute = 0, //!< Position is expressed in world space, independent of the listener
        Relative = 1, //!< Position is expressed relative to the listener (useful for sounds attached to the listener)
    };

    Listener::Cone cone{Angle::Full, Angle::Full, 1.f}; //!< Directional cone (full sphere by default, no directional attenuation)

    Vec3f position{0.f, 0.f, 0.f};   //!< Position of the sound source in 3D space
    Vec3f direction{0.f, 0.f, -1.f}; //!< Forward-pointing direction vector of the source (not necessarily normalized)
    Vec3f velocity{0.f, 0.f, 0.f};   //!< Velocity in 3D space, used to compute Doppler shift

    float volume{1.f}; //!< Volume multiplier (0 = silent, 1 = full volume)
    float pitch{1.f};  //!< Pitch multiplier (also affects playback speed; default is `1`)
    float pan{0.f};    //!< Stereo pan in `[-1, +1]` (-1 = full left, 0 = center, +1 = full right)

    float attenuation{1.f}; //!< Distance attenuation factor (0 = no attenuation, larger values = sharper falloff)
    float directionalAttenuationFactor{1.f}; //!< Strength of directional attenuation based on output channel position

    float dopplerFactor{1.f}; //!< Strength of the Doppler effect (0 = disabled)

    float minDistance{1.f};          //!< Distance below which the sound is heard at full volume
    float maxDistance{ZA_FLOAT_MAX}; //!< Distance above which the sound is heard at minimum (clamped) volume

    float minGain{0.f}; //!< Lower bound for the attenuated gain (between `0` and `1`)
    float maxGain{1.f}; //!< Upper bound for the attenuated gain (between `0` and `1`)

    Positioning positioning{Positioning::Absolute}; //!< Whether `position` is in world or listener-relative space

    bool looping{false}; //!< Whether playback restarts from the beginning after reaching the end

    bool spatializationEnabled{true}; //!< Whether 3D spatialization (panning, attenuation, Doppler) is applied

    ////////////////////////////////////////////////////////////
    /// \brief Default member-wise equality
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool operator==(const AudioSettings&) const noexcept = default;
};

// TODO P0: also save cursor? or maybe have start,current,end? maybe rename to AudioState?

} // namespace za
