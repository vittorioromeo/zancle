#include "Zancle/Audio/InputSoundFile.hpp"

// Other 1st party headers
#include "LoadIntoMemoryUtil.hpp"
#include "StringifyArrayUtil.hpp" // IWYU pragma: keep
#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/IO/FileInputStream.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/Array.hpp"

#include "Zancle/Base/IntTypes.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsDefaultConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


TEST_CASE("[Audio] za::InputSoundFile")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_DEFAULT_CONSTRUCTIBLE(za::InputSoundFile));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::InputSoundFile));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::InputSoundFile));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::InputSoundFile));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::InputSoundFile));
    }

    SECTION("openFromFile()")
    {
        SECTION("Invalid file")
        {
            CHECK(!za::InputSoundFile::openFromFile("does/not/exist.wav").hasValue());
        }

        SECTION("Valid file")
        {
            const za::Path filenameSuffixes[] = {U"", U"-ń", U"-🐌"};
            for (const auto& filenameSuffix : filenameSuffixes)
            {

                SECTION("flac")
                {
                    const za::Path filename = U"ding" + filenameSuffix + U".flac";

                    const auto inputSoundFile = za::InputSoundFile::openFromFile(filename).value();
                    CHECK(inputSoundFile.getSampleCount() == 87'798);
                    CHECK(inputSoundFile.getChannelCount() == 1);
                    CHECK(inputSoundFile.getSampleRate() == 44'100);
                    CHECK(inputSoundFile.getDuration() == za::microseconds(1'990'884));
                    CHECK(inputSoundFile.getTimeOffset(/* sampleOffset */ 0u) == za::Time{});
                }

                SECTION("mp3")
                {
                    const za::Path filename = U"ding" + filenameSuffix + U".mp3";

                    const auto inputSoundFile = za::InputSoundFile::openFromFile(filename).value();
                    CHECK(inputSoundFile.getSampleCount() == 87'798);
                    CHECK(inputSoundFile.getChannelCount() == 1);
                    CHECK(inputSoundFile.getSampleRate() == 44'100);
                    CHECK(inputSoundFile.getDuration() == za::microseconds(1'990'884));
                    CHECK(inputSoundFile.getTimeOffset(/* sampleOffset */ 0u) == za::Time{});
                }

                SECTION("ogg")
                {
                    const za::Path filename = U"doodle_pop" + filenameSuffix + U".ogg";

                    const auto inputSoundFile = za::InputSoundFile::openFromFile(filename).value();
                    CHECK(inputSoundFile.getSampleCount() == 2'116'992);
                    CHECK(inputSoundFile.getChannelCount() == 2);
                    CHECK(inputSoundFile.getSampleRate() == 44'100);
                    CHECK(inputSoundFile.getDuration() == za::microseconds(24'002'176));
                    CHECK(inputSoundFile.getTimeOffset(/* sampleOffset */ 0u) == za::Time{});
                }

                SECTION("wav")
                {
                    const za::Path filename = U"killdeer" + filenameSuffix + U".wav";

                    const auto inputSoundFile = za::InputSoundFile::openFromFile(filename).value();
                    CHECK(inputSoundFile.getSampleCount() == 112'941);
                    CHECK(inputSoundFile.getChannelCount() == 1);
                    CHECK(inputSoundFile.getSampleRate() == 22'050);
                    CHECK(inputSoundFile.getDuration() == za::microseconds(5'122'040));
                    CHECK(inputSoundFile.getTimeOffset(/* sampleOffset */ 0u) == za::Time{});
                }
            }
        }
    }

    SECTION("openFromMemory()")
    {
        const auto memory         = loadIntoMemory("killdeer.wav");
        const auto inputSoundFile = za::InputSoundFile::openFromMemory(memory.data(), memory.size()).value();
        CHECK(inputSoundFile.getSampleCount() == 112'941);
        CHECK(inputSoundFile.getChannelCount() == 1);
        CHECK(inputSoundFile.getSampleRate() == 22'050);
        CHECK(inputSoundFile.getDuration() == za::microseconds(5'122'040));
        CHECK(inputSoundFile.getTimeOffset(/* sampleOffset */ 0u) == za::Time{});
    }

    SECTION("openFromStream()")
    {
        SECTION("flac")
        {
            auto       stream         = za::FileInputStream::open("ding.flac").value();
            const auto inputSoundFile = za::InputSoundFile::openFromStream(stream).value();
            CHECK(inputSoundFile.getSampleCount() == 87'798);
            CHECK(inputSoundFile.getChannelCount() == 1);
            CHECK(inputSoundFile.getSampleRate() == 44'100);
            CHECK(inputSoundFile.getDuration() == za::microseconds(1'990'884));
            CHECK(inputSoundFile.getTimeOffset(/* sampleOffset */ 0u) == za::Time{});
        }

        SECTION("mp3")
        {
            auto       stream         = za::FileInputStream::open("ding.mp3").value();
            const auto inputSoundFile = za::InputSoundFile::openFromStream(stream).value();
            CHECK(inputSoundFile.getSampleCount() == 87'798);
            CHECK(inputSoundFile.getChannelCount() == 1);
            CHECK(inputSoundFile.getSampleRate() == 44'100);
            CHECK(inputSoundFile.getDuration() == za::microseconds(1'990'884));
            CHECK(inputSoundFile.getTimeOffset(/* sampleOffset */ 0u) == za::Time{});
        }

        SECTION("ogg")
        {
            auto       stream         = za::FileInputStream::open("doodle_pop.ogg").value();
            const auto inputSoundFile = za::InputSoundFile::openFromStream(stream).value();
            CHECK(inputSoundFile.getSampleCount() == 2'116'992);
            CHECK(inputSoundFile.getChannelCount() == 2);
            CHECK(inputSoundFile.getSampleRate() == 44'100);
            CHECK(inputSoundFile.getDuration() == za::microseconds(24'002'176));
            CHECK(inputSoundFile.getTimeOffset(/* sampleOffset */ 0u) == za::Time{});
        }

        SECTION("wav")
        {
            auto       stream         = za::FileInputStream::open("killdeer.wav").value();
            const auto inputSoundFile = za::InputSoundFile::openFromStream(stream).value();
            CHECK(inputSoundFile.getSampleCount() == 112'941);
            CHECK(inputSoundFile.getChannelCount() == 1);
            CHECK(inputSoundFile.getSampleRate() == 22'050);
            CHECK(inputSoundFile.getDuration() == za::microseconds(5'122'040));
            CHECK(inputSoundFile.getTimeOffset(/* sampleOffset */ 0u) == za::Time{});
        }
    }

    SECTION("seek(za::U64)")
    {
        SECTION("flac")
        {
            auto       inputSoundFile = za::InputSoundFile::openFromFile("ding.flac").value();
            const auto sampleOffset   = inputSoundFile.seek(1000);
            CHECK(inputSoundFile.getTimeOffset(sampleOffset) == za::microseconds(22'675));
            CHECK(sampleOffset == 1000);
        }

        SECTION("mp3")
        {
            auto       inputSoundFile = za::InputSoundFile::openFromFile("ding.mp3").value();
            const auto sampleOffset   = inputSoundFile.seek(1000);
            CHECK(inputSoundFile.getTimeOffset(sampleOffset) == za::microseconds(22'675));
            CHECK(sampleOffset == 1000);
        }

        SECTION("ogg")
        {
            auto       inputSoundFile = za::InputSoundFile::openFromFile("doodle_pop.ogg").value();
            const auto sampleOffset   = inputSoundFile.seek(1000);
            CHECK(inputSoundFile.getTimeOffset(sampleOffset) == za::microseconds(11'337));
            CHECK(sampleOffset == 1000);
        }

        SECTION("wav")
        {
            auto       inputSoundFile = za::InputSoundFile::openFromFile("killdeer.wav").value();
            const auto sampleOffset   = inputSoundFile.seek(1000);
            CHECK(inputSoundFile.getTimeOffset(sampleOffset) == za::microseconds(45'351));
            CHECK(sampleOffset == 1000);
        }
    }

    SECTION("seek(Time)")
    {
        auto       inputSoundFile = za::InputSoundFile::openFromFile("ding.flac").value();
        const auto sampleOffset   = inputSoundFile.seek(za::milliseconds(100));
        CHECK(inputSoundFile.getSampleCount() == 87'798);
        CHECK(inputSoundFile.getChannelCount() == 1);
        CHECK(inputSoundFile.getSampleRate() == 44'100);
        CHECK(inputSoundFile.getDuration() == za::microseconds(1'990'884));
        CHECK(inputSoundFile.getTimeOffset(sampleOffset) == za::milliseconds(100));
        CHECK(sampleOffset == 4410);
    }

    SECTION("read()")
    {
        auto inputSoundFile = za::InputSoundFile::openFromFile("ding.flac").value();

        SECTION("Null address")
        {
            CHECK(inputSoundFile.read(nullptr, 10) == 0);
        }

        za::Array<za::I16, 4> samples{};

        SECTION("Zero count")
        {
            CHECK(inputSoundFile.read(samples.data(), 0) == 0);
        }

        SECTION("Successful read")
        {
            SECTION("flac")
            {
                inputSoundFile = za::InputSoundFile::openFromFile("ding.flac").value();
                CHECK(inputSoundFile.read(samples.data(), samples.size()) == 4);
                CHECK(samples == za::Array<za::I16, 4>{0, 1, -1, 4});
                CHECK(inputSoundFile.read(samples.data(), samples.size()) == 4);
                CHECK(samples == za::Array<za::I16, 4>{1, 4, 9, 6});
            }

            SECTION("mp3")
            {
                inputSoundFile = za::InputSoundFile::openFromFile("ding.mp3").value();
                CHECK(inputSoundFile.read(samples.data(), samples.size()) == 4);
                CHECK(samples == za::Array<za::I16, 4>{0, -2, 0, 2});
                CHECK(inputSoundFile.read(samples.data(), samples.size()) == 4);
                CHECK(samples == za::Array<za::I16, 4>{1, 4, 6, 8});
            }

            SECTION("ogg")
            {
                inputSoundFile = za::InputSoundFile::openFromFile("doodle_pop.ogg").value();
                CHECK(inputSoundFile.read(samples.data(), samples.size()) == 4);
                CHECK(samples == za::Array<za::I16, 4>{-827, -985, -1168, -1319});
                CHECK(inputSoundFile.read(samples.data(), samples.size()) == 4);
                CHECK(samples == za::Array<za::I16, 4>{-1738, -1883, -2358, -2497});
            }

            SECTION("wav")
            {
                // Cannot be tested since reading from a .wav file triggers UB
            }
        }
    }
}
