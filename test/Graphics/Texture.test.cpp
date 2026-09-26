#include "Zancle/Graphics/Texture.hpp"

// Other 1st party headers
#include "GraphicsUtil.hpp"
#include "LoadIntoMemoryUtil.hpp"
#include "Tst/Tst.hpp"
#include "WindowUtil.hpp"

#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/TextureWrapMode.hpp"

#include "Zancle/IO/FileInputStream.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsDefaultConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"
#include "Zancle/Trait/IsNothrowSwappable.hpp"


namespace
{

const za::Vector<char>& sharedLogoBytes()
{
    static auto bytes = loadIntoMemory("zancle-logo-big.png");
    return bytes;
}

} // namespace


TEST_CASE("[Graphics] za::Texture" * tst::skip(skipDisplayTests))
{
    [[maybe_unused]] auto& graphicsContext = TST_CASE_SHARED(za::GraphicsContext::create().value());

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_DEFAULT_CONSTRUCTIBLE(za::Texture));
        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::Texture));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::Texture));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::Texture));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::Texture));
        STATIC_CHECK(ZA_IS_NOTHROW_SWAPPABLE(za::Texture));
    }

    SECTION("Move semantics")
    {
        SECTION("Construction")
        {
            za::Texture       movedTexture = za::Texture::create({64, 64}).value();
            const za::Texture texture      = ZA_MOVE(movedTexture);
            CHECK(texture.getSize() == za::Vec2u{64, 64});
            CHECK(!texture.isSmooth());
            CHECK(!texture.isSrgb());
            CHECK(texture.getWrapMode() == za::TextureWrapMode::Clamp);
            CHECK(texture.getNativeHandle() != 0);
        }

        SECTION("Assignment")
        {
            za::Texture movedTexture = za::Texture::create({64, 64}).value();
            za::Texture texture      = za::Texture::create({128, 128}).value();
            texture                  = ZA_MOVE(movedTexture);
            CHECK(texture.getSize() == za::Vec2u{64, 64});
            CHECK(!texture.isSmooth());
            CHECK(!texture.isSrgb());
            CHECK(texture.getWrapMode() == za::TextureWrapMode::Clamp);
            CHECK(texture.getNativeHandle() != 0);
        }

        SECTION("Move assignment")
        {
            auto rt0 = za::Texture::create({100, 100});
            CHECK(rt0.hasValue());
            rt0->setSmooth(true);

            auto rt1 = za::Texture::create({100, 100});
            CHECK(rt1.hasValue());
            rt1->setSmooth(true);

            *rt0 = ZA_MOVE(*rt1);
            rt0->setSmooth(true);
        }
    }

    SECTION("create()")
    {
        SECTION("At least one zero dimension")
        {
            CHECK(!za::Texture::create({}).hasValue());
            CHECK(!za::Texture::create({0, 1}).hasValue());
            CHECK(!za::Texture::create({1, 0}).hasValue());
        }

        SECTION("Valid size")
        {
            const auto texture = za::Texture::create({100, 100}).value();
            CHECK(texture.getSize() == za::Vec2u{100, 100});
            CHECK(texture.getNativeHandle() != 0);
        }

        SECTION("Too large")
        {
            CHECK(!za::Texture::create({100'000, 100'000}).hasValue());
            CHECK(!za::Texture::create({1'000'000, 1'000'000}).hasValue());
        }
    }

    SECTION("clear()")
    {
        auto texture = za::Texture::create({7, 5}).value();

        REQUIRE(texture.clear());
        const auto transparentImage = texture.copyToImage();

        REQUIRE(texture.clear(za::Color::Red));
        const auto redImage = texture.copyToImage();

        for (unsigned int y = 0u; y < 5u; ++y)
            for (unsigned int x = 0u; x < 7u; ++x)
            {
                CHECK(transparentImage.getPixel({x, y}) == za::Color::Transparent);
                CHECK(redImage.getPixel({x, y}) == za::Color::Red);
            }
    }

    SECTION("loadFromFile()")
    {
        const auto texture = za::Texture::loadFromFile("zancle-logo-big.png").value();
        CHECK(texture.getSize() == za::Vec2u{1001, 304});
        CHECK(!texture.isSmooth());
        CHECK(!texture.isSrgb());
        CHECK(texture.getWrapMode() == za::TextureWrapMode::Clamp);
        CHECK(texture.getNativeHandle() != 0);
    }

    SECTION("loadFromMemory()")
    {
        const auto& memory  = sharedLogoBytes();
        const auto  texture = za::Texture::loadFromMemory(memory.data(), memory.size()).value();
        CHECK(texture.getSize() == za::Vec2u{1001, 304});
        CHECK(!texture.isSmooth());
        CHECK(!texture.isSrgb());
        CHECK(texture.getWrapMode() == za::TextureWrapMode::Clamp);
        CHECK(texture.getNativeHandle() != 0);
    }

    SECTION("loadFromStream()")
    {
        auto       stream  = za::FileInputStream::open("zancle-logo-big.png").value();
        const auto texture = za::Texture::loadFromStream(stream).value();
        CHECK(texture.getSize() == za::Vec2u{1001, 304});
        CHECK(!texture.isSmooth());
        CHECK(!texture.isSrgb());
        CHECK(texture.getWrapMode() == za::TextureWrapMode::Clamp);
        CHECK(texture.getNativeHandle() != 0);
    }

    SECTION("loadFromImage()")
    {
        SECTION("Subarea of image")
        {
            const auto image = za::Image::create(za::Vec2u{10, 15}).value();

            SECTION("Non-truncated area")
            {
                const auto texture = za::Texture::loadFromImage(image, {.sRgb = false, .area = {{0, 0}, {5, 10}}}).value();
                CHECK(texture.getSize() == za::Vec2u{5, 10});
                CHECK(texture.getNativeHandle() != 0);
            }

            SECTION("Truncated area (negative position)")
            {
                const auto texture = za::Texture::loadFromImage(image, {.sRgb = false, .area = {{-5, -5}, {4, 8}}}).value();
                CHECK(texture.getSize() == za::Vec2u{4, 8});
                CHECK(texture.getNativeHandle() != 0);
            }

            SECTION("Truncated area (width/height too big)")
            {
                const auto texture = za::Texture::loadFromImage(image, {.sRgb = false, .area = {{5, 5}, {12, 18}}}).value();
                CHECK(texture.getSize() == za::Vec2u{5, 10});
                CHECK(texture.getNativeHandle() != 0);
            }
        }
    }

    SECTION("Copy semantics")
    {
        constexpr za::U8 red[]{0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF};

        auto texture = za::Texture::create({1, 2}).value();
        texture.update(red);
        texture.setWrapMode(za::TextureWrapMode::Repeat);

        SECTION("Construction")
        {
            const za::Texture textureCopy(texture); // NOLINT(performance-unnecessary-copy-initialization)
            REQUIRE(textureCopy.getSize() == za::Vec2u{1, 2});
            CHECK(textureCopy.copyToImage().getPixel(za::Vec2u{0, 1}) == za::Color::Red);
            CHECK(textureCopy.getWrapMode() == za::TextureWrapMode::Repeat);
        }

        SECTION("Assignment")
        {
            za::Texture textureCopy = za::Texture::create({64, 64}).value();
            textureCopy             = texture;
            REQUIRE(textureCopy.getSize() == za::Vec2u{1, 2});
            CHECK(textureCopy.copyToImage().getPixel(za::Vec2u{0, 1}) == za::Color::Red);
            CHECK(textureCopy.getWrapMode() == za::TextureWrapMode::Repeat);
        }
    }

    SECTION("update()")
    {
        constexpr za::U8 yellow[]{0xFF, 0xFF, 0x00, 0xFF};
        constexpr za::U8 cyan[]{0x00, 0xFF, 0xFF, 0xFF};

        SECTION("Pixels")
        {
            auto texture = za::Texture::create(za::Vec2u{1, 1}).value();
            texture.update(yellow);
            CHECK(texture.copyToImage().getPixel(za::Vec2u{0, 0}) == za::Color::Yellow);
        }

        SECTION("Pixels, size and destination")
        {
            auto texture = za::Texture::create(za::Vec2u{2, 1}).value();
            texture.update(yellow, za::Vec2u{1, 1}, za::Vec2u{0, 0});
            texture.update(cyan, za::Vec2u{1, 1}, za::Vec2u{1, 0});

            const auto textureAsImage = texture.copyToImage();
            CHECK(textureAsImage.getPixel(za::Vec2u{0, 0}) == za::Color::Yellow);
            CHECK(textureAsImage.getPixel(za::Vec2u{1, 0}) == za::Color::Cyan);
        }

        SECTION("Another texture")
        {
            auto otherTexture = za::Texture::create(za::Vec2u{1, 1}).value();
            otherTexture.update(cyan);
            auto texture = za::Texture::create(za::Vec2u{1, 1}).value();
            CHECK(texture.update(otherTexture));
            CHECK(texture.copyToImage().getPixel(za::Vec2u{0, 0}) == za::Color::Cyan);
        }

        SECTION("Another texture and destination")
        {
            auto texture       = za::Texture::create(za::Vec2u{2, 1}).value();
            auto otherTexture1 = za::Texture::create(za::Vec2u{1, 1}).value();
            otherTexture1.update(cyan);
            auto otherTexture2 = za::Texture::create(za::Vec2u{1, 1}).value();
            otherTexture2.update(yellow);
            CHECK(texture.update(otherTexture1, za::Vec2u{0, 0}));
            CHECK(texture.update(otherTexture2, za::Vec2u{1, 0}));

            const auto textureAsImage = texture.copyToImage();
            CHECK(textureAsImage.getPixel(za::Vec2u{0, 0}) == za::Color::Cyan);
            CHECK(textureAsImage.getPixel(za::Vec2u{1, 0}) == za::Color::Yellow);
        }

        SECTION("Image")
        {
            auto       texture = za::Texture::create(za::Vec2u{16, 32}).value();
            const auto image   = za::Image::create(za::Vec2u{16, 32}, za::Color::Red).value();
            texture.update(image);
            CHECK(texture.copyToImage().getPixel(za::Vec2u{7, 15}) == za::Color::Red);
        }

        SECTION("Image and destination")
        {
            auto       texture = za::Texture::create(za::Vec2u{16, 32}).value();
            const auto image1  = za::Image::create(za::Vec2u{16, 16}, za::Color::Red).value();
            texture.update(image1);
            const auto image2 = za::Image::create(za::Vec2u{16, 16}, za::Color::Green).value();
            texture.update(image1, za::Vec2u{0, 0});
            texture.update(image2, za::Vec2u{0, 16});

            const auto textureAsImage = texture.copyToImage();
            CHECK(textureAsImage.getPixel(za::Vec2u{7, 7}) == za::Color::Red);
            CHECK(textureAsImage.getPixel(za::Vec2u{7, 22}) == za::Color::Green);
        }
    }

    SECTION("Set/get smooth")
    {
        auto texture = za::Texture::create({64, 64}).value();
        CHECK(!texture.isSmooth());
        texture.setSmooth(true);
        CHECK(texture.isSmooth());
        texture.setSmooth(false);
        CHECK(!texture.isSmooth());
    }

    SECTION("Set/get repeated")
    {
        auto texture = za::Texture::create({64, 64}).value();
        CHECK(texture.getWrapMode() == za::TextureWrapMode::Clamp);
        texture.setWrapMode(za::TextureWrapMode::Repeat);
        CHECK(texture.getWrapMode() == za::TextureWrapMode::Repeat);
        texture.setWrapMode(za::TextureWrapMode::Clamp);
        CHECK(texture.getWrapMode() == za::TextureWrapMode::Clamp);
    }

    SECTION("generateMipmap()")
    {
        auto texture = za::Texture::create({100, 100}).value();
        texture.generateMipmap();
    }

    SECTION("swap()")
    {
        constexpr za::U8 blue[]{0x00, 0x00, 0xFF, 0xFF};
        constexpr za::U8 green[]{0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF};

        auto texture1 = za::Texture::create(za::Vec2u{1, 1}, {.sRgb = true}).value();
        texture1.update(blue);
        texture1.setSmooth(false);
        texture1.setWrapMode(za::TextureWrapMode::Repeat);

        auto texture2 = za::Texture::create(za::Vec2u{2, 1}, {.sRgb = false}).value();
        texture2.update(green);
        texture2.setSmooth(true);
        texture2.setWrapMode(za::TextureWrapMode::Clamp);

        za::swap(texture1, texture2);
        CHECK_FALSE(texture1.isSrgb());
        CHECK(texture1.isSmooth());
        CHECK_FALSE(texture1.getWrapMode() == za::TextureWrapMode::Repeat);
        // Cannot check texture2.isSrgb() because Srgb is sometimes disabled when using OpenGL ES
        CHECK_FALSE(texture2.isSmooth());
        CHECK(texture2.getWrapMode() == za::TextureWrapMode::Repeat);

        const auto image1 = texture1.copyToImage();
        const auto image2 = texture2.copyToImage();
        REQUIRE(image1.getSize() == za::Vec2u{2, 1});
        REQUIRE(image2.getSize() == za::Vec2u{1, 1});
        CHECK(image1.getPixel(za::Vec2u{1, 0}) == za::Color::Green);
        CHECK(image2.getPixel(za::Vec2u{0, 0}) == za::Color::Blue);
    }

    SECTION("Get Maximum Size")
    {
        CHECK(za::Texture::getMaximumSize() > 0);
    }
}
