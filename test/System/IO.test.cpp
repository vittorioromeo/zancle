#include "StringifyStdStringUtil.hpp"
#include "StringifyStringViewUtil.hpp"
#include "StringifyZbStringUtil.hpp"
#include "TemporaryFile.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/IO/IO.hpp"

#include "Zancle/Scn/Scn.hpp"
#include "Zancle/Scn/ScnString.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/StringView.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/PtrDiffT.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsDefaultConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"

#include <string>


using za::testing::TemporaryFile;


TEST_CASE("[System] za::OutFile and za::InFile")
{
    using namespace za::literals;

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_DEFAULT_CONSTRUCTIBLE(za::OutFile));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::OutFile));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::OutFile));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::OutFile));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::OutFile));

        STATIC_CHECK(!ZA_IS_DEFAULT_CONSTRUCTIBLE(za::InFile));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::InFile));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::InFile));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::InFile));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::InFile));
    }

    SECTION("openFromFile returns nullOpt on non-openable path")
    {
        // A clearly bogus path; the factory should report failure rather than
        // returning a half-constructed object.
        const auto opt = za::InFile::open(za::Path{"/this/path/does/not/exist__"}, za::FileOpenMode::bin);
        CHECK(!opt.hasValue());
    }

    SECTION("Write then read binary payload")
    {
        const TemporaryFile temporaryFile;

        {
            auto optOfs = za::OutFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
            REQUIRE(optOfs.hasValue());

            constexpr za::StringView payload = "Hello 42 world"_sv;
            CHECK(optOfs->write(payload.data(), payload.size()));
            CHECK(optOfs->flush());
            // Destructor closes when `optOfs` goes out of scope below.
        }

        {
            auto optIfs = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
            REQUIRE(optIfs.hasValue());

            char      buffer[14] = {};
            za::SizeT got        = 0;
            CHECK(optIfs->read(buffer, 14, got));
            CHECK(got == 14u);
            CHECK(za::StringView(buffer, 14) == "Hello 42 world"_sv);
        }
    }

    SECTION("OutFile tellPos and seekPos")
    {
        const TemporaryFile temporaryFile;

        auto optOfs = za::OutFile::open(temporaryFile.getPath());
        REQUIRE(optOfs.hasValue());
        CHECK(optOfs->write("abcdef", 6));

        za::PtrDiffT pos = 0;
        CHECK(optOfs->tellPos(pos));
        CHECK(pos == 6);

        CHECK(optOfs->seekPos(3));
        CHECK(optOfs->tellPos(pos));
        CHECK(pos == 3);

        CHECK(optOfs->write("XYZ", 3));
        CHECK(optOfs->tellPos(pos));
        CHECK(pos == 6);
        optOfs.reset(); // explicit close before reopening for read

        auto optIfs = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
        REQUIRE(optIfs.hasValue());
        char      buffer[7] = {};
        za::SizeT got       = 0;
        CHECK(optIfs->read(buffer, 6, got));
        CHECK(got == 6u);
        CHECK(za::StringView(buffer, 6) == "abcXYZ"_sv);
    }

    SECTION("InFile seekPos and tellPos")
    {
        const TemporaryFile temporaryFile("Hello world");

        auto optIfs = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
        REQUIRE(optIfs.hasValue());

        za::PtrDiffT pos = -1;
        CHECK(optIfs->tellPos(pos));
        CHECK(pos == 0);

        CHECK(optIfs->seekPos(6));
        CHECK(optIfs->tellPos(pos));
        CHECK(pos == 6);

        char      buffer[6] = {};
        za::SizeT got       = 0;
        CHECK(optIfs->read(buffer, 5, got));
        CHECK(got == 5u);
        CHECK(za::StringView(buffer, 5) == "world"_sv);

        CHECK(optIfs->seekPos(0, za::SeekDir::beg));
        CHECK(optIfs->tellPos(pos));
        CHECK(pos == 0);

        CHECK(optIfs->seekPos(0, za::SeekDir::end));
        CHECK(optIfs->tellPos(pos));
        CHECK(pos == 11);
    }

    SECTION("InFile seekPos(cur) compensates for a live peek cache")
    {
        // Regression: after `peek()` the raw FILE* cursor is one byte
        // ahead of the logical cursor. `seekPos(N, cur)` must seek from
        // the *logical* position, not the raw FILE* position.
        const TemporaryFile temporaryFile("abcdef");

        auto optIfs = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
        REQUIRE(optIfs.hasValue());

        const auto peeked = optIfs->peek();
        REQUIRE(peeked.hasValue());
        CHECK(*peeked == 'a');

        // Logical cursor is at 0 (we peeked but didn't consume).
        // `seekPos(1, cur)` should land at logical position 1 -> next read 'b'.
        CHECK(optIfs->seekPos(1, za::SeekDir::cur));

        char      ch  = '\0';
        za::SizeT got = 0;
        CHECK(optIfs->read(&ch, 1, got));
        CHECK(got == 1u);
        CHECK(ch == 'b');
    }

    SECTION("InFile reports EOF after reading past the end")
    {
        const TemporaryFile temporaryFile("abc");

        auto optIfs = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
        REQUIRE(optIfs.hasValue());

        char      buffer[8] = {};
        za::SizeT got       = 0;

        // The read returns success with a short count -- EOF is normal,
        // not an error.
        CHECK(optIfs->read(buffer, 8, got));
        CHECK(got == 3u);
        CHECK(optIfs->isEOF());
    }

    SECTION("InFile open with FileOpenMode::ate seeks to end")
    {
        const TemporaryFile temporaryFile("abc");

        auto optIfs = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin | za::FileOpenMode::ate);
        REQUIRE(optIfs.hasValue());

        za::PtrDiffT pos = 0;
        CHECK(optIfs->tellPos(pos));
        CHECK(pos == 3);
    }

    SECTION("Move semantics")
    {
        const TemporaryFile temporaryFile("Hello world");

        SECTION("OutFile move constructor")
        {
            const TemporaryFile target;

            auto optMoved = za::OutFile::open(target.getPath());
            REQUIRE(optMoved.hasValue());

            za::OutFile ofs = ZA_MOVE(*optMoved);
            CHECK(ofs.write("moved", 5));
            // ofs's destructor closes when this scope ends.
            {
                za::OutFile temp = ZA_MOVE(ofs);
                (void)temp;
            }

            auto optIfs = za::InFile::open(target.getPath(), za::FileOpenMode::bin);
            REQUIRE(optIfs.hasValue());

            char      buffer[5] = {};
            za::SizeT got       = 0;
            CHECK(optIfs->read(buffer, 5, got));
            CHECK(got == 5u);
            CHECK(za::StringView(buffer, 5) == "moved"_sv);
        }

        SECTION("InFile move constructor")
        {
            auto optMoved = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
            REQUIRE(optMoved.hasValue());

            za::InFile ifs = ZA_MOVE(*optMoved);

            char      buffer[5] = {};
            za::SizeT got       = 0;
            CHECK(ifs.read(buffer, 5, got));
            CHECK(got == 5u);
            CHECK(za::StringView(buffer, 5) == "Hello"_sv);
        }
    }
}


