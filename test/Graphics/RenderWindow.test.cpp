#include "SystemUtil.hpp"

#include "Zancle/Graphics/RenderWindow.hpp"

// Other 1st party headers
#include "GraphicsUtil.hpp"
#include "Tst/Tst.hpp"
#include "WindowUtil.hpp"

#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/Window/ContextSettings.hpp"
#include "Zancle/Window/WindowHandle.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/HasVirtualDestructor.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


TEST_CASE("[Graphics] za::RenderWindow" * tst::skip(skipDisplayTests))
{
    [[maybe_unused]] auto& graphicsContext = TST_CASE_SHARED(za::GraphicsContext::create().value());

    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_HAS_VIRTUAL_DESTRUCTOR(za::RenderWindow)); // because of RenderTarget, not WindowBase
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::RenderWindow));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::RenderWindow));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::RenderWindow));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::RenderWindow));
    }

    SECTION("Construction")
    {
        SECTION("Style, state, and settings")
        {
            const auto window = za::RenderWindow::create(
                                    {.size{256u, 256u}, .bitsPerPixel = 24, .title = "RenderWindow Tests"})
                                    .value();

            CHECK(window.getSize() == za::Vec2u{256, 256});
            // SDL3 provides a real native handle on Emscripten too, so we
            // can drop the Emscripten special case here.
            CHECK(window.getNativeHandle() != za::WindowHandle());
#ifndef ZA_SYSTEM_EMSCRIPTEN
            CHECK(window.getSettings().attributeFlags == za::ContextSettings{}.attributeFlags);
#endif
            CHECK(!window.isSrgb());

            const auto defaultView = za::View::fromScreenSize({256.f, 256.f});

            CHECK(defaultView.center == za::Vec2f{128, 128});
            CHECK(defaultView.size == za::Vec2f{256, 256});
            CHECK(defaultView.rotation == za::Angle::Zero);
            CHECK(defaultView.viewport == za::Rect2f({0, 0}, {1, 1}));
            CHECK(defaultView.getTransform() == Approx(za::Transform(0.0078125f, 0, -1, 0, -0.0078125f, 1)));
        }

        SECTION("State and settings")
        {
            const auto window = za::RenderWindow::create(
                                    {.size{240u, 300u}, .bitsPerPixel = 24, .title = "RenderWindow Tests"})
                                    .value();

            CHECK(window.getSize() == za::Vec2u{240, 300});
            // SDL3 provides a real native handle on Emscripten too, so we
            // can drop the Emscripten special case here.
            CHECK(window.getNativeHandle() != za::WindowHandle());
#ifndef ZA_SYSTEM_EMSCRIPTEN
            CHECK(window.getSettings().attributeFlags == za::ContextSettings{}.attributeFlags);
#endif
            CHECK(!window.isSrgb());

            const auto defaultView = za::View::fromScreenSize({240.f, 300.f});

            CHECK(defaultView.center == za::Vec2f{120, 150});
            CHECK(defaultView.size == za::Vec2f{240, 300});
            CHECK(defaultView.rotation == za::Angle::Zero);
            CHECK(defaultView.viewport == za::Rect2f({0, 0}, {1, 1}));
            CHECK(defaultView.getTransform() == Approx(za::Transform(0.00833333f, 0, -1, 0, -0.00666667f, 1)));
        }
    }

// `Texture::update(RenderWindow&)` reads back from the canvas's default
// framebuffer (FBO 0). On Emscripten/WebGL this is unreliable: SDL3 creates
// the WebGL2 context with the default `preserveDrawingBuffer = false`, so per
// the WebGL spec the canvas drawing buffer is treated as write-only -- after
// `glClear`, `glReadPixels`/`glBlitFramebuffer` from FBO 0 returns undefined
// data (zeros in Chrome). To read back rendered output on Emscripten, draw to
// a `RenderTexture` instead. Skipping this section there.
#ifndef ZA_SYSTEM_EMSCRIPTEN
    SECTION("Clear")
    {
        auto window = za::RenderWindow::create({.size{256u, 256u}, .title = "RenderWindow Tests"}).value();

        REQUIRE(window.getSize() == za::Vec2u{256, 256});

        auto texture = za::Texture::create(za::Vec2u{256, 256}).value();

        window.clear(za::Color::Red);
        CHECK(texture.update(window));
        CHECK(texture.copyToImage().getPixel(za::Vec2u{64, 64}) == za::Color::Red);

        window.clear(za::Color::Green);
        CHECK(texture.update(window));
        CHECK(texture.copyToImage().getPixel(za::Vec2u{128, 128}) == za::Color::Green);

        window.clear(za::Color::Blue);
        CHECK(texture.update(window));
        CHECK(texture.copyToImage().getPixel(za::Vec2u{196, 196}) == za::Color::Blue);
    }
#endif

// Creating multiple windows in Emscripten is not supported
#ifndef ZA_SYSTEM_EMSCRIPTEN
    SECTION("Move assignment")
    {
        auto window0 = za::RenderWindow::create({.size{128u, 128u}, .title = "A"}).value();
        auto window1 = za::RenderWindow::create({.size{256u, 256u}, .title = "B"}).value();

        window1 = ZA_MOVE(window0);
        CHECK(window1.getSize() == za::Vec2u{128u, 128u});
    }

    SECTION("Multiple windows clear/display orderings")
    {
        auto window      = za::RenderWindow::create({.size{256u, 256u}, .title = "A"}).value();
        auto childWindow = za::RenderWindow::create(za::RenderWindow::Settings{.size{256u, 256u}, .title = "B"});

        window.clear();
        window.display();

        childWindow->clear();
        childWindow->display();

        window.clear();
        window.display();

        childWindow->clear();
        childWindow->display();

        childWindow.reset();

        window.clear();
        window.display();
    }
#endif
}
