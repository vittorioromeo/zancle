#include "GraphicsUtil.hpp"
#include "StringifyOptionalUtil.hpp"
#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"
#include "WindowUtil.hpp"

#include "Zancle/Graphics/RenderTexture.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DepthStencilFormat.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/TextureWrapMode.hpp"
#include "Zancle/Graphics/Vertex.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsDefaultConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


TEST_CASE("[Graphics] za::RenderTexture" * tst::skip(skipDisplayTests))
{
    [[maybe_unused]] auto& graphicsContext = TST_CASE_SHARED(za::GraphicsContext::create().value());

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_DEFAULT_CONSTRUCTIBLE(za::RenderTexture));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::RenderTexture));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::RenderTexture));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::RenderTexture));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::RenderTexture));
    }

    SECTION("create()")
    {
        CHECK(!za::RenderTexture::create({1'000'000, 1'000'000}).hasValue());

        CHECK(za::RenderTexture::create({100, 100}, {.depthStencilFormat = za::DepthStencilFormat::Depth16}).hasValue());
        CHECK(za::RenderTexture::create({100, 100}, {.depthStencilFormat = za::DepthStencilFormat::Depth24}).hasValue());

        CHECK(za::RenderTexture::create({100, 100}, {.depthStencilFormat = za::DepthStencilFormat::Stencil8}).hasValue());

        const auto renderTexture = za::RenderTexture::create({360, 480}).value();
        CHECK(renderTexture.getSize() == za::Vec2u{360, 480});
        CHECK(!renderTexture.isSmooth());
        CHECK(renderTexture.getWrapMode() == za::TextureWrapMode::Clamp);
        CHECK(!renderTexture.isSrgb());

        const auto& texture = renderTexture.getTexture();
        CHECK(texture.getSize() == za::Vec2u{360, 480});
        CHECK(!texture.isSmooth());
        CHECK(!texture.isSrgb());
        CHECK(texture.getWrapMode() == za::TextureWrapMode::Clamp);
        CHECK(texture.getNativeHandle() != 0);
    }

    SECTION("getMaximumSampleCount()")
    {
        CHECK(za::RenderTexture::getMaximumSampleCount() <= 64);
    }

    SECTION("Set/get smooth")
    {
        auto renderTexture = za::RenderTexture::create({64, 64}).value();
        renderTexture.setSmooth(true);
        CHECK(renderTexture.isSmooth());
    }

    SECTION("Set/get repeated")
    {
        auto renderTexture = za::RenderTexture::create({64, 64}).value();
        renderTexture.setWrapMode(za::TextureWrapMode::Repeat);
        CHECK(renderTexture.getWrapMode() == za::TextureWrapMode::Repeat);
    }

    SECTION("generateMipmap()")
    {
        auto renderTexture = za::RenderTexture::create({64, 64}).value();
        renderTexture.generateMipmap();
    }

    SECTION("setActive()")
    {
        auto renderTexture = za::RenderTexture::create({64, 64}).value();
        CHECK(renderTexture.setActive());
        CHECK(renderTexture.setActive(false));
        CHECK(renderTexture.setActive(true));
    }

    SECTION("getTexture()")
    {
        const auto renderTexture = za::RenderTexture::create({64, 64}).value();
        CHECK(renderTexture.getTexture().getSize() == za::Vec2u{64, 64});
    }

    SECTION("Sanity check 1")
    {
        unsigned int testAALevel{};
        bool         testSRGBCapable{};

        // clang-format off
        SUBCASE("no AA, no SRGB") { testAALevel = 0u; testSRGBCapable = false; }
        SUBCASE("AA, no SRGB")    { testAALevel = 4u; testSRGBCapable = false; }
        SUBCASE("no AA, SRGB")    { testAALevel = 0u; testSRGBCapable = true; }
        SUBCASE("AA, SRGB")       { testAALevel = 4u; testSRGBCapable = true; }
        // clang-format on

        auto renderTexture = za::RenderTexture::create({64, 64}, {.sRgbCapable = testSRGBCapable, .sampleCount = testAALevel})
                                 .value();

        renderTexture.clear(za::Color::Green);
        renderTexture.display();

        auto image = renderTexture.getTexture().copyToImage();
        CHECK((image.getPixel({0u, 0u}) == za::Color::Green));
    }

    SECTION("Sanity check 2")
    {
        unsigned int testAALevel{};
        bool         testSRGBCapable{};

        // clang-format off
        SUBCASE("no AA, no SRGB") { testAALevel = 0u; testSRGBCapable = false; }
        SUBCASE("AA, no SRGB")    { testAALevel = 4u; testSRGBCapable = false; }
        SUBCASE("no AA, SRGB")    { testAALevel = 0u; testSRGBCapable = true; }
        SUBCASE("AA, SRGB")       { testAALevel = 4u; testSRGBCapable = true; }
        // clang-format on

        const float width     = 128.f;
        const float height    = 64.f;
        const float halfWidth = width / 2.f;

        const za::Vec2u size{static_cast<unsigned int>(width), static_cast<unsigned int>(height)};

        auto image   = za::Image::create(size, za::Color::White).value();
        auto texture = za::Texture::loadFromImage(image).value();

        auto baseRenderTexture = za::RenderTexture::create(size, {.sRgbCapable = testSRGBCapable, .sampleCount = testAALevel})
                                     .value();

        auto leftInnerRT = za::RenderTexture::create(size, {.sRgbCapable = testSRGBCapable, .sampleCount = testAALevel}).value();
        auto rightInnerRT = za::RenderTexture::create(size, {.sRgbCapable = testSRGBCapable, .sampleCount = testAALevel})
                                .value();

        const za::Vertex leftVertexArray[6]{{{0.f, 0.f}, za::Color::White, {0.f, 0.f}},
                                            {{halfWidth, 0.f}, za::Color::White, {halfWidth, 0.f}},
                                            {{0.f, height}, za::Color::White, {0.f, height}},
                                            {{0.f, height}, za::Color::White, {0.f, height}},
                                            {{halfWidth, 0.f}, za::Color::White, {halfWidth, 0.f}},
                                            {{halfWidth, height}, za::Color::White, {halfWidth, height}}};

        const za::Vertex rightVertexArray[6]{{{halfWidth, 0.f}, za::Color::White, {halfWidth, 0.f}},
                                             {{width, 0.f}, za::Color::White, {width, 0.f}},
                                             {{halfWidth, height}, za::Color::White, {halfWidth, height}},
                                             {{halfWidth, height}, za::Color::White, {halfWidth, height}},
                                             {{width, 0.f}, za::Color::White, {width, 0.f}},
                                             {{width, height}, za::Color::White, {width, height}}};

        leftInnerRT.clear();
        rightInnerRT.clear();

        leftInnerRT.draw(texture);
        rightInnerRT.draw(texture, {.color = za::Color::Green});

        baseRenderTexture.clear();

        leftInnerRT.display();
        baseRenderTexture.draw(leftVertexArray, za::PrimitiveType::Triangles, {.texture = &leftInnerRT.getTexture()});

        rightInnerRT.display();
        baseRenderTexture.draw(rightVertexArray, za::PrimitiveType::Triangles, {.texture = &rightInnerRT.getTexture()});

        baseRenderTexture.display();

        auto finalImage = baseRenderTexture.getTexture().copyToImage();

        CHECK(finalImage.getSize() == size);

        CHECK((finalImage.getPixel({0u, 0u}) == za::Color::White));
        CHECK((finalImage.getPixel({static_cast<unsigned int>(width / 2.f) + 1u, 0u}) == za::Color::Green));
    }

    SECTION("Sanity check -- flipping")
    {
        unsigned int testAALevel{};
        bool         testSRGBCapable{};

        // clang-format off
        SUBCASE("no AA, no SRGB") { testAALevel = 0u; testSRGBCapable = false; }
        SUBCASE("AA, no SRGB")    { testAALevel = 4u; testSRGBCapable = false; }
        SUBCASE("no AA, SRGB")    { testAALevel = 0u; testSRGBCapable = true; }
        SUBCASE("AA, SRGB")       { testAALevel = 4u; testSRGBCapable = true; }
        // clang-format on

        const float width     = 128.f;
        const float height    = 64.f;
        const float halfWidth = width / 2.f;

        const za::Vec2u size{static_cast<unsigned int>(width), static_cast<unsigned int>(height)};

        auto image   = za::Image::create(size, za::Color::White).value();
        auto texture = za::Texture::loadFromImage(image).value();

        auto baseRenderTexture = za::RenderTexture::create(size, {.sRgbCapable = testSRGBCapable, .sampleCount = testAALevel})
                                     .value();

        auto leftInnerRT = za::RenderTexture::create(size, {.sRgbCapable = testSRGBCapable, .sampleCount = testAALevel}).value();

        const za::Vertex leftVertexArray[6]{{{0.f, 0.f}, za::Color::Red, {0.f, 0.f}},
                                            {{halfWidth, 0.f}, za::Color::Red, {halfWidth, 0.f}},
                                            {{0.f, height}, za::Color::Red, {0.f, height}},
                                            {{0.f, height}, za::Color::Green, {0.f, height}},
                                            {{halfWidth, 0.f}, za::Color::Green, {halfWidth, 0.f}},
                                            {{halfWidth, height}, za::Color::Green, {halfWidth, height}}};

        leftInnerRT.clear();
        leftInnerRT.draw(texture);

        baseRenderTexture.clear();

        leftInnerRT.display();
        baseRenderTexture.draw(leftVertexArray, za::PrimitiveType::Triangles, {.texture = &leftInnerRT.getTexture()});

        baseRenderTexture.display();

        auto finalImage = baseRenderTexture.getTexture().copyToImage();

        CHECK(finalImage.getSize() == size);
        CHECK(finalImage.getPixel({0u, 0u}) == za::Color::Red);

        CHECK(finalImage.getPixel({0u, 31u}) == za::Color::Red);
        CHECK(finalImage.getPixel({31u, 0u}) == za::Color::Red);
        CHECK(finalImage.getPixel({31u, 31u}) == za::Color::Red);

        CHECK(finalImage.getPixel({0u, 60u}) == za::Color::Red);
        CHECK(finalImage.getPixel({60u, 0u}) == za::Color::Red);

        CHECK(finalImage.getPixel({6u, 58u}) == za::Color::Green);
        CHECK(finalImage.getPixel({58u, 6u}) == za::Color::Green);
        CHECK(finalImage.getPixel({61u, 61u}) == za::Color::Green);
    }

    SECTION("Move assignment")
    {
        auto rt0 = za::RenderTexture::create({100, 100}, {.depthStencilFormat = za::DepthStencilFormat::Depth24Stencil8});
        CHECK(rt0.hasValue());

        auto rt1 = za::RenderTexture::create({100, 100}, {.depthStencilFormat = za::DepthStencilFormat::Depth24Stencil8});
        CHECK(rt1.hasValue());

        *rt0 = ZA_MOVE(*rt1);
    }
}