TEST_CASE("[System] za::InFile peek/consume cache semantics")
{
    using namespace za::literals;

    const TemporaryFile temporaryFile("ABCD"_sv);

    SECTION("peek without consume is idempotent")
    {
        auto optFile = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
        REQUIRE(optFile.hasValue());

        CHECK(*optFile->peek() == 'A');
        CHECK(*optFile->peek() == 'A'); // Still 'A' -- cache hit, no advance.

        optFile->consume();
        CHECK(*optFile->peek() == 'B');
    }

    SECTION("peek + read delivers cached byte first")
    {
        auto optFile = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
        REQUIRE(optFile.hasValue());

        CHECK(*optFile->peek() == 'A');

        char      buf[4]{};
        za::SizeT got = 0u;
        CHECK(optFile->read(buf, 3, got));
        CHECK(got == 3u);
        CHECK(za::StringView(buf, 3) == "ABC"_sv);
    }

    SECTION("tellPos accounts for peeked-but-not-consumed byte")
    {
        auto optFile = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
        REQUIRE(optFile.hasValue());

        za::PtrDiffT pos = -1;
        CHECK(optFile->tellPos(pos));
        CHECK(pos == 0);

        CHECK(*optFile->peek() == 'A');
        CHECK(optFile->tellPos(pos));
        CHECK(pos == 0); // The peek must not advance the visible position.

        optFile->consume();
        CHECK(optFile->tellPos(pos));
        CHECK(pos == 1);
    }

    SECTION("seekPos drops the peek cache")
    {
        auto optFile = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
        REQUIRE(optFile.hasValue());

        CHECK(*optFile->peek() == 'A');
        CHECK(optFile->seekPos(2)); // jump to 'C'
        CHECK(*optFile->peek() == 'C');
    }

    SECTION("peek at EOF returns nullOpt")
    {
        auto optFile = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
        REQUIRE(optFile.hasValue());

        CHECK(optFile->seekPos(4));
        CHECK_FALSE(optFile->peek().hasValue());
        CHECK(optFile->isEOF());
    }
}


