#include "Zancle/Network/Packet.hpp"

// Other 1st party headers
#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"
#include "Zancle/Trait/RemoveConst.hpp"

#include <limits>
#include <string>

#include <cstddef>


#define CHECK_PACKET_STREAM_OPERATORS(expected)              \
    do                                                       \
    {                                                        \
        za::Packet packet;                                   \
        packet << (expected);                                \
        CHECK(packet.getReadPosition() == 0);                \
        CHECK(packet.getData() != nullptr);                  \
        CHECK(packet.getDataSize() == sizeof(expected));     \
        CHECK(!packet.endOfPacket());                        \
        CHECK(bool{packet});                                 \
                                                             \
        decltype(expected) received;                         \
        packet >> received;                                  \
        CHECK(packet.getReadPosition() == sizeof(expected)); \
        CHECK(packet.getData() != nullptr);                  \
        CHECK(packet.getDataSize() == sizeof(expected));     \
        CHECK(packet.endOfPacket());                         \
        CHECK(bool{packet});                                 \
        CHECK((expected) == received);                       \
    } while (false)

#define CHECK_PACKET_NARROW_STRING_STREAM_OPERATORS(expected, size) \
    do                                                              \
    {                                                               \
        za::Packet packet;                                          \
        packet << (expected);                                       \
        CHECK(packet.getReadPosition() == 0);                       \
        CHECK(packet.getData() != nullptr);                         \
        CHECK(packet.getDataSize() == (size));                      \
        CHECK(!packet.endOfPacket());                               \
        CHECK(bool{packet});                                        \
                                                                    \
        ZA_REMOVE_CONST(decltype(expected)) received;               \
        packet >> received;                                         \
        CHECK(packet.getReadPosition() == (size));                  \
        CHECK(packet.getData() != nullptr);                         \
        CHECK(packet.getDataSize() == (size));                      \
        CHECK(packet.endOfPacket());                                \
        CHECK(bool{packet});                                        \
        CHECK(za::String{expected} == za::String{received});        \
    } while (false)

#define CHECK_PACKET_WIDE_STRING_STREAM_OPERATORS(expected, size)                               \
    do                                                                                          \
    {                                                                                           \
        za::Packet packet;                                                                      \
        packet << (expected);                                                                   \
        CHECK(packet.getReadPosition() == 0);                                                   \
        CHECK(packet.getData() != nullptr);                                                     \
        CHECK(packet.getDataSize() == (size));                                                  \
        CHECK(!packet.endOfPacket());                                                           \
        CHECK(bool{packet});                                                                    \
                                                                                                \
        ZA_REMOVE_CONST(decltype(expected)) received;                                           \
        packet >> received;                                                                     \
        CHECK(packet.getReadPosition() == (size));                                              \
        CHECK(packet.getData() != nullptr);                                                     \
        CHECK(packet.getDataSize() == (size));                                                  \
        CHECK(packet.endOfPacket());                                                            \
        CHECK(bool{packet});                                                                    \
        /* `std::wstring` is not stringifiable by the test framework, so compare via `bool`. */ \
        const bool roundTripEqual = std::wstring{expected} == std::wstring{received};           \
        CHECK(roundTripEqual);                                                                  \
    } while (false)

struct Packet : za::Packet
{
    using za::Packet::onReceive;
    using za::Packet::onSend;
};

