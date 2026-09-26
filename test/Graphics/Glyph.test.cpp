#include "GraphicsUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Graphics/Glyph.hpp"

#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Trait/IsTriviallyCopyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyConstructible.hpp"
#include "Zancle/Trait/IsTriviallyMoveAssignable.hpp"
#include "Zancle/Trait/IsTriviallyMoveConstructible.hpp"


TEST_CASE("[Graphics] za::Glyph")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Glyph));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Glyph));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Glyph));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Glyph));
    }

    SECTION("Construction")
    {
        constexpr za::Glyph glyph{};
        STATIC_CHECK(glyph.advance == 0.f);
        STATIC_CHECK(glyph.bounds == za::Rect2f());
        STATIC_CHECK(glyph.textureRect == za::Rect2f());
    }
}
