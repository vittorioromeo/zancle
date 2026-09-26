#include "StringifyIpAddressUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Network/UdpSocket.hpp"

#include "Zancle/Network/IpAddress.hpp"
#include "Zancle/Network/Socket.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/HasVirtualDestructor.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


TEST_CASE("[Network] za::UdpSocket")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::UdpSocket));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::UdpSocket));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::UdpSocket));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::UdpSocket));
        STATIC_CHECK(!ZA_HAS_VIRTUAL_DESTRUCTOR(za::UdpSocket));
    }

    SECTION("Constants")
    {
        STATIC_CHECK(za::UdpSocket::MaxDatagramSize == 65'507);
    }

    SECTION("Factory construction")
    {
        auto udpSocketOpt = za::UdpSocket::create(/* isBlocking */ true);
        REQUIRE(udpSocketOpt.hasValue());

        CHECK(udpSocketOpt->getLocalPort() == 0);
    }

    SECTION("bind()")
    {
        auto udpSocketOpt = za::UdpSocket::create(/* isBlocking */ true);
        REQUIRE(udpSocketOpt.hasValue());

        // Binding to the broadcast address is invalid.
        CHECK(udpSocketOpt->bind(za::Socket::AnyPort, za::IpAddress::Broadcast) == za::Socket::Status::Error);
        CHECK(udpSocketOpt->getLocalPort() == 0);

        // Binding to `AnyPort` picks an available port.
        CHECK(udpSocketOpt->bind(za::Socket::AnyPort) == za::Socket::Status::Done);
        CHECK(udpSocketOpt->getLocalPort() != 0);
    }

    SECTION("send() after bind() (regression: factory refactor)")
    {
        // Regression test: prior to the factory refactor, calling `send` on a
        // UdpSocket that had already been bound failed with `Status::Error`,
        // because `send` unconditionally re-entered the lazy `create()` path
        // which rejected already-initialized sockets. The factory-based
        // design eliminates the lazy-create step entirely.

        auto senderOpt   = za::UdpSocket::create(/* isBlocking */ true);
        auto receiverOpt = za::UdpSocket::create(/* isBlocking */ true);
        REQUIRE(senderOpt.hasValue());
        REQUIRE(receiverOpt.hasValue());

        // Bind both to ephemeral ports on the loopback address. Both must
        // succeed; `send` must still work afterwards on the bound sender.
        REQUIRE(senderOpt->bind(za::Socket::AnyPort, za::IpAddress::LocalHost) == za::Socket::Status::Done);
        REQUIRE(receiverOpt->bind(za::Socket::AnyPort, za::IpAddress::LocalHost) == za::Socket::Status::Done);

        const unsigned short receiverPort = receiverOpt->getLocalPort();
        REQUIRE(receiverPort != 0);

        // Make the receiver non-blocking so that any `receive` failure surfaces
        // quickly if the datagram never arrived.
        receiverOpt->setBlocking(false);

        const char payload[] = "ping";
        CHECK(senderOpt->send(payload, sizeof(payload), za::IpAddress::LocalHost, receiverPort) == za::Socket::Status::Done);

        // Poll a few times to ride out loopback scheduling jitter.
        char                        buffer[sizeof(payload)]{};
        za::SizeT                   received{};
        za::Optional<za::IpAddress> sender;
        unsigned short              senderPort{};

        za::Socket::Status receiveStatus = za::Socket::Status::NotReady;
        for (int attempt = 0; attempt < 100 && receiveStatus != za::Socket::Status::Done; ++attempt)
            receiveStatus = receiverOpt->receive(buffer, sizeof(buffer), received, sender, senderPort);

        REQUIRE(receiveStatus == za::Socket::Status::Done);
        CHECK(received == sizeof(payload));

        for (za::SizeT i = 0; i < sizeof(payload); ++i)
            CHECK(buffer[i] == payload[i]);

        REQUIRE(sender.hasValue());
        CHECK(*sender == za::IpAddress::LocalHost);
        CHECK(senderPort == senderOpt->getLocalPort());
    }
}
