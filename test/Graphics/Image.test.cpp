#include "Zancle/Graphics/Image.hpp"

// Other 1st party headers
#include "GraphicsUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/IO/FileInputStream.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsDefaultConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


TEST_CASE("[Graphics] za::Image")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_DEFAULT_CONSTRUCTIBLE(za::Image));
        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::Image));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::Image));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::Image));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::Image));
    }

    SECTION("Construction")
    {
        SECTION("Invalid size")
        {
            CHECK(!za::Image::create({10, 0}, za::Color::Magenta).hasValue());
            CHECK(!za::Image::create({0, 10}, za::Color::Magenta).hasValue());
        }

        SECTION("Vec2 constructor")
        {
            const auto image = za::Image::create(za::Vec2u{10, 10}).value();
            CHECK(image.getSize() == za::Vec2u{10, 10});
            CHECK(image.getPixelsPtr() != nullptr);

            for (za::U32 i = 0; i < 10; ++i)
            {
                for (za::U32 j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(za::Vec2u{i, j}) == za::Color::Black);
                }
            }
        }

        SECTION("Vec2 and color constructor")
        {
            const auto image = za::Image::create(za::Vec2u{10, 10}, za::Color::Red).value();
            CHECK(image.getSize() == za::Vec2u{10, 10});
            CHECK(image.getPixelsPtr() != nullptr);

            for (za::U32 i = 0; i < 10; ++i)
            {
                for (za::U32 j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(za::Vec2u{i, j}) == za::Color::Red);
                }
            }
        }

        SECTION("Vec2 and za::U8* constructor")
        {
            // 10 x 10, with 4 color channels array
            za::U8 pixels[400]{};
            for (za::SizeT i = 0; i < 400; i += 4)
            {
                pixels[i]     = 255; // r
                pixels[i + 1] = 0;   // g
                pixels[i + 2] = 0;   // b
                pixels[i + 3] = 255; // a
            }

            const auto image = za::Image::create(za::Vec2u{10, 10}, pixels).value();
            CHECK(image.getSize() == za::Vec2u{10, 10});
            CHECK(image.getPixelsPtr() != nullptr);

            for (za::U32 i = 0; i < 10; ++i)
            {
                for (za::U32 j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(za::Vec2u{i, j}) == za::Color::Red);
                }
            }
        }
    }

    SECTION("loadFromFile()")
    {
        SECTION("Invalid file") // TODO P1: fails under CLANG64 env
        {
            CHECK(!za::Image::loadFromFile(".").hasValue());
            CHECK(!za::Image::loadFromFile("this/does/not/exist.jpg").hasValue());

            // small n with tilde, from Spanish, outside of ASCII, inside common Latin 1 codepage
            CHECK(!za::Image::loadFromFile(za::Path(U"missing-file-ñ.png")).hasValue());

            // small n with acute accent, from Polish, outside of Latin 1 codepage
            CHECK(!za::Image::loadFromFile(za::Path(U"missing-file-ń.png")).hasValue());

            // CJK symbol for Sun, outside of any European language codepage
            CHECK(!za::Image::loadFromFile(za::Path(U"missing-file-日.png")).hasValue());

            // snail emoji, outside of Unicode Basic Multilingual Plane
            CHECK(!za::Image::loadFromFile(za::Path(U"missing-file-🐌.png")).hasValue());
        }

        SECTION("Successful load")
        {
            za::Optional<za::Image> image;

            SECTION("bmp")
            {
                image = za::Image::loadFromFile("zancle-logo-big.bmp");
                REQUIRE(image.hasValue());
                CHECK(image->getPixel({0, 0}) == za::Color::White);
                CHECK(image->getPixel({200, 150}) == za::Color(144, 208, 62));
            }

            SECTION("png")
            {
                image = za::Image::loadFromFile("zancle-logo-big.png");
                REQUIRE(image.hasValue());
                CHECK(image->getPixel({0, 0}) == za::Color(255, 255, 255, 0));
                CHECK(image->getPixel({200, 150}) == za::Color(144, 208, 62));
            }

            SECTION("jpg")
            {
                image = za::Image::loadFromFile("zancle-logo-big.jpg");
                REQUIRE(image.hasValue());
                CHECK(image->getPixel({0, 0}) == za::Color::White);
                CHECK(image->getPixel({200, 150}) == za::Color(144, 208, 62));
            }

            SECTION("gif")
            {
                image = za::Image::loadFromFile("zancle-logo-big.gif");
                REQUIRE(image.hasValue());
                CHECK(image->getPixel({0, 0}) == za::Color::White);
                CHECK(image->getPixel({200, 150}) == za::Color(146, 210, 62));
            }

            SECTION("psd")
            {
                image = za::Image::loadFromFile("zancle-logo-big.psd");
                REQUIRE(image.hasValue());
                CHECK(image->getPixel({0, 0}) == za::Color::White);
                CHECK(image->getPixel({200, 150}) == za::Color(144, 208, 62));
            }

            SECTION("qoi")
            {
                image = za::Image::loadFromFile("zancle-logo-big.qoi");
                REQUIRE(image.hasValue());
                CHECK(image->getPixel({0, 0}) == za::Color(255, 255, 255, 0));
                CHECK(image->getPixel({200, 150}) == za::Color(144, 208, 62));
            }

            CHECK(image->getSize() == za::Vec2u{1001, 304});
            CHECK(image->getPixelsPtr() != nullptr);
        }
    }

    SECTION("loadFromMemory()")
    {
        SECTION("Invalid pointer")
        {
            CHECK(!za::Image::loadFromMemory(nullptr, 1).hasValue());
        }

        SECTION("Invalid size")
        {
            const unsigned char testByte{0xAB};
            CHECK(!za::Image::loadFromMemory(&testByte, 0).hasValue());
        }

        SECTION("Failed load")
        {
            za::Vector<za::U8> memory;

            SECTION("Empty")
            {
                memory.clear();
            }

            SECTION("Junk data")
            {
                memory.pushBackMultiple(za::U8{1}, za::U8{2}, za::U8{3}, za::U8{4});
            }

            CHECK(!za::Image::loadFromMemory(memory.data(), memory.size()).hasValue());
        }

        SECTION("Successful load")
        {
            const auto memory = za::Image::create({24, 24}, za::Color::Green).value().saveToMemory(za::Image::SaveFormat::PNG);

            const auto image = za::Image::loadFromMemory(memory.data(), memory.size()).value();
            CHECK(image.getSize() == za::Vec2u{24, 24});

            CHECK(image.getPixelsPtr() != nullptr);
            CHECK(image.getPixel({0, 0}) == za::Color::Green);
            CHECK(image.getPixel({23, 23}) == za::Color::Green);
        }
    }

    SECTION("loadFromStream()")
    {
        auto       stream = za::FileInputStream::open("zancle-logo-big.png").value();
        const auto image  = za::Image::loadFromStream(stream).value();
        CHECK(image.getSize() == za::Vec2u{1001, 304});
        CHECK(image.getPixelsPtr() != nullptr);
        CHECK(image.getPixel({0, 0}) == za::Color(255, 255, 255, 0));
        CHECK(image.getPixel({200, 150}) == za::Color(144, 208, 62));
    }

    SECTION("saveToFile()")
    {
        const auto image = za::Image::create({256, 256}, za::Color::Magenta).value();

        SECTION("No extension")
        {
            CHECK(!image.saveToFile("wheresmyextension"));
            CHECK(!image.saveToFile("pls/add/extension"));
        }

        SECTION("Invalid extension")
        {
            CHECK(!image.saveToFile("test.ps"));
            CHECK(!image.saveToFile("test.foo"));
        }

        SECTION("Successful save")
        {
            SECTION("To .bmp")
            {
                auto filename = za::Path::getTempDirectory().value() / za::Path("test.bmp");
                CHECK(image.saveToFile(filename));

                const auto loadedImage = za::Image::loadFromFile(filename).value();
                CHECK(loadedImage.getSize() == za::Vec2u{256, 256});
                CHECK(loadedImage.getPixelsPtr() != nullptr);

                CHECK(filename.removeFromDisk());
            }

            SECTION("To .tga")
            {
                auto filename = za::Path::getTempDirectory().value() / za::Path("test.tga");
                CHECK(image.saveToFile(filename));

                const auto loadedImage = za::Image::loadFromFile(filename).value();
                CHECK(loadedImage.getSize() == za::Vec2u{256, 256});
                CHECK(loadedImage.getPixelsPtr() != nullptr);

                CHECK(filename.removeFromDisk());
            }

            SECTION("To .png")
            {
                auto filename = za::Path::getTempDirectory().value() / za::Path("test.png");
                CHECK(image.saveToFile(filename));

                const auto loadedImage = za::Image::loadFromFile(filename).value();
                CHECK(loadedImage.getSize() == za::Vec2u{256, 256});
                CHECK(loadedImage.getPixelsPtr() != nullptr);

                CHECK(filename.removeFromDisk());
            }

            SECTION("To .qoi")
            {
                auto filename = za::Path::getTempDirectory().value() / za::Path("test.qoi");
                CHECK(image.saveToFile(filename));

                const auto loadedImage = za::Image::loadFromFile(filename).value();
                CHECK(loadedImage.getSize() == za::Vec2u{256, 256});
                CHECK(loadedImage.getPixelsPtr() != nullptr);

                CHECK(filename.removeFromDisk());
            }

            SECTION("To Spanish Latin1 filename .png")
            {
                // small n with tilde, from Spanish, outside of ASCII, inside common Latin 1 codepage
                auto filename = za::Path::getTempDirectory().value() / za::Path(U"test-ñ.png");
                CHECK(image.saveToFile(filename));

                const auto loadedImage = za::Image::loadFromFile(filename).value();
                CHECK(loadedImage.getSize() == za::Vec2u{256, 256});
                CHECK(loadedImage.getPixelsPtr() != nullptr);

                CHECK(filename.removeFromDisk());
            }

            SECTION("To Polish filename .png")
            {
                // small n with acute accent, from Polish, outside of Latin 1 codepage
                auto filename = za::Path::getTempDirectory().value() / za::Path(U"test-ń.png");
                CHECK(image.saveToFile(filename));

                const auto loadedImage = za::Image::loadFromFile(filename).value();
                CHECK(loadedImage.getSize() == za::Vec2u{256, 256});
                CHECK(loadedImage.getPixelsPtr() != nullptr);

                CHECK(filename.removeFromDisk());
            }

            SECTION("To Japanese CJK filename .png")
            {
                // CJK symbol for Sun, outside of any European language codepage
                auto filename = za::Path::getTempDirectory().value() / za::Path(U"test-日.png");
                CHECK(image.saveToFile(filename));

                const auto loadedImage = za::Image::loadFromFile(filename).value();
                CHECK(loadedImage.getSize() == za::Vec2u{256, 256});
                CHECK(loadedImage.getPixelsPtr() != nullptr);

                CHECK(filename.removeFromDisk());
            }

            SECTION("To emoji non-BMP Unicode filename .png")
            {
                // snail emoji, outside of Unicode Basic Multilingual Plane
                auto filename = za::Path::getTempDirectory().value() / za::Path(U"test-🐌.png");
                CHECK(image.saveToFile(filename));

                const auto loadedImage = za::Image::loadFromFile(filename).value();
                CHECK(loadedImage.getSize() == za::Vec2u{256, 256});
                CHECK(loadedImage.getPixelsPtr() != nullptr);

                CHECK(filename.removeFromDisk());
            }

            // Cannot test JPEG encoding due to it triggering UB in stbiw__jpg_writeBits
        }
    }

    SECTION("saveToMemory()")
    {
        const auto image = za::Image::create({16, 16}, za::Color::Magenta).value();

        SECTION("Successful save")
        {
            za::Vector<za::U8> output;

            SECTION("To bmp")
            {
                output = image.saveToMemory(za::Image::SaveFormat::BMP);
                REQUIRE(output.size() == 1146);
                CHECK(output[0] == 66);
                CHECK(output[1] == 77);
                CHECK(output[2] == 122);
                CHECK(output[3] == 4);
                CHECK(output[1000] == 255);
                CHECK(output[1001] == 255);
                CHECK(output[1002] == 255);
                CHECK(output[1003] == 0);
            }

            SECTION("To tga")
            {
                output = image.saveToMemory(za::Image::SaveFormat::TGA);
                REQUIRE(output.size() == 98);
                CHECK(output[0] == 0);
                CHECK(output[1] == 0);
                CHECK(output[2] == 10);
                CHECK(output[3] == 0);
            }

            SECTION("To png")
            {
                output = image.saveToMemory(za::Image::SaveFormat::PNG);
                REQUIRE(output.size() == 92);
                CHECK(output[0] == 137);
                CHECK(output[1] == 80);
                CHECK(output[2] == 78);
                CHECK(output[3] == 71);
            }

            SECTION("To qoi")
            {
                output = image.saveToMemory(za::Image::SaveFormat::QOI);
                REQUIRE(output.size() == 28);
                CHECK(output[0] == 113);
                CHECK(output[1] == 111);
                CHECK(output[2] == 105);
                CHECK(output[3] == 102);
            }

            // Cannot test JPEG encoding due to it triggering UB in stbiw__jpg_writeBits
        }
    }

    SECTION("Set/get pixel")
    {
        auto image = za::Image::create(za::Vec2u{10, 10}, za::Color::Green).value();
        CHECK(image.getPixel(za::Vec2u{2, 2}) == za::Color::Green);

        image.setPixel(za::Vec2u{2, 2}, za::Color::Blue);
        CHECK(image.getPixel(za::Vec2u{2, 2}) == za::Color::Blue);
    }

    SECTION("Copy from Image")
    {
        SECTION("Copy (Image, Vec2u)")
        {
            const auto image1 = za::Image::create(za::Vec2u{10, 10}, za::Color::Blue).value();
            auto       image2 = za::Image::create(za::Vec2u{10, 10}).value();
            CHECK(image2.copy(image1, za::Vec2u{0, 0}));

            for (za::U32 i = 0; i < 10; ++i)
            {
                for (za::U32 j = 0; j < 10; ++j)
                {
                    CHECK(image1.getPixel(za::Vec2u{i, j}) == image2.getPixel(za::Vec2u{i, j}));
                }
            }
        }

        SECTION("Copy (Image, Vec2u, Rect2i)")
        {
            const auto image1 = za::Image::create(za::Vec2u{5, 5}, za::Color::Blue).value();
            auto       image2 = za::Image::create(za::Vec2u{10, 10}).value();
            CHECK(image2.copy(image1, za::Vec2u{0, 0}, za::Rect2i(za::Vec2i{0, 0}, za::Vec2i{5, 5})));

            for (za::U32 i = 0; i < 10; ++i)
            {
                for (za::U32 j = 0; j < 10; ++j)
                {
                    if (i <= 4 && j <= 4)
                        CHECK(image2.getPixel(za::Vec2u{i, j}) == za::Color::Blue);
                    else
                        CHECK(image2.getPixel(za::Vec2u{i, j}) == za::Color::Black);
                }
            }
        }

        SECTION("Copy (Image, Vec2u, Rect2i, bool)")
        {
            const za::Color dest(255, 0, 0, 255);
            const za::Color source(5, 255, 78, 232);

            // Create the composited color for via the alpha composite over operation
            const auto a = static_cast<za::U8>(source.a + (dest.a * (255 - source.a)) / 255);
            const auto r = static_cast<za::U8>(((source.r * source.a) + ((dest.r * dest.a) * (255 - source.a)) / 255) / a);
            const auto g = static_cast<za::U8>(((source.g * source.a) + ((dest.g * dest.a) * (255 - source.a)) / 255) / a);
            const auto b = static_cast<za::U8>(((source.b * source.a) + ((dest.b * dest.a) * (255 - source.a)) / 255) / a);
            const za::Color composite(r, g, b, a);

            auto       image1 = za::Image::create(za::Vec2u{10, 10}, dest).value();
            const auto image2 = za::Image::create(za::Vec2u{10, 10}, source).value();
            CHECK(image1.copy(image2, za::Vec2u{0, 0}, za::Rect2i(za::Vec2i{0, 0}, za::Vec2i{10, 10}), true));

            for (za::U32 i = 0; i < 10; ++i)
            {
                for (za::U32 j = 0; j < 10; ++j)
                {
                    CHECK(image1.getPixel(za::Vec2u{i, j}) == composite);
                }
            }
        }

        SECTION("Copy (Out of bounds sourceRect)")
        {
            const auto image1 = za::Image::create(za::Vec2u{5, 5}, za::Color::Blue).value();
            auto       image2 = za::Image::create(za::Vec2u{10, 10}, za::Color::Red).value();
            CHECK(!image2.copy(image1, za::Vec2u{0, 0}, za::Rect2i(za::Vec2i{5, 5}, za::Vec2i{9, 9})));

            for (za::U32 i = 0; i < 10; ++i)
            {
                for (za::U32 j = 0; j < 10; ++j)
                {
                    CHECK(image2.getPixel(za::Vec2u{i, j}) == za::Color::Red);
                }
            }
        }
    }

    SECTION("Create mask from color")
    {
        SECTION("createMaskFromColor(Color)")
        {
            auto image = za::Image::create(za::Vec2u{10, 10}, za::Color::Blue).value();
            image.createMaskFromColor(za::Color::Blue);

            for (za::U32 i = 0; i < 10; ++i)
            {
                for (za::U32 j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(za::Vec2u{i, j}) == za::Color(0, 0, 255, 0));
                }
            }
        }

        SECTION("createMaskFromColor(Color, za::U8)")
        {
            auto image = za::Image::create(za::Vec2u{10, 10}, za::Color::Blue).value();
            image.createMaskFromColor(za::Color::Blue, 100);

            for (za::U32 i = 0; i < 10; ++i)
            {
                for (za::U32 j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(za::Vec2u{i, j}) == za::Color(0, 0, 255, 100));
                }
            }
        }
    }

    SECTION("Flip horizontally")
    {
        auto image = za::Image::create(za::Vec2u{10, 10}, za::Color::Red).value();
        image.setPixel(za::Vec2u{0, 0}, za::Color::Green);
        image.flipHorizontally();

        CHECK(image.getPixel(za::Vec2u{9, 0}) == za::Color::Green);
    }

    SECTION("Flip vertically")
    {
        auto image = za::Image::create(za::Vec2u{10, 10}, za::Color::Red).value();
        image.setPixel(za::Vec2u{0, 0}, za::Color::Green);
        image.flipVertically();

        CHECK(image.getPixel(za::Vec2u{0, 9}) == za::Color::Green);
    }
}
