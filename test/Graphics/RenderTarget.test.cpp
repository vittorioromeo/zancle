#include "GraphicsUtil.hpp"
#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Graphics/RenderTarget.hpp"

#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/IsConstructible.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


namespace
{

class TestRenderTarget : public za::RenderTarget
{
public:
    TestRenderTarget() = default;

private:
    [[nodiscard]] za::Vec2u getSize() const override
    {
        return {640, 480};
    }
};

TEST_CASE("[Graphics] za::RenderTarget")
{
    [[maybe_unused]] auto& graphicsContext = TST_CASE_SHARED(za::GraphicsContext::create().value());

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::RenderTarget));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::RenderTarget));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::RenderTarget));
        STATIC_CHECK(!ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::RenderTarget));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::RenderTarget));
    }

    SECTION("Construction")
    {
        const TestRenderTarget renderTarget;

        const auto defaultView = za::View::fromScreenSize({640.f, 480.f});

        CHECK(defaultView.center == za::Vec2f{320, 240});
        CHECK(defaultView.size == za::Vec2f{640, 480});
        CHECK(defaultView.rotation == za::Angle::Zero);
        CHECK(defaultView.viewport == za::Rect2f({0, 0}, {1, 1}));
        CHECK(!renderTarget.isSrgb());
    }

    SECTION("Move assignment")
    {
        TestRenderTarget renderTarget0;
        TestRenderTarget renderTarget1;

        renderTarget1 = ZA_MOVE(renderTarget0);
    }

    SECTION("setActive()")
    {
        TestRenderTarget renderTarget;
        CHECK(renderTarget.setActive());
        CHECK(renderTarget.setActive(false));
        CHECK(renderTarget.setActive(true));
    }
}

} // namespace
