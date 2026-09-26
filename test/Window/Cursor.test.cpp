#include "StringifyOptionalUtil.hpp"
#include "Tst/Tst.hpp"
#include "WindowUtil.hpp"

#include "Zancle/Window/Cursor.hpp"

#include "Zancle/Window/WindowContext.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Base/IntTypes.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsDefaultConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"

TEST_CASE("[Window] za::Cursor" * tst::skip(skipDisplayTests))
{
    [[maybe_unused]] auto& windowContext = TST_CASE_SHARED(za::WindowContext::create().value());

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_DEFAULT_CONSTRUCTIBLE(za::Cursor));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::Cursor));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::Cursor));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::Cursor));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::Cursor));
    }

    SECTION("loadFromPixels()")
    {
        static constexpr za::U8 pixels[4]{};

        CHECK(!za::Cursor::loadFromPixels(nullptr, {}, {}).hasValue());
        CHECK(!za::Cursor::loadFromPixels(pixels, {0, 1}, {}).hasValue());
        CHECK(!za::Cursor::loadFromPixels(pixels, {1, 0}, {}).hasValue());
        CHECK(za::Cursor::loadFromPixels(pixels, {1, 1}, {}).hasValue());
    }

    SECTION("loadFromSystem()")
    {
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::Hand).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::SizeHorizontal).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::SizeVertical).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::SizeLeft).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::SizeRight).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::SizeTop).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::SizeBottom).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::SizeTopLeft).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::SizeTopRight).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::SizeBottomLeft).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::SizeBottomRight).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::Cross).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::Help).hasValue());
        CHECK(za::Cursor::loadFromSystem(za::Cursor::Type::NotAllowed).hasValue());
    }
}
