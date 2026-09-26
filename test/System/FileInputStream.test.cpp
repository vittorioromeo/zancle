#include "StringifyOptionalUtil.hpp"
#include "StringifyStringViewUtil.hpp"
#include "TemporaryFile.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/IO/FileInputStream.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/StringView.hpp"

#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsDefaultConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


using za::testing::TemporaryFile;

TEST_CASE("[System] za::FileInputStream")
{
    using namespace za::literals;

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_DEFAULT_CONSTRUCTIBLE(za::FileInputStream));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::FileInputStream));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::FileInputStream));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::FileInputStream));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::FileInputStream));
    }

    const TemporaryFile temporaryFile("Hello world");
    char                buffer[32];

    SECTION("Move semantics")
    {
        SECTION("Move constructor")
        {
            auto                movedFileInputStream = za::FileInputStream::open(temporaryFile.getPath()).value();
            za::FileInputStream fileInputStream      = ZA_MOVE(movedFileInputStream);
            CHECK(fileInputStream.read(buffer, 6).value() == 6);
            CHECK(fileInputStream.tell().value() == 6);
            CHECK(fileInputStream.getSize().value() == 11);
            CHECK(za::StringView(buffer, 6) == "Hello "_sv);
        }

        SECTION("Move assignment")
        {
            auto                movedFileInputStream = za::FileInputStream::open(temporaryFile.getPath()).value();
            const TemporaryFile temporaryFile2("Hello world the sequel");
            auto                fileInputStream = za::FileInputStream::open(temporaryFile2.getPath()).value();
            fileInputStream                     = ZA_MOVE(movedFileInputStream);
            CHECK(fileInputStream.read(buffer, 6).value() == 6);
            CHECK(fileInputStream.tell().value() == 6);
            CHECK(fileInputStream.getSize().value() == 11);
            CHECK(za::StringView(buffer, 6) == "Hello "_sv);
        }
    }

    SECTION("Temporary file stream")
    {
        auto fileInputStream = za::FileInputStream::open(temporaryFile.getPath()).value();
        CHECK(fileInputStream.read(buffer, 5).value() == 5);
        CHECK(fileInputStream.tell().value() == 5);
        CHECK(fileInputStream.getSize().value() == 11);
        CHECK(za::StringView(buffer, 5) == "Hello"_sv);
        CHECK(fileInputStream.seek(6).value() == 6);
        CHECK(fileInputStream.tell().value() == 6);
    }

#ifndef ZA_SYSTEM_EMSCRIPTEN // TODO P1: throws an exception on Emscripten
    SECTION("open()")
    {
        const za::Path filenameSuffixes[] = {U"", U"-ń", U"-🐌"};
        for (const auto& filenameSuffix : filenameSuffixes)
        {
            const za::Path filename = U"test" + filenameSuffix + U".txt";
            INFO("Filename: " << filename.to<za::String>().cStr());

            auto fileInputStream = za::FileInputStream::open(filename).value();
            CHECK(fileInputStream.read(buffer, 5).value() == 5);
            CHECK(fileInputStream.tell().value() == 5);
            CHECK(fileInputStream.getSize().value() == 12);
            CHECK(za::StringView(buffer, 5) == "Hello"_sv);
            CHECK(fileInputStream.seek(6).value() == 6);
            CHECK(fileInputStream.tell().value() == 6);
        }
    }
#endif
}
