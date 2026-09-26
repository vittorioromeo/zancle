#include "Zancle/Audio/Music.hpp"

#include "Zancle/Audio/AudioContext.hpp"
#include "Zancle/Audio/PlaybackDevice.hpp"

// Other 1st party headers
#include "AudioUtil.hpp"
#include "LoadIntoMemoryUtil.hpp"
#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Audio/MusicReader.hpp"

#include "Zancle/Lifetime/LifetimeDependee.hpp"

#include "Zancle/IO/FileInputStream.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/Concurrency/Thread.hpp"

#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/Memset.hpp"

#include "Zancle/Trait/HasVirtualDestructor.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


TEST_CASE("[Audio] za::Music" * tst::skip(skipAudioDeviceTests))
{
    auto               audioContext = za::AudioContext::create().value();
    za::PlaybackDevice playbackDevice{za::AudioContext::getDefaultPlaybackDeviceHandle().value()};

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::Music));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::Music));
        STATIC_CHECK(!ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::Music));
        STATIC_CHECK(!ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::Music));
        STATIC_CHECK(ZA_HAS_VIRTUAL_DESTRUCTOR(za::Music));
    }

    SECTION("Span")
    {
        const za::Music::Span<float> span;
        CHECK(span.offset == 0);
        CHECK(span.length == 0);

        const za::Music::TimeSpan timeSpan;
        CHECK(timeSpan.offset == za::Time{});
        CHECK(timeSpan.length == za::Time{});
    }

    SECTION("openFromFile()")
    {
        SECTION("Invalid file")
        {
            CHECK(!za::MusicReader::openFromFile("does/not/exist.wav").hasValue());
        }

        SECTION("Valid file")
        {
            const za::Path filenameSuffixes[] = {U"", U"-ń", U"-🐌"};

            for (const auto& filenameSuffix : filenameSuffixes)
            {
                const za::Path filename = U"ding" + filenameSuffix + U".mp3";

                auto musicReader = za::MusicReader::openFromFile("ding.mp3").value();
                CHECK(musicReader.getDuration() == za::microseconds(1'990'884));

                za::Music music(playbackDevice, musicReader);

                const auto [offset, length] = music.getLoopPoints();
                CHECK(offset == za::Time{});
                CHECK(length == za::microseconds(1'990'884));

                CHECK(!music.isPlaying());
                CHECK(music.getPlayingOffset() == za::Time{});
                CHECK(!music.isLooping());
            }
        }
    }

    SECTION("openFromMemory()")
    {
        za::Vector<char> memory(10);
        ZA_MEMSET(memory.data(), 0xCA, 10);

        SECTION("Invalid buffer")
        {
            CHECK(!za::MusicReader::openFromMemory(memory.data(), memory.size()).hasValue());
        }

        SECTION("Valid buffer")
        {
            memory = loadIntoMemory("ding.flac");

            auto musicReader = za::MusicReader::openFromMemory(memory.data(), memory.size()).value();
            CHECK(static_cast<const za::MusicReader&>(musicReader).getDuration() == za::microseconds(1'990'884));
            CHECK(static_cast<const za::MusicReader&>(musicReader).getChannelCount() == 1);
            CHECK(static_cast<const za::MusicReader&>(musicReader).getSampleRate() == 44'100);

            za::Music music(playbackDevice, musicReader);

            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == za::Time{});
            CHECK(length == za::microseconds(1'990'884));

            CHECK(!music.isPlaying());
            CHECK(music.getPlayingOffset() == za::Time{});
            CHECK(!music.isLooping());
        }
    }

    SECTION("openFromStream()")
    {
        auto stream      = za::FileInputStream::open("doodle_pop.ogg").value();
        auto musicReader = za::MusicReader::openFromStream(stream).value();
        CHECK(static_cast<const za::MusicReader&>(musicReader).getDuration() == za::microseconds(24'002'176));
        CHECK(static_cast<const za::MusicReader&>(musicReader).getChannelCount() == 2);
        CHECK(static_cast<const za::MusicReader&>(musicReader).getSampleRate() == 44'100);

        za::Music music(playbackDevice, musicReader);

        const auto [offset, length] = music.getLoopPoints();
        CHECK(offset == za::Time{});
        CHECK(length == za::microseconds(24'002'176));

        CHECK(!music.isPlaying());
        CHECK(music.getPlayingOffset() == za::Time{});
        CHECK(!music.isLooping());
    }

    SECTION("play/pause/stop")
    {
        auto musicReader = za::MusicReader::openFromFile("ding.mp3").value();

        za::Music music(playbackDevice, musicReader);

        // Wait for background thread to start
        music.play();
        while (!music.isPlaying())
            za::ThisThread::sleepFor(za::milliseconds(10));
        CHECK(music.isPlaying());

        // Wait for background thread to pause
        music.pause();
        while (music.isPlaying())
            za::ThisThread::sleepFor(za::milliseconds(10));
        CHECK(!music.isPlaying());

        // Wait for background thread to stop
        music.stop();
        while (music.isPlaying())
            za::ThisThread::sleepFor(za::milliseconds(10));
        CHECK(!music.isPlaying());
    }

    SECTION("setLoopPoints()")
    {
        auto musicReader = za::MusicReader::openFromFile("killdeer.wav").value();

        za::Music music(playbackDevice, musicReader);

        music.setLoopPoints({za::seconds(1), za::seconds(2)});
        CHECK(musicReader.getChannelCount() == 1);
        CHECK(musicReader.getSampleRate() == 22'050);

        SECTION("Within range")
        {
            music.setLoopPoints({za::seconds(1), za::seconds(2)});
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == za::seconds(1));
            CHECK(length == za::seconds(2));
        }

        SECTION("Duration too long")
        {
            music.setLoopPoints({za::seconds(1), za::seconds(1000)});
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == za::seconds(1));
            CHECK(length == za::microseconds(4'122'040));
        }

        SECTION("Offset too long")
        {
            musicReader = za::MusicReader::openFromFile("killdeer.wav").value();

            music.setLoopPoints({za::seconds(1000), za::milliseconds(10)});
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == za::seconds(0));
            CHECK(length == za::microseconds(5'122'040));
        }

        CHECK(musicReader.getChannelCount() == 1);
        CHECK(musicReader.getSampleRate() == 22'050);
        CHECK(!music.isPlaying());
        CHECK(music.getPlayingOffset() == za::Time{});
        CHECK(!music.isLooping());
    }

