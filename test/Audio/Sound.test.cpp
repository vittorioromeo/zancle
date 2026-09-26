#include "Zancle/Audio/Sound.hpp"

#include "Zancle/Audio/AudioContext.hpp"
#include "Zancle/Audio/PlaybackDevice.hpp"

// Other 1st party headers
#include "AudioUtil.hpp"
#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Audio/SoundBuffer.hpp"

#include "Zancle/Lifetime/LifetimeDependee.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/HasVirtualDestructor.hpp"
#include "Zancle/Trait/IsConstructible.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsMoveAssignable.hpp"
#include "Zancle/Trait/IsMoveConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"

TEST_CASE("[Audio] za::Sound" * tst::skip(skipAudioDeviceTests))
{
    auto               audioContext = za::AudioContext::create().value();
    za::PlaybackDevice playbackDevice{za::AudioContext::getDefaultPlaybackDeviceHandle().value()};

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::Sound, za::SoundBuffer&&));
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::Sound, const za::SoundBuffer&&));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::Sound));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::Sound));
        STATIC_CHECK(!ZA_IS_MOVE_CONSTRUCTIBLE(za::Sound));
        STATIC_CHECK(!ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::Sound));
        STATIC_CHECK(!ZA_IS_MOVE_ASSIGNABLE(za::Sound));
        STATIC_CHECK(!ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::Sound));
        STATIC_CHECK(ZA_HAS_VIRTUAL_DESTRUCTOR(za::Sound));
    }

    const auto soundBuffer = za::SoundBuffer::loadFromFile("ding.flac").value();

    SECTION("Construction")
    {
        const za::Sound sound(playbackDevice, soundBuffer);
        CHECK(&sound.getBuffer() == &soundBuffer);
        CHECK(!sound.isLooping());
        CHECK(sound.getPlayingOffset() == za::Time{});
        CHECK(!sound.isPlaying());
    }

    SECTION("Get buffer")
    {
        za::Sound sound(playbackDevice, soundBuffer);
        CHECK(&sound.getBuffer() == &soundBuffer);
    }

    SECTION("Set/get loop")
    {
        za::Sound sound(playbackDevice, soundBuffer);
        sound.setLooping(true);
        CHECK(sound.isLooping());
    }

    SECTION("Set/get playing offset")
    {
        za::Sound sound(playbackDevice, soundBuffer);
        sound.setPlayingOffset(za::seconds(10));
        CHECK(sound.getPlayingOffset() == za::seconds(10));
    }

    SECTION("Sound buffer move")
    {
        auto soundBufferA = za::SoundBuffer::loadFromFile("ding.flac").value();
        auto soundBufferB = za::SoundBuffer::loadFromFile("ding.flac").value();

        const za::Sound sound(playbackDevice, soundBufferA);
        CHECK(&sound.getBuffer() == &soundBufferA);
        CHECK(!sound.isLooping());
        CHECK(sound.getPlayingOffset() == za::Time{});
        CHECK(!sound.isPlaying());

        soundBufferB = ZA_MOVE(soundBufferA);
        CHECK(&sound.getBuffer() == &soundBufferA);
    }

    SECTION("Multiple sounds and buffers -- copy")
    {
        auto soundBufferA = za::SoundBuffer::loadFromFile("ding.flac").value();
        auto soundBufferB = za::SoundBuffer::loadFromFile("ding.flac").value();

        const za::Sound soundA(playbackDevice, soundBufferA);
        const za::Sound soundB(playbackDevice, soundBufferB);

        CHECK(&soundA.getBuffer() == &soundBufferA);
        CHECK(&soundB.getBuffer() == &soundBufferB);

        soundBufferB = soundBufferA;

        CHECK(&soundA.getBuffer() == &soundBufferA);
        CHECK(&soundB.getBuffer() == &soundBufferB);
    }

    SECTION("Multiple sounds and buffers")
    {
        auto soundBufferA = za::SoundBuffer::loadFromFile("ding.flac").value();
        auto soundBufferB = za::SoundBuffer::loadFromFile("ding.flac").value();

        const za::Sound soundA(playbackDevice, soundBufferA);
        const za::Sound soundB(playbackDevice, soundBufferB);

        CHECK(&soundA.getBuffer() == &soundBufferA);
        CHECK(&soundB.getBuffer() == &soundBufferB);

        soundBufferB = ZA_MOVE(soundBufferA);

        CHECK(&soundA.getBuffer() == &soundBufferA);
        CHECK(&soundB.getBuffer() == &soundBufferB);
    }

#if defined(ZA_ENABLE_LIFETIME_TRACKING)
    SECTION("Lifetime tracking")
    {
        SECTION("Return local from function")
        {
            const auto badFunction = [&playbackDevice]
            {
                const auto localSoundBuffer = za::SoundBuffer::loadFromFile("ding.flac").value();
                return za::Sound(playbackDevice, localSoundBuffer);
            };

            const za::priv::LifetimeDependee::TestingModeGuard guard{"SoundBuffer"};
            CHECK(!guard.fatalErrorTriggered("SoundBuffer"));

            badFunction();

            CHECK(guard.fatalErrorTriggered("SoundBuffer"));
        }

        SECTION("Move struct holding both dependee and dependant")
        {
            struct BadStruct
            {
                explicit BadStruct(za::PlaybackDevice& thePlaybackDevice) :
                    memberSoundBuffer{za::SoundBuffer::loadFromFile("ding.flac").value()},
                    memberSound{thePlaybackDevice, memberSoundBuffer}
                {
                }

                za::SoundBuffer memberSoundBuffer;
                za::Sound       memberSound;
            };

            const za::priv::LifetimeDependee::TestingModeGuard guard{"SoundBuffer"};
            CHECK(!guard.fatalErrorTriggered("SoundBuffer"));

            za::Optional<BadStruct> badStruct0;
            badStruct0.emplace(playbackDevice);
            CHECK(!guard.fatalErrorTriggered("SoundBuffer"));

            badStruct0.reset();
            CHECK(!guard.fatalErrorTriggered("SoundBuffer"));
        }

        SECTION("Dependee move assignment")
        {
            const za::priv::LifetimeDependee::TestingModeGuard guard{"SoundBuffer"};
            CHECK(!guard.fatalErrorTriggered("SoundBuffer"));

            auto sb0 = za::SoundBuffer::loadFromFile("ding.flac").value();
            CHECK(!guard.fatalErrorTriggered("SoundBuffer"));

            za::Sound s0(playbackDevice, sb0);
            CHECK(!guard.fatalErrorTriggered("SoundBuffer"));

            sb0 = za::SoundBuffer::loadFromFile("ding.flac").value();
            CHECK(!guard.fatalErrorTriggered("SoundBuffer"));
        }

        SECTION("Dependee destroyed before dependant")
        {
            const za::priv::LifetimeDependee::TestingModeGuard guard{"SoundBuffer"};
            CHECK(!guard.fatalErrorTriggered("SoundBuffer"));

            auto optDependee = za::SoundBuffer::loadFromFile("ding.flac");
            CHECK(optDependee.hasValue());
            CHECK(!guard.fatalErrorTriggered("SoundBuffer"));

            za::Sound s0(playbackDevice, *optDependee);
            CHECK(!guard.fatalErrorTriggered("SoundBuffer"));

            optDependee.reset();
            CHECK(guard.fatalErrorTriggered("SoundBuffer"));
        }
    }
#endif
}
