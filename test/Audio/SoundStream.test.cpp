#include "AudioUtil.hpp"
#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Audio/SoundStream.hpp"

#include "Zancle/Audio/AudioContext.hpp"
#include "Zancle/Audio/ChannelMap.hpp"
#include "Zancle/Audio/PlaybackDevice.hpp"
#include "Zancle/Audio/SoundChannel.hpp"

#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Vec3.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/IntTypes.hpp"

#include "Zancle/Trait/HasVirtualDestructor.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


namespace
{
struct TestState
{
    bool onGetData(za::Vector<za::I16>& /* outBuffer */)
    {
        return true;
    }
};

using TestSoundStream = za::SoundStream<TestState>;

} // namespace

TEST_CASE("[Audio] za::SoundStream" * tst::skip(skipAudioDeviceTests))
{
    auto               audioContext = za::AudioContext::create().value();
    za::PlaybackDevice playbackDevice{za::AudioContext::getDefaultPlaybackDeviceHandle().value()};

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(TestSoundStream));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(TestSoundStream));
        STATIC_CHECK(!ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(TestSoundStream));
        STATIC_CHECK(!ZA_IS_NOTHROW_MOVE_ASSIGNABLE(TestSoundStream));
        STATIC_CHECK(ZA_HAS_VIRTUAL_DESTRUCTOR(TestSoundStream));
    }

    SECTION("Construction")
    {
        const TestSoundStream testSoundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        CHECK(!testSoundStream.isPlaying());
        CHECK(testSoundStream.getPlayingOffset() == za::Time{});
        CHECK(!testSoundStream.isLooping());
    }

    SECTION("Set/get playing offset")
    {
        TestSoundStream testSoundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        testSoundStream.setPlayingOffset(za::milliseconds(100));
        CHECK(testSoundStream.getPlayingOffset() == za::milliseconds(100));
    }

    SECTION("Set/get loop")
    {
        TestSoundStream testSoundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        testSoundStream.setLooping(true);
        CHECK(testSoundStream.isLooping());
    }

    SECTION("Set/get pitch")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setPitch(42);
        CHECK(soundStream.getPitch() == 42);
    }

    SECTION("Set/get pan")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setPan(1.f);
        CHECK(soundStream.getPan() == 1.f);
        soundStream.setPan(2.f);
        CHECK(soundStream.getPan() == 2.f);
        soundStream.setPan(-2.f);
        CHECK(soundStream.getPan() == -2.f);
    }

    SECTION("Set/get volume")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);

        soundStream.setVolume(0.f);
        CHECK(soundStream.getVolume() == 0.f);

        soundStream.setVolume(0.5f);
        CHECK(soundStream.getVolume() == 0.5f);

        soundStream.setVolume(1.f);
        CHECK(soundStream.getVolume() == 1.f);
    }

    SECTION("Set/get spatialization enabled")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setSpatializationEnabled(true);
        CHECK(soundStream.isSpatializationEnabled());
    }

    SECTION("Set/get position")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setPosition({1, 2, 3});
        CHECK(soundStream.getPosition() == za::Vec3f(1, 2, 3));
    }

    SECTION("Set/get direction")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setDirection({4, 5, 6});
        CHECK(soundStream.getDirection() == za::Vec3f(4, 5, 6));
    }

    SECTION("Set/get cone")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setCone({za::radians(1), za::radians(2), 3});
        CHECK(soundStream.getCone().innerAngle == za::radians(1));
        CHECK(soundStream.getCone().outerAngle == za::radians(2));
        CHECK(soundStream.getCone().outerGain == 3);
    }

    SECTION("Set/get velocity")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setVelocity({7, 8, 9});
        CHECK(soundStream.getVelocity() == za::Vec3f(7, 8, 9));
    }

    SECTION("Set/get doppler factor")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setDopplerFactor(1);
        CHECK(soundStream.getDopplerFactor() == 1);
    }

    SECTION("Set/get directional attenuation factor")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setDirectionalAttenuationFactor(1);
        CHECK(soundStream.getDirectionalAttenuationFactor() == 1);
    }

    SECTION("Set/get relative to listener")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setRelativeToListener(true);
        CHECK(soundStream.isRelativeToListener());
    }

    SECTION("Set/get min distance")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setMinDistance(12.34f);
        CHECK(soundStream.getMinDistance() == 12.34f);
    }

    SECTION("Set/get max distance")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setMaxDistance(12.34f);
        CHECK(soundStream.getMaxDistance() == 12.34f);
    }

    SECTION("Set/get min gain")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setMinGain(12.34f);
        CHECK(soundStream.getMinGain() == 12.34f);
    }

    SECTION("Set/get max gain")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setMaxGain(12.34f);
        CHECK(soundStream.getMaxGain() == 12.34f);
    }

    SECTION("Set/get attenuation")
    {
        TestSoundStream soundStream(playbackDevice, za::ChannelMap{za::SoundChannel::Mono}, 44'100u);
        soundStream.setAttenuation(10);
        CHECK(soundStream.getAttenuation() == 10);
    }
}