#if defined(ZA_ENABLE_LIFETIME_TRACKING)
    SECTION("Lifetime tracking")
    {
        SECTION("Return local from function")
        {
            const auto badFunction = [&playbackDevice]
            {
                auto localMusicSource = za::MusicReader::openFromFile("ding.mp3").value();
                return za::Music(playbackDevice, localMusicSource);
            };

            const za::priv::LifetimeDependee::TestingModeGuard guard{"MusicReader"};
            CHECK(!guard.fatalErrorTriggered("MusicReader"));

            badFunction();

            CHECK(guard.fatalErrorTriggered("MusicReader"));
        }

        SECTION("Move struct holding both dependee and dependant")
        {
            struct BadStruct
            {
                explicit BadStruct(za::PlaybackDevice& thePlaybackDevice) :
                    memberMusicSource{za::MusicReader::openFromFile("ding.mp3").value()},
                    memberSound{thePlaybackDevice, memberMusicSource}
                {
                }

                za::MusicReader memberMusicSource;
                za::Music       memberSound;
            };

            const za::priv::LifetimeDependee::TestingModeGuard guard{"MusicReader"};
            CHECK(!guard.fatalErrorTriggered("MusicReader"));

            za::Optional<BadStruct> badStruct0;
            badStruct0.emplace(playbackDevice);
            CHECK(!guard.fatalErrorTriggered("MusicReader"));

            badStruct0.reset();
            CHECK(!guard.fatalErrorTriggered("MusicReader"));
        }

        SECTION("Dependee move assignment")
        {
            const za::priv::LifetimeDependee::TestingModeGuard guard{"MusicReader"};
            CHECK(!guard.fatalErrorTriggered("MusicReader"));

            auto sb0 = za::MusicReader::openFromFile("ding.mp3").value();
            CHECK(!guard.fatalErrorTriggered("MusicReader"));

            za::Music s0(playbackDevice, sb0);
            CHECK(!guard.fatalErrorTriggered("MusicReader"));

            sb0 = za::MusicReader::openFromFile("ding.mp3").value();
            CHECK(!guard.fatalErrorTriggered("MusicReader"));
        }

        SECTION("Dependee destroyed before dependant")
        {
            const za::priv::LifetimeDependee::TestingModeGuard guard{"MusicReader"};
            CHECK(!guard.fatalErrorTriggered("MusicReader"));

            auto optDependee = za::MusicReader::openFromFile("ding.mp3");
            CHECK(optDependee.hasValue());
            CHECK(!guard.fatalErrorTriggered("MusicReader"));

            za::Music s0(playbackDevice, *optDependee);
            CHECK(!guard.fatalErrorTriggered("MusicReader"));

            optDependee.reset();
            CHECK(guard.fatalErrorTriggered("MusicReader"));
        }
    }
#endif
}