TEST_CASE("[System] za::InFile as za::ScnSource")
{
    using namespace za::literals;

    SECTION("scnReadLine over a multi-KiB file")
    {
        // Big enough to exercise the FILE* internal buffer transitioning
        // mid-line (stdio default buffer is typically 4-8 KiB).
        za::String          payload;
        constexpr za::SizeT longLineSize = 16u * 1024u;
        payload.reserve(longLineSize + 16u);
        for (za::SizeT i = 0u; i < longLineSize; ++i)
            payload.append('x');
        payload.append("\nshort\n");

        const TemporaryFile temporaryFile(payload.toStringView());

        auto optFile = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
        REQUIRE(optFile.hasValue());

        za::String line;
        CHECK(za::scnReadLine(*optFile, line));
        CHECK(line.size() == longLineSize);

        CHECK(za::scnReadLine(*optFile, line));
        CHECK(line == za::String{"short"});

        CHECK(!za::scnReadLine(*optFile, line));
        CHECK(za::scnAtEnd(*optFile));
    }

    SECTION("scn<int> / scn<String> from a file")
    {
        const TemporaryFile temporaryFile("42 hello\n3.14 world"_sv);

        auto optFile = za::InFile::open(temporaryFile.getPath(), za::FileOpenMode::bin);
        REQUIRE(optFile.hasValue());

        const auto v = za::scn<int>(*optFile);
        REQUIRE(v.hasValue());
        CHECK(*v == 42);

        const auto tok = za::scn<za::String>(*optFile);
        REQUIRE(tok.hasValue());
        CHECK(*tok == za::String{"hello"});
    }
}


