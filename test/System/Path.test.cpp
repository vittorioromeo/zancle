#include "StringifyStdStringUtil.hpp" // IWYU: pragma keep
#include "StringifyZbStringUtil.hpp"  // IWYU: pragma keep
#include "TemporaryFile.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Err/FmtPath.hpp" // IWYU pragma: keep -- enables `fmtArg(Path, ...)` for the format test

#include "Zancle/Fmt/FmtToString.hpp"

#include "Zancle/IO/IO.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/StringView.hpp"

#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsDefaultConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"

#include <filesystem>
#include <string>


using za::testing::TemporaryFile;


TEST_CASE("[System] za::Path")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_DEFAULT_CONSTRUCTIBLE(za::Path));
        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::Path));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::Path));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::Path));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::Path));
    }

    SECTION("Default construction")
    {
        const za::Path p;
        CHECK(p.empty());
        CHECK(!p.exists());
    }

    SECTION("Construction from const char*")
    {
        const za::Path p("hello.txt");
        CHECK(!p.empty());
        CHECK(p.to<std::string>() == "hello.txt");
    }

    SECTION("Construction from const wchar_t*")
    {
        const za::Path p(L"hello.txt");
        CHECK(p.to<std::wstring>() == std::wstring(L"hello.txt"));
    }

    SECTION("Construction from const char32_t*")
    {
        const za::Path p(U"hello.txt");
        CHECK(p.to<std::u32string>() == std::u32string(U"hello.txt"));
    }

    SECTION("Construction from std::string")
    {
        const std::string s = "hello.txt";
        const za::Path    p(s);
        CHECK(p.to<std::string>() == "hello.txt");
    }

    SECTION("Construction from std::wstring")
    {
        const std::wstring s = L"hello.txt";
        const za::Path     p(s);
        CHECK(p.to<std::wstring>() == s);
    }

    SECTION("Construction from std::u32string")
    {
        const std::u32string s = U"hello.txt";
        const za::Path       p(s);
        CHECK(p.to<std::u32string>() == s);
    }

    SECTION("Construction from za::String")
    {
        const za::String s("hello.txt");
        const za::Path   p(s);
        CHECK(p.to<za::String>() == za::String("hello.txt"));
    }

    SECTION("Construction from std::filesystem::path")
    {
        const auto     fsp = za::Path("hello.txt").to<std::filesystem::path>();
        const za::Path p(fsp);
        CHECK(p.to<std::filesystem::path>() == fsp);
        CHECK(p == za::Path("hello.txt"));
    }

    SECTION("Copy construction is independent")
    {
        const za::Path a("foo.txt");
        za::Path       b(a);
        CHECK(a == b);
        b /= za::Path("sub");
        CHECK(a == za::Path("foo.txt")); // original unchanged
        CHECK(b != a);
    }

    SECTION("Copy assignment is independent")
    {
        const za::Path a("foo.txt");
        za::Path       b("bar.txt");
        b = a;
        CHECK(a == b);
        b /= za::Path("sub");
        CHECK(a == za::Path("foo.txt")); // original unchanged
    }

    SECTION("Move construction transfers state")
    {
        za::Path       src("foo.txt");
        const za::Path dst(ZA_MOVE(src));
        CHECK(dst.to<std::string>() == "foo.txt");
    }

    SECTION("Move assignment transfers state")
    {
        za::Path src("foo.txt");
        za::Path dst("bar.txt");
        dst = ZA_MOVE(src);
        CHECK(dst.to<std::string>() == "foo.txt");
    }

    SECTION("filename()")
    {
        CHECK(za::Path("/foo/bar.txt").getFilename() == za::Path("bar.txt"));
        CHECK(za::Path("bar.txt").getFilename() == za::Path("bar.txt"));
        CHECK(za::Path("/foo/").getFilename().empty());
        CHECK(za::Path("").getFilename().empty());
    }

    SECTION("extension()")
    {
        CHECK(za::Path("bar.txt").getExtension() == za::Path(".txt"));
        CHECK(za::Path("/foo/bar.TAR.GZ").getExtension() == za::Path(".GZ"));
        CHECK(za::Path("noext").getExtension().empty());
        CHECK(za::Path("").getExtension().empty());
        CHECK(za::Path(".hidden").getExtension().empty()); // leading dot is stem, not extension
    }

    SECTION("parent()")
    {
        CHECK(za::Path("/foo/bar.txt").getParent() == za::Path("/foo"));
        CHECK(za::Path("bar.txt").getParent().empty());
    }

    SECTION("absolute() returns non-empty for non-empty relative path")
    {
        const za::Path rel("hello.txt");
        const auto     abs = rel.getAbsolute();
        REQUIRE(abs.hasValue());
        CHECK(!abs->empty());
    }

    SECTION("c_str() returns non-null native string")
    {
        const za::Path              p("hello.txt");
        const za::Path::value_type* ptr = p.c_str();
        REQUIRE(ptr != nullptr);
        const bool firstNonNull = ptr[0] != za::Path::value_type{0};
        CHECK(firstNonNull);
    }

    SECTION("to<T>() conversions")
    {
        const za::Path p("hello.txt");
        CHECK(p.to<std::string>() == "hello.txt");
        CHECK(p.to<std::wstring>() == std::wstring(L"hello.txt"));
        CHECK(p.to<std::u8string>() == std::u8string(u8"hello.txt"));
        CHECK(p.to<std::u32string>() == std::u32string(U"hello.txt"));
        // Round-trip through std::filesystem::path and back via za::Path
        CHECK(za::Path(p.to<std::filesystem::path>()) == p);
        CHECK(p.to<za::String>() == za::String("hello.txt"));
    }

    SECTION("empty()")
    {
        CHECK(za::Path().empty());
        CHECK(za::Path("").empty());
        CHECK(!za::Path("a").empty());
    }

    SECTION("exists() and removeFromDisk() with a real file")
    {
        const TemporaryFile tmp("payload");
        CHECK(tmp.getPath().exists());
        CHECK(tmp.getPath().removeFromDisk());
        CHECK(!tmp.getPath().exists());
    }

    SECTION("exists() returns false for non-existent path")
    {
        CHECK(!za::Path("this/really/should/not/exist/0xDEADBEEF.tmp").exists());
    }

    SECTION("removeFromDisk() returns false for non-existent path")
    {
        CHECK(!za::Path("this/really/should/not/exist/0xDEADBEEF.tmp").removeFromDisk());
    }

    SECTION("extensionIs() matches ASCII extensions")
    {
        CHECK(za::Path("hello.png").extensionIs(".png"));
        CHECK(za::Path("hello.wav").extensionIs(".wav"));
        CHECK(za::Path("/path/to/hello.wav").extensionIs(".wav"));
        CHECK(!za::Path("hello.png").extensionIs(".jpg"));
    }

    SECTION("extensionIs() is case-insensitive")
    {
        CHECK(za::Path("hello.PNG").extensionIs(".png"));
        CHECK(za::Path("hello.Png").extensionIs(".pNg"));
    }

    SECTION("extensionIs() returns false when no extension is present")
    {
        CHECK(!za::Path("hello").extensionIs(".png"));
        CHECK(!za::Path("").extensionIs(".png"));
    }

    SECTION("extensionIs() honors std::filesystem extension semantics")
    {
        // Leading dot of the filename is part of the stem, not the extension.
        CHECK(!za::Path(".hidden").extensionIs(".hidden"));
        CHECK(!za::Path("/foo/.hidden").extensionIs(".hidden"));
        CHECK(!za::Path(".bashrc").extensionIs(".bashrc"));

        // `.` and `..` filenames have no extension.
        CHECK(!za::Path(".").extensionIs("."));
        CHECK(!za::Path("..").extensionIs(".."));
        CHECK(!za::Path("/foo/.").extensionIs("."));
        CHECK(!za::Path("/foo/..").extensionIs(".."));

        // Multiple dots: the rightmost wins.
        CHECK(za::Path("hello.tar.gz").extensionIs(".gz"));
        CHECK(!za::Path("hello.tar.gz").extensionIs(".tar"));
        CHECK(!za::Path("hello.tar.gz").extensionIs(".tar.gz"));
        CHECK(za::Path("/path/with.dots/file.png").extensionIs(".png"));

        // Filename starting with a dot but containing another dot has the right-side extension.
        CHECK(za::Path(".foo.bar").extensionIs(".bar"));

        // Empty extension query: matches iff the path has no extension.
        CHECK(!za::Path("hello.png").extensionIs(""));
        CHECK(za::Path("hello").extensionIs(""));
        CHECK(za::Path(".hidden").extensionIs(""));
        CHECK(za::Path("/foo/").extensionIs(""));
        CHECK(za::Path("").extensionIs(""));
    }

    SECTION("operator/= appends path components")
    {
        za::Path p("foo");
        p /= za::Path("bar.txt");
        CHECK(p.getFilename() == za::Path("bar.txt"));
        CHECK(p.getParent() == za::Path("foo"));
    }

    SECTION("operator/ composes a new path")
    {
        const za::Path joined = za::Path("foo") / za::Path("bar.txt");
        CHECK(joined.getFilename() == za::Path("bar.txt"));
        CHECK(joined.getParent() == za::Path("foo"));
    }

    SECTION("operator== / operator!= on za::Path")
    {
        CHECK(za::Path("a.txt") == za::Path("a.txt"));
        CHECK(za::Path("a.txt") != za::Path("b.txt"));
    }

    SECTION("operator== / operator!= against const char*")
    {
        CHECK(za::Path("a.txt") == "a.txt");
        CHECK(za::Path("a.txt") != "b.txt");
    }

    SECTION("operator== / operator!= against const wchar_t*")
    {
        // Extra parens force the expression to evaluate to `bool` before the framework.s
        // expression decomposer runs, avoiding stringification of `wchar_t[N]`.
        CHECK((za::Path(L"a.txt") == L"a.txt"));
        CHECK((za::Path(L"a.txt") != L"b.txt"));
    }

    SECTION("operator== / operator!= against std::string")
    {
        const std::string rhs = "a.txt";
        CHECK(za::Path("a.txt") == rhs);
        CHECK(za::Path("b.txt") != rhs);
    }

    SECTION("Path formats via fmtToString(\"{}\", path)")
    {
        CHECK(za::fmtToString("{}", za::Path("hello.txt")) == za::String("hello.txt"));
    }

    SECTION("tempDirectoryPath() returns an existing directory")
    {
        const auto tmp = za::Path::getTempDirectory();
        REQUIRE(tmp.hasValue());
        CHECK(!tmp->empty());
        CHECK(tmp->exists());
    }

    SECTION("absolute() returns Optional that is convertible to bool")
    {
        const auto abs = za::Path("hello.txt").getAbsolute();
        CHECK(static_cast<bool>(abs));
    }

    SECTION("stem()")
    {
        CHECK(za::Path("/foo/bar.txt").getStem() == za::Path("bar"));
        CHECK(za::Path("/foo/bar.tar.gz").getStem() == za::Path("bar.tar"));
        CHECK(za::Path("noext").getStem() == za::Path("noext"));
        CHECK(za::Path(".hidden").getStem() == za::Path(".hidden"));
        CHECK(za::Path("").getStem().empty());
    }

    SECTION("isDirectory(), isRegularFile(), isSymlink() on temp file")
    {
        const TemporaryFile tmp("payload");
        CHECK(tmp.getPath().isRegularFile());
        CHECK(!tmp.getPath().isDirectory());
        CHECK(!tmp.getPath().isSymlink());

        const auto tempDir = za::Path::getTempDirectory();
        REQUIRE(tempDir.hasValue());
        CHECK(tempDir->isDirectory());
        CHECK(!tempDir->isRegularFile());
    }

    SECTION("isDirectory(), isRegularFile() return false for missing path")
    {
        const za::Path missing("this/really/should/not/exist/0xDEADBEEF.tmp");
        CHECK(!missing.isDirectory());
        CHECK(!missing.isRegularFile());
        CHECK(!missing.isSymlink());
    }

    SECTION("fileSize() returns the byte length of the file")
    {
        const TemporaryFile tmp("0123456789");
        const auto          sz = tmp.getPath().getFileSize();
        REQUIRE(sz.hasValue());
        CHECK(*sz == 10u);
    }

    SECTION("fileSize() fails for missing file")
    {
        CHECK(!za::Path("this/really/does/not/exist.tmp").getFileSize().hasValue());
    }

    SECTION("lastWriteTimeSecondsSinceEpoch() returns a sensible value")
    {
        const TemporaryFile tmp("payload");
        const auto          t = tmp.getPath().getLastWriteTimeSecondsSinceEpoch();
        REQUIRE(t.hasValue());
        // Sanity: should be after 2020-01-01 (1577836800) and before 2200-01-01 (7258118400).
        CHECK(*t > 1'577'836'800);
        CHECK(*t < 7'258'118'400);
    }

    SECTION("currentWorkingDirectory() returns an existing directory")
    {
        const auto cwd = za::Path::getCurrentDirectory();
        REQUIRE(cwd.hasValue());
        CHECK(cwd->isDirectory());
    }

    SECTION("setCurrentWorkingDirectory() round-trips through getter")
    {
        const auto originalMaybe = za::Path::getCurrentDirectory();
        REQUIRE(originalMaybe.hasValue());
        const za::Path& original = *originalMaybe;

        const auto tempMaybe = za::Path::getTempDirectory();
        REQUIRE(tempMaybe.hasValue());

        REQUIRE(za::Path::setCurrentDirectory(*tempMaybe));
        const auto afterChange = za::Path::getCurrentDirectory();
        REQUIRE(afterChange.hasValue());
        CHECK(afterChange->isDirectory());

        // Restore so other tests aren't affected.
        REQUIRE(za::Path::setCurrentDirectory(original));
    }

    SECTION("homeDirectory() returns a path on platforms with HOME / USERPROFILE set")
    {
        const auto home = za::Path::getHomeDirectory();
        // We don't strictly require the env var to be set, but if it is, the
        // returned path shouldn't be empty.
        if (home.hasValue())
            CHECK(!home->empty());
    }

    SECTION("operator+= concatenates without inserting a separator")
    {
        za::Path p("foo");
        p += za::Path(".bak");
        CHECK(p == za::Path("foo.bak"));
    }

    SECTION("operator+ concatenates without inserting a separator")
    {
        const za::Path joined = za::Path("file") + za::Path(".tmp");
        CHECK(joined == za::Path("file.tmp"));
    }

    SECTION("renameTo() moves an existing file")
    {
        const TemporaryFile src("payload");
        const auto          dstParent = za::Path::getTempDirectory();
        REQUIRE(dstParent.hasValue());

        // Disambiguate temp-file names across parallel test processes
        const za::Path dst = *dstParent /
                             za::Path(za::fmtToString("sf_rename_target_{}.tmp", za::testing::getProcessUniqueId()));

        // Best-effort cleanup in case a previous run left it behind.
        (void)dst.removeFromDisk();

        REQUIRE(src.getPath().renameTo(dst));
        CHECK(!src.getPath().exists());
        CHECK(dst.exists());

        CHECK(dst.removeFromDisk());
    }

    SECTION("renameTo() fails for a missing source")
    {
        const za::Path missing("this/really/does/not/exist.tmp");
        const za::Path dst("sf_rename_unused_target.tmp");
        CHECK(!missing.renameTo(dst));
    }

    SECTION("forEachEntry() iterates over directory contents")
    {
        const auto tempDir = za::Path::getTempDirectory();
        REQUIRE(tempDir.hasValue());

        // Disambiguate temp-file names across parallel test processes
        const za::Path subdir = *tempDir /
                                za::Path(za::fmtToString("sf_path_iter_test_dir_{}", za::testing::getProcessUniqueId()));

        (void)subdir.removeFromDisk(); // best-effort cleanup
        REQUIRE(subdir.createDirectoryTree());

        const za::Path fileA = subdir / za::Path("a.txt");
        const za::Path fileB = subdir / za::Path("b.txt");
        {
            const TemporaryFile srcA("aaa");
            const TemporaryFile srcB("bbb");
            REQUIRE(srcA.getPath().copyFileTo(fileA));
            REQUIRE(srcB.getPath().copyFileTo(fileB));
        }

        int        count = 0;
        bool       sawA  = false;
        bool       sawB  = false;
        const bool ok    = subdir.forEachEntry([&](const za::Path& entry)
        {
            ++count;
            if (entry.getFilename() == za::Path("a.txt"))
                sawA = true;
            if (entry.getFilename() == za::Path("b.txt"))
                sawB = true;
        });

        CHECK(ok);
        CHECK(count == 2);
        CHECK(sawA);
        CHECK(sawB);

        // Cleanup.
        CHECK(fileA.removeFromDisk());
        CHECK(fileB.removeFromDisk());
        CHECK(subdir.removeFromDisk());
    }

    SECTION("forEachEntry() returns false for a non-existent path")
    {
        const bool ok = za::Path("this/really/does/not/exist").forEachEntry([](const za::Path&) {});
        CHECK(!ok);
    }

    // ------------------------------------------------------------------
    // Unicode / UTF-8 round-trip (previously threw under MinGW/Clang64)
    // ------------------------------------------------------------------

    SECTION("to<std::string>() with non-ASCII paths does not throw")
    {
        CHECK_NOTHROW((void)za::Path(U"hello-ñ.txt").to<std::string>());
        CHECK_NOTHROW((void)za::Path(U"hello-ń.txt").to<std::string>());
        CHECK_NOTHROW((void)za::Path(U"hello-日.txt").to<std::string>());
        CHECK_NOTHROW((void)za::Path(U"hello-🐌.txt").to<std::string>());
    }

    SECTION("to<std::string>() produces UTF-8 for non-ASCII")
    {
        // U+00F1 (ñ) encodes to the UTF-8 bytes 0xC3 0xB1.
        const auto s = za::Path(U"\u00F1").to<std::string>();
        REQUIRE(s.size() == 2);
        CHECK(static_cast<unsigned char>(s[0]) == 0xC3);
        CHECK(static_cast<unsigned char>(s[1]) == 0xB1);
    }

    SECTION("to<za::String>() with non-ASCII paths does not throw")
    {
        CHECK_NOTHROW((void)za::Path(U"hello-ñ.txt").to<za::String>());
        CHECK_NOTHROW((void)za::Path(U"hello-日.txt").to<za::String>());
        CHECK_NOTHROW((void)za::Path(U"hello-🐌.txt").to<za::String>());
    }

    SECTION("to<std::u32string>() round-trips non-ASCII exactly")
    {
        CHECK(za::Path(U"hello-🐌.txt").to<std::u32string>() == U"hello-🐌.txt");
        CHECK(za::Path(U"hello-日.txt").to<std::u32string>() == U"hello-日.txt");
    }

    SECTION("Path formatting with non-ASCII does not throw")
    {
        // If formatting threw, the test runner catches it and fails the test -- no explicit guard needed.
        CHECK(!za::fmtToString("{}", za::Path(U"hello-🐌.txt")).empty());
    }

    SECTION("extensionIs() does not throw on non-ASCII paths")
    {
        CHECK_NOTHROW((void)za::Path(U"hello-🐌").extensionIs(".png"));
        CHECK_NOTHROW((void)za::Path(U"hello-ñ").extensionIs(".png"));
        CHECK(!za::Path(U"hello-🐌").extensionIs(".png"));
        CHECK(za::Path(U"hello-🐌.png").extensionIs(".png"));
    }
}
