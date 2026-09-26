#include "Tst/Tst.hpp"

#include "Zancle/Network/Socket.hpp"

#include "Zancle/Network/UdpSocket.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/HasVirtualDestructor.hpp"
#include "Zancle/Trait/IsConstructible.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


TEST_CASE("[Network] za::Socket")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::Socket));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::Socket));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::Socket));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::Socket));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::Socket));
        STATIC_CHECK(!ZA_HAS_VIRTUAL_DESTRUCTOR(za::Socket));
    }

    SECTION("Constants")
    {
        STATIC_CHECK(za::Socket::AnyPort == 0);
    }

    // `za::Socket` is abstract (protected ctor); use `za::UdpSocket` to exercise
    // the move/blocking behaviour inherited from the base.
    SECTION("Factory produces a valid socket")
    {
        auto socketOpt = za::UdpSocket::create(/* isBlocking */ true);
        REQUIRE(socketOpt.hasValue());

        CHECK(socketOpt->isBlocking());

        // Unbound: local port is 0 until `bind` is called.
        CHECK(socketOpt->getLocalPort() == 0);
    }

    SECTION("Set/get blocking")
    {
        auto socketOpt = za::UdpSocket::create(/* isBlocking */ true);
        REQUIRE(socketOpt.hasValue());

        socketOpt->setBlocking(false);
        CHECK(!socketOpt->isBlocking());
    }

    SECTION("Move semantics")
    {
        SECTION("Construction")
        {
            auto movedOpt = za::UdpSocket::create(/* isBlocking */ true);
            REQUIRE(movedOpt.hasValue());

            movedOpt->setBlocking(false);


            const za::UdpSocket socket(ZA_MOVE(*movedOpt));
            CHECK(!socket.isBlocking());
        }

        SECTION("Assignment")
        {
            auto movedOpt = za::UdpSocket::create(/* isBlocking */ true);
            REQUIRE(movedOpt.hasValue());

            movedOpt->setBlocking(false);

            auto targetOpt = za::UdpSocket::create(/* isBlocking */ true);
            REQUIRE(targetOpt.hasValue());

            *targetOpt = ZA_MOVE(*movedOpt);
            CHECK(!targetOpt->isBlocking());
        }
    }
}
