#include "Zancle/Audio/SoundFileFactory.hpp"

// Other 1st party headers
#include "StringifyOptionalUtil.hpp"
#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Audio/ChannelMap.hpp"
#include "Zancle/Audio/SoundFileReader.hpp"
#include "Zancle/Audio/SoundFileWriter.hpp"

#include "Zancle/IO/FileInputStream.hpp"
#include "Zancle/IO/InputStream.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/IntTypes.hpp"

#include "Zancle/Trait/IsTriviallyCopyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyConstructible.hpp"
#include "Zancle/Trait/IsTriviallyMoveAssignable.hpp"
#include "Zancle/Trait/IsTriviallyMoveConstructible.hpp"


namespace
{

struct NoopSoundFileReader : za::SoundFileReader
{
    static bool check(za::InputStream&)
    {
        return false;
    }

    za::Optional<Info> open(za::InputStream&) override
    {
        return za::nullOpt;
    }

    void seek(za::U64) override
    {
    }

    za::U64 read(za::I16*, za::U64) override
    {
        return 0;
    }
};

struct NoopSoundFileWriter : za::SoundFileWriter
{
    static bool check(const za::Path&)
    {
        return false;
    }

    bool open(const za::Path&, unsigned int, unsigned int, const za::ChannelMap&) override
    {
        return false;
    }

    void write(const za::I16*, za::U64) override
    {
    }
};

} // namespace

TEST_CASE("[Audio] za::SoundFileFactory")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::SoundFileFactory));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::SoundFileFactory));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::SoundFileFactory));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::SoundFileFactory));
    }

    SECTION("isReaderRegistered()")
    {
        CHECK(!za::SoundFileFactory::isReaderRegistered<NoopSoundFileReader>());

        za::SoundFileFactory::registerReader<NoopSoundFileReader>();
        CHECK(za::SoundFileFactory::isReaderRegistered<NoopSoundFileReader>());

        za::SoundFileFactory::unregisterReader<NoopSoundFileReader>();
        CHECK(!za::SoundFileFactory::isReaderRegistered<NoopSoundFileReader>());
    }

    SECTION("isWriterRegistered()")
    {
        CHECK(!za::SoundFileFactory::isWriterRegistered<NoopSoundFileWriter>());

        za::SoundFileFactory::registerWriter<NoopSoundFileWriter>();
        CHECK(za::SoundFileFactory::isWriterRegistered<NoopSoundFileWriter>());

        za::SoundFileFactory::unregisterWriter<NoopSoundFileWriter>();
        CHECK(!za::SoundFileFactory::isWriterRegistered<NoopSoundFileWriter>());
    }

    SECTION("createReaderFromFilename()")
    {
        SECTION("Missing file")
        {
            CHECK((za::SoundFileFactory::createReaderFromFilename("does/not/exist.wav") == nullptr));
        }

        SECTION("Valid file")
        {
            CHECK((za::SoundFileFactory::createReaderFromFilename("ding.flac") != nullptr));
            CHECK((za::SoundFileFactory::createReaderFromFilename("ding.mp3") != nullptr));
            CHECK((za::SoundFileFactory::createReaderFromFilename("doodle_pop.ogg") != nullptr));
            CHECK((za::SoundFileFactory::createReaderFromFilename("killdeer.wav") != nullptr));
        }
    }

    SECTION("createReaderFromStream()")
    {
        za::Optional<za::FileInputStream> stream;

        SECTION("flac")
        {
            stream = za::FileInputStream::open("ding.flac");
        }

        SECTION("mp3")
        {
            stream = za::FileInputStream::open("ding.mp3");
        }

        SECTION("ogg")
        {
            stream = za::FileInputStream::open("doodle_pop.ogg");
        }

        SECTION("wav")
        {
            stream = za::FileInputStream::open("killdeer.wav");
        }

        REQUIRE(stream.hasValue());
        CHECK((za::SoundFileFactory::createReaderFromStream(*stream) != nullptr));
    }

    SECTION("createWriterFromFilename()")
    {
        SECTION("Invalid extension")
        {
            CHECK((za::SoundFileFactory::createWriterFromFilename("cannot/write/to.txt") == nullptr));
        }

        SECTION("Valid extension")
        {
            CHECK((za::SoundFileFactory::createWriterFromFilename("file.flac") != nullptr));
            CHECK((za::SoundFileFactory::createWriterFromFilename("file.mp3") == nullptr)); // Mp3 writing not yet implemented
            CHECK((za::SoundFileFactory::createWriterFromFilename("file.ogg") != nullptr));
            CHECK((za::SoundFileFactory::createWriterFromFilename("file.wav") != nullptr));
        }
    }
}
