#include "GraphicsUtil.hpp"
#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Graphics/Color.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsStandardLayout.hpp"
#include "Zancle/Trait/IsTrivial.hpp"
#include "Zancle/Trait/IsTriviallyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyConstructible.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyMoveAssignable.hpp"
#include "Zancle/Trait/IsTriviallyMoveConstructible.hpp"

#include "Zancle/Base/IntTypes.hpp"


TEST_CASE("[Graphics] za::Color")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Color));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Color));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Color));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Color));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Color)); // because of member initializers
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(za::Color));
        STATIC_CHECK(ZA_IS_AGGREGATE(za::Color));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Color));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Color));
        STATIC_CHECK(ZA_IS_TRIVIALLY_ASSIGNABLE(za::Color, za::Color));
    }

    SECTION("Construction")
    {
        SECTION("Default constructor")
        {
            constexpr za::Color color{};
            STATIC_CHECK(color.r == 0);
            STATIC_CHECK(color.g == 0);
            STATIC_CHECK(color.b == 0);
            STATIC_CHECK(color.a == 255);
        }

        SECTION("(r, g, b) constructor")
        {
            constexpr za::Color color(1, 2, 3);
            STATIC_CHECK(color.r == 1);
            STATIC_CHECK(color.g == 2);
            STATIC_CHECK(color.b == 3);
            STATIC_CHECK(color.a == 255);
        }

        SECTION("(r, g, b, a) constructor")
        {
            constexpr za::Color color(1, 2, 3, 4);
            STATIC_CHECK(color.r == 1);
            STATIC_CHECK(color.g == 2);
            STATIC_CHECK(color.b == 3);
            STATIC_CHECK(color.a == 4);
        }

        SECTION("za::I32 constructor")
        {
            STATIC_CHECK(za::Color::fromRGBA(0x00'00'00'00) == za::Color(0, 0, 0, 0));
            STATIC_CHECK(za::Color::fromRGBA(0x01'02'03'04) == za::Color(1, 2, 3, 4));
            STATIC_CHECK(za::Color::fromRGBA(0xFF'FF'FF'FF) == za::Color(255, 255, 255, 255));
        }
    }

    SECTION("toInteger()")
    {
        STATIC_CHECK(za::Color(0, 0, 0, 0).toInteger() == 0x00'00'00'00);
        STATIC_CHECK(za::Color(1, 2, 3, 4).toInteger() == 0x01'02'03'04);
        STATIC_CHECK(za::Color(255, 255, 255, 255).toInteger() == 0xFF'FF'FF'FF);
    }

    SECTION("Operations")
    {
        SECTION("operator==")
        {
            STATIC_CHECK(za::Color() == za::Color());
            STATIC_CHECK(za::Color(1, 2, 3, 4) == za::Color(1, 2, 3, 4));

            STATIC_CHECK_FALSE(za::Color(1, 0, 0, 0) == za::Color(0, 0, 0, 0));
            STATIC_CHECK_FALSE(za::Color(0, 1, 0, 0) == za::Color(0, 0, 0, 0));
            STATIC_CHECK_FALSE(za::Color(0, 0, 1, 0) == za::Color(0, 0, 0, 0));
            STATIC_CHECK_FALSE(za::Color(0, 0, 0, 1) == za::Color(0, 0, 0, 0));
        }

        SECTION("operator!=")
        {
            STATIC_CHECK(za::Color(1, 0, 0, 0) != za::Color(0, 0, 0, 0));
            STATIC_CHECK(za::Color(0, 1, 0, 0) != za::Color(0, 0, 0, 0));
            STATIC_CHECK(za::Color(0, 0, 1, 0) != za::Color(0, 0, 0, 0));
            STATIC_CHECK(za::Color(0, 0, 0, 1) != za::Color(0, 0, 0, 0));

            STATIC_CHECK_FALSE(za::Color() != za::Color());
            STATIC_CHECK_FALSE(za::Color(1, 2, 3, 4) != za::Color(1, 2, 3, 4));
        }

        SECTION("operator+")
        {
            STATIC_CHECK(za::Color(0, 0, 0, 0) + za::Color(0, 0, 0, 0) == za::Color(0, 0, 0, 0));
            STATIC_CHECK(za::Color(50, 50, 50, 50) + za::Color(50, 50, 50, 50) == za::Color(100, 100, 100, 100));
            STATIC_CHECK(za::Color(100, 100, 100, 100) + za::Color(100, 100, 100, 100) == za::Color(200, 200, 200, 200));
            STATIC_CHECK(za::Color(150, 150, 150, 150) + za::Color(150, 150, 150, 150) == za::Color(255, 255, 255, 255));
            STATIC_CHECK(za::Color(255, 255, 255, 255) + za::Color(255, 255, 255, 255) == za::Color(255, 255, 255, 255));
        }

        SECTION("operator-")
        {
            constexpr za::Color c(50, 50, 50, 50);
            constexpr za::Color c2(150, 150, 150, 150);
            STATIC_CHECK(c2 - c == za::Color(100, 100, 100, 100));
            STATIC_CHECK(c - c2 == za::Color(0, 0, 0, 0));
        }

        SECTION("operator*")
        {
            constexpr za::Color c(255, 255, 255, 255);
            constexpr za::Color c2(2, 2, 2, 2);
            STATIC_CHECK(c * c2 == za::Color(2, 2, 2, 2));
            STATIC_CHECK(c2 * c == za::Color(2, 2, 2, 2));
        }

        SECTION("operator+=")
        {
            za::Color color(42, 42, 42, 42);
            color += za::Color(1, 1, 1, 1);
            CHECK(color == za::Color(43, 43, 43, 43));
            color += za::Color(250, 250, 250, 250);
            CHECK(color == za::Color(255, 255, 255, 255));
        }

        SECTION("operator-=")
        {
            za::Color color(248, 248, 248, 248);
            color -= za::Color(1, 1, 1, 1);
            CHECK(color == za::Color(247, 247, 247, 247));
            color -= za::Color(250, 250, 250, 250);
            CHECK(color == za::Color(0, 0, 0, 0));
        }

        SECTION("operator*=")
        {
            za::Color color(50, 50, 50, 50);
            color *= za::Color(20, 20, 20, 20);
            CHECK(color == za::Color(3, 3, 3, 3));
            color *= za::Color(120, 120, 120, 120);
            CHECK(color == za::Color(1, 1, 1, 1));
        }
    }

    SECTION("Constants")
    {
        STATIC_CHECK(za::Color::Black == za::Color(0, 0, 0));
        STATIC_CHECK(za::Color::White == za::Color(255, 255, 255));
        STATIC_CHECK(za::Color::Red == za::Color(255, 0, 0));
        STATIC_CHECK(za::Color::Green == za::Color(0, 255, 0));
        STATIC_CHECK(za::Color::Blue == za::Color(0, 0, 255));
        STATIC_CHECK(za::Color::Yellow == za::Color(255, 255, 0));
        STATIC_CHECK(za::Color::Magenta == za::Color(255, 0, 255));
        STATIC_CHECK(za::Color::Cyan == za::Color(0, 255, 255));
        STATIC_CHECK(za::Color::Transparent == za::Color(0, 0, 0, 0));
    }

    SECTION("Reinterpret as za::U8*")
    {
        STATIC_CHECK(sizeof(za::Color) == 4);
        STATIC_CHECK(alignof(za::Color) == 1);

        za::Vector<za::Color> pixels(3);
        pixels[0] = {10, 11, 12, 13};
        pixels[1] = {14, 15, 16, 17};
        pixels[2] = {18, 19, 20, 21};

        const auto* begin = reinterpret_cast<const za::U8*>(pixels.data());
        CHECK(begin[0] == pixels[0].r);
        CHECK(begin[1] == pixels[0].g);
        CHECK(begin[2] == pixels[0].b);
        CHECK(begin[3] == pixels[0].a);

        CHECK(begin[4] == pixels[1].r);
        CHECK(begin[5] == pixels[1].g);
        CHECK(begin[6] == pixels[1].b);
        CHECK(begin[7] == pixels[1].a);

        CHECK(begin[8] == pixels[2].r);
        CHECK(begin[9] == pixels[2].g);
        CHECK(begin[10] == pixels[2].b);
        CHECK(begin[11] == pixels[2].a);
    }


    SECTION("HSLtoColor converts basic colors correctly")
    {
        SECTION("Primary colors")
        {
            // Red (Hue = 0)
            CHECK(za::Color::fromHSLA({0.f, 1.f, 0.5f}) == za::Color(255, 0, 0));

            // Green (Hue = 120)
            CHECK(za::Color::fromHSLA({120.f, 1.f, 0.5f}) == za::Color(0, 255, 0));

            // Blue (Hue = 240)
            CHECK(za::Color::fromHSLA({240.f, 1.f, 0.5f}) == za::Color(0, 0, 255));
        }

        SECTION("Secondary colors")
        {
            // Yellow (Hue = 60)
            CHECK(za::Color::fromHSLA({60.f, 1.f, 0.5f}) == za::Color(255, 255, 0));

            // Cyan (Hue = 180)
            CHECK(za::Color::fromHSLA({180.f, 1.f, 0.5f}) == za::Color(0, 255, 255));

            // Magenta (Hue = 300)
            CHECK(za::Color::fromHSLA({300.f, 1.f, 0.5f}) == za::Color(255, 0, 255));
        }
    }

    SECTION("HSLtoColor handles edge cases correctly")
    {
        SECTION("Tiny negative hue wraps to red")
        {
            // Used to wrap to hue `360`, producing an out-of-range hue segment
            CHECK(za::Color::fromHSLA({-1e-6f, 1.f, 0.5f}) == za::Color(255, 0, 0));
        }

        SECTION("Grayscale (Saturation = 0)")
        {
            // Black
            CHECK(za::Color::fromHSLA({0.f, 0.f, 0.f}) == za::Color(0, 0, 0));

            // White
            CHECK(za::Color::fromHSLA({0.f, 0.f, 1.f}) == za::Color(255, 255, 255));

            // 50% Gray (any hue)
            CHECK(za::Color::fromHSLA({0.f, 0.f, 0.5f}) == za::Color(128, 128, 128));
            CHECK(za::Color::fromHSLA({180.f, 0.f, 0.5f}) == za::Color(128, 128, 128));
        }
    }

    SECTION("HSLtoColor produces expected intermediate values")
    {
        SECTION("Pastels (High lightness, medium saturation)")
        {
            za::Color pastelPink = za::Color::fromHSLA({350.f, 0.5f, 0.8f});
            CHECK(pastelPink.r > 200);
            CHECK(pastelPink.g > 150);
            CHECK(pastelPink.b > 150);
        }

        SECTION("Deep colors (Low lightness, high saturation)")
        {
            za::Color deepBlue = za::Color::fromHSLA({240.f, 1.f, 0.2f});
            CHECK(deepBlue.r < 50);
            CHECK(deepBlue.g < 50);
            CHECK(deepBlue.b > 100);
        }
    }

    SECTION("Hue wrapping")
    {
        // Negative hue should wrap to positive
        CHECK(za::Color::fromHSLA({-120.f, 1.f, 0.5f}) == za::Color::fromHSLA({240.f, 1.f, 0.5f}));

        // Hue > 360 should wrap
        CHECK(za::Color::fromHSLA({480.f, 1.f, 0.5f}) == za::Color::fromHSLA({120.f, 1.f, 0.5f}));
    }

    SECTION("Saturation and lightness clamping")
    {
        // Oversaturated should clamp to 1
        CHECK(za::Color::fromHSLA({0.f, 1.5f, 0.5f}) == za::Color::fromHSLA({0.f, 1.f, 0.5f}));

        // Negative saturation should clamp to 0
        CHECK(za::Color::fromHSLA({0.f, -0.5f, 0.5f}) == za::Color::fromHSLA({0.f, 0.f, 0.5f}));

        // Overlight should clamp to 1
        CHECK(za::Color::fromHSLA({0.f, 1.f, 1.5f}) == za::Color::fromHSLA({0.f, 1.f, 1.f}));

        // Negative lightness should clamp to 0
        CHECK(za::Color::fromHSLA({0.f, 1.f, -0.5f}) == za::Color::fromHSLA({0.f, 1.f, 0.f}));
    }

    SECTION("withRotatedHue modifier")
    {
        SECTION("Basic hue shifts")
        {
            // Red (0°) + 120° → Green (120°)
            za::Color red   = za::Color::Red;
            za::Color green = red.withRotatedHue(120.f);
            CHECK(green == za::Color::Green);

            // Green (120°) - 120° → Red (0°)
            za::Color greenColor = za::Color::Green;
            za::Color redColor   = greenColor.withRotatedHue(-120.f);
            CHECK(redColor == za::Color::Red);

            // Blue (240°) + 60° → 300° (Magenta)
            za::Color blue    = za::Color::Blue;
            za::Color magenta = blue.withRotatedHue(60.f);
            CHECK(magenta == za::Color::Magenta);
        }

        SECTION("Hue wrapping")
        {
            // 350° + 20° → 10° (wrapped within [0, 360))
            za::Color color        = za::Color::fromHSLA({350.f, 1.f, 0.5f});
            za::Color shiftedColor = color.withRotatedHue(20.f);
            CHECK(shiftedColor == Approx(za::Color::fromHSLA({10.f, 1.f, 0.5f})));

            // 30° - 50° → 340°
            color        = za::Color::fromHSLA({30.f, 1.f, 0.5f});
            shiftedColor = color.withRotatedHue(-50.f);
            CHECK(shiftedColor == Approx(za::Color::fromHSLA({340.f, 1.f, 0.5f})));

            // 300° + 120° → 60° (Yellow)
            color        = za::Color::fromHSLA({300.f, 1.f, 0.5f});
            shiftedColor = color.withRotatedHue(120.f);
            CHECK(shiftedColor == Approx(za::Color::fromHSLA({60.f, 1.f, 0.5f})));

            // 300° + 480° → (300 + 480) % 360 = 60°
            shiftedColor = color.withRotatedHue(480.f);
            CHECK(shiftedColor == Approx(za::Color::fromHSLA({60.f, 1.f, 0.5f})));
        }

        SECTION("Alpha preservation")
        {
            // Original alpha should remain unchanged
            za::Color color{255, 0, 0, 128};
            za::Color shiftedColor = color.withRotatedHue(120.f);
            CHECK(shiftedColor.a == 128);
        }

        SECTION("Saturation and lightness unchanged")
        {
            // After hue shift, saturation and lightness should match original
            za::Color      color        = za::Color::fromHSLA({180.f, 0.8f, 0.6f}, 255);
            za::Color      shiftedColor = color.withRotatedHue(90.f);
            za::Color::HSL hsla         = shiftedColor.toHSL();
            CHECK(hsla.saturation == Approx(0.8039f));
            CHECK(hsla.lightness == Approx(0.6f));
            CHECK(hsla.hue == Approx(270.f)); // 180° + 90°
        }

        SECTION("Edge cases")
        {
            // Shift by 0° (no change)
            za::Color color     = za::Color::Red;
            za::Color sameColor = color.withRotatedHue(0.f);
            CHECK(sameColor == color);

            // Shift by 360° (no change)
            sameColor = color.withRotatedHue(360.f);
            CHECK(sameColor == color);

            // Shift by 720° (equivalent to 0°)
            sameColor = color.withRotatedHue(720.f);
            CHECK(sameColor == color);

            // Negative shift wrapping (50° - 400° = -350° ≡ 10°)
            color                  = za::Color::fromHSLA({50.f, 1.f, 0.5f});
            za::Color shiftedColor = color.withRotatedHue(-400.f);
            CHECK(shiftedColor == za::Color::fromHSLA({10.f, 1.f, 0.5f}));
        }
    }
}