TEST_CASE("[System] za::writeToFile and za::readFromFile")
{
    using namespace za::literals;

    SECTION("Round-trip")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, "Hello world"_sv));

        std::string contents;
        CHECK(za::readFromFile(path, contents));
        CHECK(contents == "Hello world");
    }

    SECTION("Read empty file")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, ""_sv));

        std::string contents = "stale";
        CHECK(za::readFromFile(path, contents));
        CHECK(contents.empty());
    }

    SECTION("Read missing file fails")
    {
        const TemporaryFile temporaryFile;
        // The file does not exist on disk yet.
        const za::Path& path = temporaryFile.getPath();

        std::string contents;
        CHECK(!za::readFromFile(path, contents));
    }

    SECTION("Binary content with embedded NULs and high bytes is preserved")
    {
        // The native fast path goes through a single buffered read, so make sure
        // it doesn't truncate at NUL or mangle non-ASCII bytes.
        const char           raw[] = {'a', '\0', 'b', '\xff', '\x01', '\x80', 'z'};
        const za::StringView payload(raw, sizeof(raw));

        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, payload));

        std::string contents;
        CHECK(za::readFromFile(path, contents));
        CHECK(contents.size() == sizeof(raw));
        CHECK(za::StringView(contents.data(), contents.size()) == payload);
    }

    SECTION("Large file (multiple MiB) round-trips byte-exactly")
    {
        // Sized to comfortably exceed any plausible internal buffer in the
        // fallback iostream path (typically 4-8 KiB) and force the native paths
        // to issue more than one read/ReadFile call's worth of work.
        constexpr za::SizeT bytes = 4u * 1024u * 1024u + 17u; // 4 MiB + odd tail

        std::string payload;
        payload.resize(bytes);

        // Deterministic but non-trivial content; covers all 256 byte values.
        za::SizeT seed = 0x12'34'56'78u;
        for (za::SizeT i = 0; i < bytes; ++i)
        {
            seed       = seed * 1'103'515'245u + 12'345u;
            payload[i] = static_cast<char>(seed >> 16);
        }

        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, za::StringView(payload.data(), payload.size())));

        std::string contents;
        CHECK(za::readFromFile(path, contents));
        REQUIRE(contents.size() == payload.size());
        CHECK(contents == payload);
    }

    SECTION("Read overwrites pre-existing content in target")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, "short"_sv));

        std::string contents = "this is a much longer pre-existing string that should be overwritten";
        CHECK(za::readFromFile(path, contents));
        CHECK(contents == "short");
    }

    SECTION("Round-trip into za::String")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, "Hello za::String"_sv));

        za::String contents;
        CHECK(za::readFromFile(path, contents));
        CHECK(contents == za::String{"Hello za::String"});
    }

    SECTION("Read empty file into za::String")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, ""_sv));

        za::String contents{"stale"};
        CHECK(za::readFromFile(path, contents));
        CHECK(contents.empty());
    }

    SECTION("Read missing file into za::String fails")
    {
        const TemporaryFile temporaryFile;
        // File never created.
        const za::Path& path = temporaryFile.getPath();

        za::String contents;
        CHECK(!za::readFromFile(path, contents));
    }

    SECTION("Round-trip into za::Vector<char>")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, "Hello vector"_sv));

        za::Vector<char> contents;
        CHECK(za::readFromFile(path, contents));
        REQUIRE(contents.size() == 12u);
        CHECK(za::StringView(contents.data(), contents.size()) == "Hello vector"_sv);
    }

    SECTION("Read empty file into za::Vector<char>")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, ""_sv));

        za::Vector<char> contents;
        // Pre-populate to verify that an empty read clears existing content.
        contents.pushBack('x');
        contents.pushBack('y');
        CHECK(za::readFromFile(path, contents));
        CHECK(contents.empty());
    }

    SECTION("Read missing file into za::Vector<char> fails")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        za::Vector<char> contents;
        CHECK(!za::readFromFile(path, contents));
    }

    SECTION("Read into za::Vector<char> reuses capacity across calls")
    {
        // Two reads into the same Vector: the second (smaller) read should not
        // grow capacity, and the first's allocation should already be amortized.
        const TemporaryFile temporaryFile1;
        const TemporaryFile temporaryFile2;
        const za::Path&     path1 = temporaryFile1.getPath();
        const za::Path&     path2 = temporaryFile2.getPath();

        // Make file1 large enough to definitely allocate, file2 strictly smaller.
        std::string big(1024u, 'A');
        CHECK(za::writeToFile(path1, za::StringView{big.data(), big.size()}));
        CHECK(za::writeToFile(path2, "small"_sv));

        za::Vector<char> buffer;
        CHECK(za::readFromFile(path1, buffer));
        const auto capacityAfterFirst = buffer.capacity();
        REQUIRE(buffer.size() == 1024u);

        CHECK(za::readFromFile(path2, buffer));
        CHECK(buffer.size() == 5u);
        CHECK(za::StringView(buffer.data(), buffer.size()) == "small"_sv);
        // The smaller read must NOT shrink or re-grow capacity.
        CHECK(buffer.capacity() == capacityAfterFirst);
    }

    SECTION("Read into za::Vector<char> overwrites pre-existing content")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, "abc"_sv));

        za::Vector<char> buffer;
        for (char c : {'X', 'Y', 'Z', 'W', 'V'})
            buffer.pushBack(c);

        CHECK(za::readFromFile(path, buffer));
        CHECK(buffer.size() == 3u);
        CHECK(za::StringView(buffer.data(), buffer.size()) == "abc"_sv);
    }

    SECTION("Binary content with embedded NULs round-trips through za::Vector<char>")
    {
        const char           raw[] = {'a', '\0', 'b', '\xff', '\x01', '\x80', 'z'};
        const za::StringView payload(raw, sizeof(raw));

        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, payload));

        za::Vector<char> contents;
        CHECK(za::readFromFile(path, contents));
        REQUIRE(contents.size() == sizeof(raw));
        CHECK(za::StringView(contents.data(), contents.size()) == payload);
    }

    SECTION("StringView overloads")
    {
        // Exercise the `za::StringView` filename overloads: the rest of the
        // suite uses the `Path` overloads; these tests cover the alternate API.
        const TemporaryFile  temporaryFile;
        const auto           pathOwning = temporaryFile.getPath().to<std::string>();
        const za::StringView pathView{pathOwning.data(), pathOwning.size()};

        CHECK(za::writeToFile(pathView, "Hello via StringView"_sv));

        SECTION("Read into std::string via StringView")
        {
            std::string contents;
            CHECK(za::readFromFile(pathView, contents));
            CHECK(contents == "Hello via StringView");
        }

        SECTION("Read into za::String via StringView")
        {
            za::String contents;
            CHECK(za::readFromFile(pathView, contents));
            CHECK(contents == za::String{"Hello via StringView"});
        }

        SECTION("Read into za::Vector<char> via StringView")
        {
            za::Vector<char> contents;
            CHECK(za::readFromFile(pathView, contents));
            REQUIRE(contents.size() == 20u);
            CHECK(za::StringView(contents.data(), contents.size()) == "Hello via StringView"_sv);
        }
    }
}