TEST_CASE("[Network] za::Packet")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::Packet));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::Packet));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::Packet));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::Packet));
    }

    SECTION("Default constructor")
    {
        const za::Packet packet;
        CHECK(packet.getReadPosition() == 0);
        CHECK(packet.getData() == nullptr);
        CHECK(packet.getDataSize() == 0);
        CHECK(packet.endOfPacket());
        CHECK(bool{packet});
    }

    static constexpr int data[]{1, 2, 3, 4, 5, 6};

    SECTION("Append and clear")
    {
        za::Packet packet;
        packet.append(data, 6);
        CHECK(packet.getReadPosition() == 0);
        CHECK(packet.getData() != nullptr);
        CHECK(packet.getDataSize() == 6);
        CHECK(!packet.endOfPacket());
        CHECK(bool{packet});

        packet.clear();
        CHECK(packet.getReadPosition() == 0);
        CHECK(packet.getData() == nullptr);
        CHECK(packet.getDataSize() == 0);
        CHECK(packet.endOfPacket());
        CHECK(bool{packet});
    }

    SECTION("Wire byte order (little-endian, host order)")
    {
        za::Packet packet;

        SECTION("16 bit int")
        {
            packet << za::U16{12'345};
            const auto*                 dataPtr = static_cast<const std::byte*>(packet.getData());
            const za::Vector<std::byte> bytes(dataPtr, dataPtr + packet.getDataSize());
            const za::Vector<std::byte> expectedBytes{std::byte{0x39}, std::byte{0x30}};
            CHECK((bytes == expectedBytes));
        }

        SECTION("32 bit int")
        {
            packet << za::U32{1'234'567'890};
            const auto*                 dataPtr = static_cast<const std::byte*>(packet.getData());
            const za::Vector<std::byte> bytes(dataPtr, dataPtr + packet.getDataSize());
            const za::Vector<std::byte> expectedBytes{std::byte{0xD2}, std::byte{0x02}, std::byte{0x96}, std::byte{0x49}};
            CHECK((bytes == expectedBytes));
        }

        SECTION("float")
        {
            packet << 123.456f;
            const auto*                 dataPtr = static_cast<const std::byte*>(packet.getData());
            const za::Vector<std::byte> bytes(dataPtr, dataPtr + packet.getDataSize());
            const za::Vector<std::byte> expectedBytes{std::byte{0x79}, std::byte{0xe9}, std::byte{0xf6}, std::byte{0x42}};
            CHECK((bytes == expectedBytes));
        }

        SECTION("double")
        {
            packet << 789.123;
            const auto*                 dataPtr = static_cast<const std::byte*>(packet.getData());
            const za::Vector<std::byte> bytes(dataPtr, dataPtr + packet.getDataSize());
            const za::Vector<std::byte> expectedBytes{std::byte{0x44},
                                                      std::byte{0x8b},
                                                      std::byte{0x6c},
                                                      std::byte{0xe7},
                                                      std::byte{0xfb},
                                                      std::byte{0xa8},
                                                      std::byte{0x88},
                                                      std::byte{0x40}};
            CHECK((bytes == expectedBytes));
        }
    }

    SECTION("Stream operators")
    {
        SECTION("bool")
        {
            CHECK_PACKET_STREAM_OPERATORS(true);
            CHECK_PACKET_STREAM_OPERATORS(false);
        }

        SECTION("za::I8")
        {
            CHECK_PACKET_STREAM_OPERATORS(za::I8(0));
            CHECK_PACKET_STREAM_OPERATORS(za::I8(1));
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::I8>::min());
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::I8>::max());
        }

        SECTION("za::U8")
        {
            CHECK_PACKET_STREAM_OPERATORS(za::U8(0));
            CHECK_PACKET_STREAM_OPERATORS(za::U8(1));
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::U8>::min());
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::U8>::max());
        }

        SECTION("za::I16")
        {
            CHECK_PACKET_STREAM_OPERATORS(za::I16(0));
            CHECK_PACKET_STREAM_OPERATORS(za::I16(1));
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::I16>::min());
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::I16>::max());
        }

        SECTION("za::U16")
        {
            CHECK_PACKET_STREAM_OPERATORS(za::U16(0));
            CHECK_PACKET_STREAM_OPERATORS(za::U16(1));
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::U16>::min());
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::U16>::max());
        }

        SECTION("za::I32")
        {
            CHECK_PACKET_STREAM_OPERATORS(za::I32(0));
            CHECK_PACKET_STREAM_OPERATORS(za::I32(1));
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::I32>::min());
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::I32>::max());
        }

        SECTION("za::U32")
        {
            CHECK_PACKET_STREAM_OPERATORS(za::U32(0));
            CHECK_PACKET_STREAM_OPERATORS(za::U32(1));
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::U32>::min());
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::U32>::max());
        }

        SECTION("za::I64")
        {
            CHECK_PACKET_STREAM_OPERATORS(za::I64(0));
            CHECK_PACKET_STREAM_OPERATORS(za::I64(1));
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::I64>::min());
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::I64>::max());
        }

        SECTION("za::U64")
        {
            CHECK_PACKET_STREAM_OPERATORS(za::U64(0));
            CHECK_PACKET_STREAM_OPERATORS(za::U64(1));
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::U64>::min());
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<za::U64>::max());
        }

        SECTION("float")
        {
            CHECK_PACKET_STREAM_OPERATORS(0.f);
            CHECK_PACKET_STREAM_OPERATORS(1.f);
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<float>::min());
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<float>::max());
        }

        SECTION("double")
        {
            CHECK_PACKET_STREAM_OPERATORS(0.);
            CHECK_PACKET_STREAM_OPERATORS(1.);
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<double>::min());
            CHECK_PACKET_STREAM_OPERATORS(std::numeric_limits<double>::max());
        }

        SECTION("std::string")
        {
            const std::string string = "testing";
            CHECK_PACKET_NARROW_STRING_STREAM_OPERATORS(string, string.size() + 4);
        }

        SECTION("za::String")
        {
            const za::String string = "testing";
            CHECK_PACKET_NARROW_STRING_STREAM_OPERATORS(string, string.size() + 4);
        }

        SECTION("std::wstring")
        {
            const std::wstring string = L"testing";
            CHECK_PACKET_WIDE_STRING_STREAM_OPERATORS(string, 4 * string.size() + 4);
        }

        SECTION("za::Utf8String")
        {
            const za::Utf8String string = "testing";
            // New wire format: 4-byte length + UTF-8 byte payload.
            CHECK_PACKET_NARROW_STRING_STREAM_OPERATORS(string, string.byteSize() + 4);
        }
    }

    SECTION("onSend")
    {
        Packet    packet;
        za::SizeT size = 0;
        CHECK(packet.onSend(size) == nullptr);
        CHECK(size == 0);

        packet.append(data, 6);
        CHECK(packet.onSend(size) != nullptr);
        CHECK(size == 6);
    }

    SECTION("onReceive")
    {
        Packet packet;
        packet.onReceive(data, 6);
        CHECK(packet.getReadPosition() == 0);
        CHECK(packet.getData() != nullptr);
        CHECK(packet.getDataSize() == 6);
    }

    SECTION("Attempt overflow")
    {
        static constexpr struct
        {
            za::U32 length{std::numeric_limits<decltype(length)>::max()};
            char    data[4]{'S', 'F', 'M', 'L'};
        } string;

        za::Packet packet;
        packet.append(&string, sizeof(string));

        std::string out;
        packet >> out; // Ensure this does not trigger a crash
        CHECK(out.empty());
    }
}
