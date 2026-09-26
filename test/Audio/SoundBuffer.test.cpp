#include "Zancle/Audio/SoundBuffer.hpp"

// Other 1st party headers
#include "AudioUtil.hpp"
#include "LoadIntoMemoryUtil.hpp"
#include "SystemUtil.hpp" // IWYU pragma: keep
#include "TemporaryFile.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Fmt/FmtToString.hpp"

#include "Zancle/IO/FileInputStream.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsDefaultConstructible.hpp"
#include "Zancle/Trait/IsMoveAssignable.hpp"
#include "Zancle/Trait/IsMoveConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


TEST_CASE("[Audio] za::SoundBuffer" * tst::skip(skipAudioDeviceTests))
{
    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_DEFAULT_CONSTRUCTIBLE(za::SoundBuffer));
        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::SoundBuffer));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::SoundBuffer));
        STATIC_CHECK(ZA_IS_MOVE_CONSTRUCTIBLE(za::SoundBuffer));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::SoundBuffer));
        STATIC_CHECK(ZA_IS_MOVE_ASSIGNABLE(za::SoundBuffer));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::SoundBuffer));
    }

    SECTION("Copy semantics")
    {
        const auto soundBuffer = za::SoundBuffer::loadFromFile("ding.flac").value();

        SECTION("Construction")
        {
            const za::SoundBuffer soundBufferCopy(soundBuffer); // NOLINT(performance-unnecessary-copy-initialization)
            CHECK(soundBufferCopy.getSamples() != nullptr);
            CHECK(soundBufferCopy.getSampleCount() == 87'798);
            CHECK(soundBufferCopy.getSampleRate() == 44'100);
            CHECK(soundBufferCopy.getChannelCount() == 1);
            CHECK(soundBufferCopy.getDuration() == za::microseconds(1'990'884));
        }

        SECTION("Assignment")
        {
            za::SoundBuffer soundBufferCopy = za::SoundBuffer::loadFromFile("doodle_pop.ogg").value();
            soundBufferCopy                 = soundBuffer;
            CHECK(soundBufferCopy.getSamples() != nullptr);
            CHECK(soundBufferCopy.getSampleCount() == 87'798);
            CHECK(soundBufferCopy.getSampleRate() == 44'100);
            CHECK(soundBufferCopy.getChannelCount() == 1);
            CHECK(soundBufferCopy.getDuration() == za::microseconds(1'990'884));
        }
    }

    SECTION("Move construction")
    {
        auto soundBuffer = za::SoundBuffer::loadFromFile("ding.flac").value();

        const za::SoundBuffer soundBufferMove(ZA_MOVE(soundBuffer));
        CHECK(soundBufferMove.getSamples() != nullptr);
        CHECK(soundBufferMove.getSampleCount() == 87'798);
        CHECK(soundBufferMove.getSampleRate() == 44'100);
        CHECK(soundBufferMove.getChannelCount() == 1);
        CHECK(soundBufferMove.getDuration() == za::microseconds(1'990'884));
    }

    SECTION("Move assignmment")
    {
        auto soundBuffer     = za::SoundBuffer::loadFromFile("ding.flac").value();
        auto soundBufferMove = za::SoundBuffer::loadFromFile("ding.flac").value();

        soundBufferMove = ZA_MOVE(soundBuffer);
        CHECK(soundBufferMove.getSamples() != nullptr);
        CHECK(soundBufferMove.getSampleCount() == 87'798);
        CHECK(soundBufferMove.getSampleRate() == 44'100);
        CHECK(soundBufferMove.getChannelCount() == 1);
        CHECK(soundBufferMove.getDuration() == za::microseconds(1'990'884));
    }

    SECTION("loadFromFile()")
    {
        SECTION("Invalid filename")
        {
            CHECK(!za::SoundBuffer::loadFromFile("does/not/exist.wav").hasValue());
        }

        SECTION("Valid file")
        {
            const za::Path filenameSuffixes[]{U"", U"-ń", U"-🐌"};

            for (const auto& filenameSuffix : filenameSuffixes)
            {
                const za::Path filename = U"ding" + filenameSuffix + U".flac";

                const auto soundBuffer = za::SoundBuffer::loadFromFile("ding.flac").value();

                CHECK(soundBuffer.getSamples() != nullptr);
                CHECK(soundBuffer.getSampleCount() == 87'798);
                CHECK(soundBuffer.getSampleRate() == 44'100);
                CHECK(soundBuffer.getChannelCount() == 1);
                CHECK(soundBuffer.getDuration() == za::microseconds(1'990'884));
            }
        }
    }

    SECTION("loadFromMemory()")
    {
        SECTION("Invalid memory")
        {
            constexpr unsigned char memory[5]{};
            CHECK(!za::SoundBuffer::loadFromMemory(memory, 5).hasValue());
        }

        SECTION("Valid memory")
        {
            const auto memory      = loadIntoMemory("ding.flac");
            const auto soundBuffer = za::SoundBuffer::loadFromMemory(memory.data(), memory.size()).value();
            CHECK(soundBuffer.getSamples() != nullptr);
            CHECK(soundBuffer.getSampleCount() == 87'798);
            CHECK(soundBuffer.getSampleRate() == 44'100);
            CHECK(soundBuffer.getChannelCount() == 1);
            CHECK(soundBuffer.getDuration() == za::microseconds(1'990'884));
        }
    }

    SECTION("loadFromStream()")
    {
        auto       stream      = za::FileInputStream::open("ding.flac").value();
        const auto soundBuffer = za::SoundBuffer::loadFromStream(stream).value();
        CHECK(soundBuffer.getSamples() != nullptr);
        CHECK(soundBuffer.getSampleCount() == 87'798);
        CHECK(soundBuffer.getSampleRate() == 44'100);
        CHECK(soundBuffer.getChannelCount() == 1);
        CHECK(soundBuffer.getDuration() == za::microseconds(1'990'884));
    }

    SECTION("saveToFile()")
    {
        // Disambiguate temp-file names across parallel test processes
        const auto     pidSuffix = za::Path(za::fmtToString("-{}", za::testing::getProcessUniqueId()));
        const za::Path stems[]{U"tmp" + pidSuffix, U"tmp-ń" + pidSuffix, U"tmp-🐌" + pidSuffix};
        const za::Path extensions[]{U".wav", U".ogg", U".flac"};

        for (const auto& stem : stems)
        {
            for (const auto& extension : extensions)
            {
                const auto filename = za::Path::getTempDirectory().value() / (stem + extension);

                {
                    const auto soundBuffer = za::SoundBuffer::loadFromFile("ding.flac").value();
                    REQUIRE(soundBuffer.saveToFile(filename));
                }

                const auto soundBuffer = za::SoundBuffer::loadFromFile(filename).value();
                CHECK(soundBuffer.getSamples() != nullptr);
                CHECK(soundBuffer.getSampleCount() == 87'798);
                CHECK(soundBuffer.getSampleRate() == 44'100);
                CHECK(soundBuffer.getChannelCount() == 1);
                CHECK(soundBuffer.getDuration() == za::microseconds(1'990'884));

                CHECK(filename.removeFromDisk());
            }
        }
    }
}