TEST_CASE("[System] za::appendFromFile")
{
    using namespace za::literals;

    SECTION("Append into empty target equals readFromFile")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, "alpha"_sv));

        za::Vector<char> contents;
        CHECK(za::appendFromFile(path, contents));
        REQUIRE(contents.size() == 5u);
        CHECK(za::StringView(contents.data(), contents.size()) == "alpha"_sv);
    }

    SECTION("Append preserves existing content")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, "world"_sv));

        za::Vector<char> contents;
        for (char c : {'h', 'e', 'l', 'l', 'o', ' '})
            contents.pushBack(c);

        CHECK(za::appendFromFile(path, contents));
        REQUIRE(contents.size() == 11u);
        CHECK(za::StringView(contents.data(), contents.size()) == "hello world"_sv);
    }

    SECTION("Multiple successive appends concatenate cleanly")
    {
        const TemporaryFile temporaryFile1;
        const TemporaryFile temporaryFile2;
        const TemporaryFile temporaryFile3;

        CHECK(za::writeToFile(temporaryFile1.getPath(), "foo"_sv));
        CHECK(za::writeToFile(temporaryFile2.getPath(), "bar"_sv));
        CHECK(za::writeToFile(temporaryFile3.getPath(), "baz"_sv));

        za::Vector<char> contents;
        CHECK(za::appendFromFile(temporaryFile1.getPath(), contents));
        CHECK(za::appendFromFile(temporaryFile2.getPath(), contents));
        CHECK(za::appendFromFile(temporaryFile3.getPath(), contents));

        REQUIRE(contents.size() == 9u);
        CHECK(za::StringView(contents.data(), contents.size()) == "foobarbaz"_sv);
    }

    SECTION("Append from empty file leaves target unchanged")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, ""_sv));

        za::Vector<char> contents;
        for (char c : {'k', 'e', 'e', 'p'})
            contents.pushBack(c);

        CHECK(za::appendFromFile(path, contents));
        REQUIRE(contents.size() == 4u);
        CHECK(za::StringView(contents.data(), contents.size()) == "keep"_sv);
    }

    SECTION("Append from missing file fails and does not touch target")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        za::Vector<char> contents;
        for (char c : {'i', 'n', 't', 'a', 'c', 't'})
            contents.pushBack(c);

        CHECK(!za::appendFromFile(path, contents));
        REQUIRE(contents.size() == 6u);
        CHECK(za::StringView(contents.data(), contents.size()) == "intact"_sv);
    }

    SECTION("Append via StringView filename overload")
    {
        const TemporaryFile  temporaryFile;
        const auto           pathOwning = temporaryFile.getPath().to<std::string>();
        const za::StringView pathView{pathOwning.data(), pathOwning.size()};

        CHECK(za::writeToFile(pathView, " world"_sv));

        za::Vector<char> contents;
        for (char c : {'h', 'e', 'l', 'l', 'o'})
            contents.pushBack(c);

        CHECK(za::appendFromFile(pathView, contents));
        REQUIRE(contents.size() == 11u);
        CHECK(za::StringView(contents.data(), contents.size()) == "hello world"_sv);
    }
}


