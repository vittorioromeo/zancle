#include "StringifyStringViewUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/IO/MemoryInputStream.hpp"

#include "Zancle/String/StringView.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"

TEST_CASE("[System] za::MemoryInputStream")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::MemoryInputStream));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::MemoryInputStream));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::MemoryInputStream));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::MemoryInputStream));
    }

    using namespace za::literals;

    SECTION("open()")
    {
        static constexpr auto input = "hello world"_sv;

        SECTION("Zero length")
        {
            za::MemoryInputStream memoryInputStream(input.data(), 0);
            CHECK(memoryInputStream.tell().value() == 0);
            CHECK(memoryInputStream.getSize().value() == 0);
        }

        SECTION("Full length")
        {
            za::MemoryInputStream memoryInputStream(input.data(), input.size());
            CHECK(memoryInputStream.tell().value() == 0);
            CHECK(memoryInputStream.getSize().value() == input.size());
        }
    }

    SECTION("read()")
    {
        static constexpr auto input = "hello world"_sv;
        za::MemoryInputStream memoryInputStream(input.data(), input.size());
        CHECK(memoryInputStream.tell().value() == 0);
        CHECK(memoryInputStream.getSize().value() == input.size());

        // Read within input
        char output[32]{};
        CHECK(memoryInputStream.read(output, 5).value() == 5);
        CHECK(za::StringView(output, 5) == "hello"_sv);
        CHECK(memoryInputStream.tell().value() == 5);
        CHECK(memoryInputStream.getSize().value() == input.size());

        // Read beyond input
        CHECK(memoryInputStream.read(output, 100).value() == 6);
        CHECK(za::StringView(output, 6) == " world"_sv);
        CHECK(memoryInputStream.tell().value() == 11);
        CHECK(memoryInputStream.getSize().value() == input.size());
    }

    SECTION("seek()")
    {
        static constexpr auto input = "We Love Zancle!"_sv;
        za::MemoryInputStream memoryInputStream(input.data(), input.size());
        CHECK(memoryInputStream.tell().value() == 0);
        CHECK(memoryInputStream.getSize().value() == input.size());

        SECTION("Seek within input")
        {
            CHECK(memoryInputStream.seek(0).value() == 0);
            CHECK(memoryInputStream.tell().value() == 0);

            CHECK(memoryInputStream.seek(5).value() == 5);
            CHECK(memoryInputStream.tell().value() == 5);
        }

        SECTION("Seek beyond input")
        {
            CHECK(memoryInputStream.seek(1000).value() == input.size());
            CHECK(memoryInputStream.tell().value() == input.size());
        }
    }
}


TEST_CASE("[System] za::MemoryInputStream over no data")
{
    // E.g. an empty vector's `data()`
    za::MemoryInputStream memoryInputStream(nullptr, 0u);

    char output[4];
    CHECK(memoryInputStream.read(output, 4u).value() == 0u);
    CHECK(memoryInputStream.seek(10u).value() == 0u);
    CHECK(memoryInputStream.tell().value() == 0u);
    CHECK(memoryInputStream.getSize().value() == 0u);
}
