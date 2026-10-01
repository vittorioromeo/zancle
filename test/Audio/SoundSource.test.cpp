#include "AudioUtil.hpp"
#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Audio/AudioContext.hpp"
#include "Zancle/Audio/PlaybackDevice.hpp"
#include "Zancle/Audio/Priv/MiniaudioSoundSource.hpp"
#include "Zancle/Audio/Sound.hpp"
#include "Zancle/Audio/SoundBuffer.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Vec3.hpp"

#include "Zancle/Base/Limits.hpp"

#include "Zancle/Trait/HasVirtualDestructor.hpp"
#include "Zancle/Trait/IsConstructible.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsMoveAssignable.hpp"
#include "Zancle/Trait/IsMoveConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"


#define ZA_TEST_CHECK_DEFAULT_VALUES(xSoundSource)                     \
    CHECK((xSoundSource).getPitch() == 1.f);                           \
    CHECK((xSoundSource).getPan() == 0.f);                             \
    CHECK((xSoundSource).getVolume() == 1.f);                          \
    CHECK((xSoundSource).isSpatializationEnabled());                   \
    CHECK((xSoundSource).getPosition() == za::Vec3{0.f, 0.f, 0.f});    \
    CHECK((xSoundSource).getDirection() == za::Vec3f{0.f, 0.f, -1.f}); \
    CHECK((xSoundSource).getCone().innerAngle == za::Angle::Full);     \
    CHECK((xSoundSource).getCone().outerAngle == za::Angle::Full);     \
    CHECK((xSoundSource).getCone().outerGain == 1.f);                  \
    CHECK((xSoundSource).getVelocity() == za::Vec3f{});                \
    CHECK((xSoundSource).getDopplerFactor() == 1.f);                   \
    CHECK((xSoundSource).getDirectionalAttenuationFactor() == 1.f);    \
    CHECK(!(xSoundSource).isRelativeToListener());                     \
    CHECK((xSoundSource).getMinDistance() == 1.f);                     \
    CHECK((xSoundSource).getMaxDistance() == ZA_FLOAT_MAX);            \
    CHECK((xSoundSource).getMinGain() == 0);                           \
    CHECK((xSoundSource).getMaxGain() == 1.f);                         \
    CHECK((xSoundSource).getAttenuation() == 1.f);                     \
    CHECK(!(xSoundSource).isPlaying());


TEST_CASE("[Audio] za::priv::MiniaudioSoundSource" * tst::skip(skipAudioDeviceTests))
{
    auto               audioContext = za::AudioContext::create().value();
    za::PlaybackDevice playbackDevice{za::AudioContext::getDefaultPlaybackDeviceHandle().value()};
    const auto         soundBuffer = za::SoundBuffer::loadFromFile("ding.flac").value();

    using TestSoundSource = za::Sound;

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::priv::MiniaudioSoundSource));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::priv::MiniaudioSoundSource));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::priv::MiniaudioSoundSource));
        STATIC_CHECK(!ZA_IS_MOVE_CONSTRUCTIBLE(za::priv::MiniaudioSoundSource));
        STATIC_CHECK(ZA_IS_MOVE_ASSIGNABLE(za::priv::MiniaudioSoundSource));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::priv::MiniaudioSoundSource));
        STATIC_CHECK(ZA_HAS_VIRTUAL_DESTRUCTOR(za::priv::MiniaudioSoundSource));
    }

    SECTION("Construction")
    {
        const TestSoundSource soundSource(playbackDevice, soundBuffer);
        ZA_TEST_CHECK_DEFAULT_VALUES(soundSource);
    }

    SECTION("Set/get pitch")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);
        soundSource.setPitch(42.f);
        CHECK(soundSource.getPitch() == 42.f);
    }

    SECTION("Set/get pan")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setPan(1.f);
        CHECK(soundSource.getPan() == 1.f);
    }

    SECTION("Set/get volume")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setVolume(0.f);
        CHECK(soundSource.getVolume() == 0.f);

        soundSource.setVolume(0.5f);
        CHECK(soundSource.getVolume() == 0.5f);

        soundSource.setVolume(1.f);
        CHECK(soundSource.getVolume() == 1.f);
    }

    SECTION("Set/get spatialization enabled")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setSpatializationEnabled(true);
        CHECK(soundSource.isSpatializationEnabled());
    }

    SECTION("Set/get position")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setPosition({1, 2, 3});
        CHECK(soundSource.getPosition() == za::Vec3f{1, 2, 3});
    }

    SECTION("Set/get direction")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setDirection({4, 5, 6});
        CHECK(soundSource.getDirection() == za::Vec3f{4, 5, 6});
    }

    SECTION("Set/get cone")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setCone({za::radians(1), za::radians(2), 3});
        CHECK(soundSource.getCone().innerAngle == za::radians(1));
        CHECK(soundSource.getCone().outerAngle == za::radians(2));
        CHECK(soundSource.getCone().outerGain == 3);
    }

    SECTION("Set/get velocity")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setVelocity({7, 8, 9});
        CHECK(soundSource.getVelocity() == za::Vec3f{7, 8, 9});
    }

    SECTION("Set/get doppler factor")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setDopplerFactor(1);
        CHECK(soundSource.getDopplerFactor() == 1);
    }

    SECTION("Set/get directional attenuation factor")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setDirectionalAttenuationFactor(1);
        CHECK(soundSource.getDirectionalAttenuationFactor() == 1);
    }

    SECTION("Set/get relative to listener")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setRelativeToListener(true);
        CHECK(soundSource.isRelativeToListener());
    }

    SECTION("Set/get min distance")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setMinDistance(12.34f);
        CHECK(soundSource.getMinDistance() == 12.34f);
    }

    SECTION("Set/get max distance")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setMaxDistance(12.34f);
        CHECK(soundSource.getMaxDistance() == 12.34f);
    }

    SECTION("Set/get min gain")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setMinGain(12.34f);
        CHECK(soundSource.getMinGain() == 12.34f);
    }

    SECTION("Set/get max gain")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);
        soundSource.setMaxGain(12.34f);
        CHECK(soundSource.getMaxGain() == 12.34f);
    }

    SECTION("Set/get attenuation")
    {
        TestSoundSource soundSource(playbackDevice, soundBuffer);

        soundSource.setAttenuation(10);
        CHECK(soundSource.getAttenuation() == 10);
    }
}