TEST_CASE("[System] za::getThreadLocalScratchCharBuffer")
{
    using namespace za::literals;

    SECTION("Returns the same reference on repeated calls (same thread)")
    {
        za::Vector<char>& a = za::getThreadLocalScratchCharBuffer();
        za::Vector<char>& b = za::getThreadLocalScratchCharBuffer();
        CHECK(&a == &b);
    }

    SECTION("Buffer is usable as a readFromFile target")
    {
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        CHECK(za::writeToFile(path, "scratch payload"_sv));

        za::Vector<char>& scratch = za::getThreadLocalScratchCharBuffer();
        CHECK(za::readFromFile(path, scratch));
        CHECK(za::StringView(scratch.data(), scratch.size()) == "scratch payload"_sv);
    }

    SECTION("Capacity is preserved across reads on the same thread")
    {
        // Successive smaller reads must reuse the buffer that the first read
        // grew, since this is the whole point of the scratch.
        const TemporaryFile temporaryFile;
        const za::Path&     path = temporaryFile.getPath();

        std::string big(2048u, 'q');
        CHECK(za::writeToFile(path, za::StringView{big.data(), big.size()}));

        za::Vector<char>& scratch = za::getThreadLocalScratchCharBuffer();
        CHECK(za::readFromFile(path, scratch));
        const auto capacityAfterBig = scratch.capacity();
        REQUIRE(capacityAfterBig >= 2048u);

        CHECK(za::writeToFile(path, "tiny"_sv));
        CHECK(za::readFromFile(path, scratch));
        CHECK(scratch.size() == 4u);
        CHECK(scratch.capacity() == capacityAfterBig); // not shrunk, not regrown
    }
}


TEST_CASE("[System] za::FileOpenMode")
{
    SECTION("Bitwise operations")
    {
        constexpr auto combined = za::FileOpenMode::in | za::FileOpenMode::bin;
        STATIC_CHECK((combined & za::FileOpenMode::in) == za::FileOpenMode::in);
        STATIC_CHECK((combined & za::FileOpenMode::bin) == za::FileOpenMode::bin);
        STATIC_CHECK((combined & za::FileOpenMode::out) == za::FileOpenMode::none);
    }
}
