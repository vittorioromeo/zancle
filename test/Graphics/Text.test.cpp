#include "SystemUtil.hpp"

#include "Zancle/Graphics/Text.hpp"

#include "Zancle/Graphics/GraphicsContext.hpp"

// Other 1st party headers
#include "GraphicsUtil.hpp"
#include "Tst/Tst.hpp"
#include "WindowUtil.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/TextData.hpp"
#include "Zancle/Graphics/TextUtils.hpp"

#include "Zancle/Lifetime/LifetimeDependee.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/IsConstructible.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


TEST_CASE("[Graphics] za::Text" * tst::skip(skipDisplayTests))
{
    [[maybe_unused]] auto& graphicsContext = TST_CASE_SHARED(za::GraphicsContext::create().value());

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::Text, za::Font&&, za::Utf8String, unsigned int));
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::Text, const za::Font&&, za::Utf8String, unsigned int));
        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(za::Text));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(za::Text));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::Text));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::Text));
    }

    const auto& font = TST_CASE_SHARED(za::Font::openFromFile("tuffy.ttf").value());

    SECTION("Construction")
    {
        SECTION("Font constructor")
        {
            const za::Text text(font, {});
            CHECK(text.getString() == "");
            CHECK(&text.getFont() == &font);
            CHECK(text.getCharacterSize() == 0);
            CHECK(text.getLetterSpacing() == 1.f);
            CHECK(text.getLineSpacing() == 1.f);
            CHECK(text.isBold() == false);
            CHECK(text.isItalic() == false);
            CHECK(text.isUnderlined() == false);
            CHECK(text.isStrikeThrough() == false);
            CHECK(text.getFillColor() == za::Color::White);
            CHECK(text.getOutlineColor() == za::Color::Black);
            CHECK(text.getOutlineThickness() == 0);
            CHECK(text.findCharacterPos(0) == za::Vec2f{});
            CHECK(text.getLocalBounds() == za::Rect2f());
            CHECK(text.getGlobalBounds() == za::Rect2f());
        }

        SECTION("Font and string constructor")
        {
            const za::Text text(font, {.string = "abcdefghijklmnopqrstuvwxyz", .characterSize = 30u});
            CHECK(text.getString() == "abcdefghijklmnopqrstuvwxyz");
            CHECK(&text.getFont() == &font);
            CHECK(text.getCharacterSize() == 30);
            CHECK(text.getLetterSpacing() == 1.f);
            CHECK(text.getLineSpacing() == 1.f);
            CHECK(text.isBold() == false);
            CHECK(text.isItalic() == false);
            CHECK(text.isUnderlined() == false);
            CHECK(text.isStrikeThrough() == false);
            CHECK(text.getFillColor() == za::Color::White);
            CHECK(text.getOutlineColor() == za::Color::Black);
            CHECK(text.getOutlineThickness() == 0);
            CHECK(text.findCharacterPos(0) == za::Vec2f{});
            CHECK(text.getLocalBounds() == za::Rect2f({1, 8}, {357, 28}));
            CHECK(text.getGlobalBounds() == za::Rect2f({1, 8}, {357, 28}));
        }

        SECTION("Font, string, and character size constructor")
        {
            const za::Text text(font, {.string = "abcdefghijklmnopqrstuvwxyz", .characterSize = 24});
            CHECK(text.getString() == "abcdefghijklmnopqrstuvwxyz");
            CHECK(&text.getFont() == &font);
            CHECK(text.getCharacterSize() == 24);
            CHECK(text.getLetterSpacing() == 1.f);
            CHECK(text.getLineSpacing() == 1.f);
            CHECK(text.isBold() == false);
            CHECK(text.isItalic() == false);
            CHECK(text.isUnderlined() == false);
            CHECK(text.isStrikeThrough() == false);
            CHECK(text.getFillColor() == za::Color::White);
            CHECK(text.getOutlineColor() == za::Color::Black);
            CHECK(text.getOutlineThickness() == 0);
            CHECK(text.findCharacterPos(0) == za::Vec2f{});
            CHECK(text.getLocalBounds() == za::Rect2f({1, 7}, {290, 22}));
            CHECK(text.getGlobalBounds() == za::Rect2f({1, 7}, {290, 22}));
        }
    }

    SECTION("Set/get string")
    {
        za::Text text(font, {});
        text.setString("abcdefghijklmnopqrstuvwxyz");
        CHECK(text.getString() == "abcdefghijklmnopqrstuvwxyz");
    }

    SECTION("Set/get font")
    {
        const auto otherFont = za::Font::openFromFile("tuffy.ttf").value();

        za::Text text(font, {});
        text.setFont(otherFont);
        CHECK(&text.getFont() == &otherFont);
    }

    SECTION("Set/get character size")
    {
        za::Text text(font, {});
        text.setCharacterSize(48);
        CHECK(text.getCharacterSize() == 48);
    }

    SECTION("Set/get line spacing")
    {
        za::Text text(font, {});
        text.setLineSpacing(42);
        CHECK(text.getLineSpacing() == 42);
    }

    SECTION("Set/get letter spacing")
    {
        za::Text text(font, {});
        text.setLetterSpacing(15);
        CHECK(text.getLetterSpacing() == 15);
    }

    SECTION("Set/get bold")
    {
        za::Text text(font, {});
        text.setBold(true);
        CHECK(text.isBold() == true);
    }

    SECTION("Set/get italic")
    {
        za::Text text(font, {});
        text.setItalic(true);
        CHECK(text.isItalic() == true);
    }

    SECTION("Set/get underlined")
    {
        za::Text text(font, {});
        text.setUnderlined(true);
        CHECK(text.isUnderlined() == true);
    }

    SECTION("Set/get strikeThrough")
    {
        za::Text text(font, {});
        text.setStrikeThrough(true);
        CHECK(text.isStrikeThrough() == true);
    }

    SECTION("Set/get fill color")
    {
        za::Text text(font, {.string = "Fill color", .characterSize = 30u});
        text.setFillColor(za::Color::Red);
        CHECK(text.getFillColor() == za::Color::Red);
    }

    SECTION("Set/get outline color")
    {
        za::Text text(font, {.string = "Outline color", .characterSize = 30u});
        text.setOutlineColor(za::Color::Green);
        CHECK(text.getOutlineColor() == za::Color::Green);
    }

    SECTION("Set/get outline thickness")
    {
        za::Text text(font, {});
        text.setOutlineThickness(3.14f);
        CHECK(text.getOutlineThickness() == 3.14f);
    }

    SECTION("findCharacterPos()")
    {
        za::Text text(font, {.string = "\tabcdefghijklmnopqrstuvwxyz \n", .characterSize = 30u});
        text.position = {120, 240};
        CHECK(text.findCharacterPos(0) == za::Vec2f{120, 240});
        CHECK(text.findCharacterPos(1) == za::Vec2f{156, 240});
        CHECK(text.findCharacterPos(2) == za::Vec2f{170, 240});
        CHECK(text.findCharacterPos(3) == za::Vec2f{185, 240});
        CHECK(text.findCharacterPos(4) == za::Vec2f{198, 240});

        // Indices that are too large are capped at maximum valid index
        CHECK(text.findCharacterPos(1000) == za::Vec2f{120, 277});
    }

    SECTION("TextUtils helpers")
    {
        using za::TextUtils::precomputeTextQuadCount;

        SECTION("precomputeTextQuadCount with no styles")
        {
            // No underline / strikethrough -> just one quad per visible glyph
            CHECK(precomputeTextQuadCount("A", false, false) == 1u);
            CHECK(precomputeTextQuadCount("Abc", false, false) == 3u);

            // Whitespace is not a glyph
            CHECK(precomputeTextQuadCount(" ", false, false) == 0u);
            CHECK(precomputeTextQuadCount("\t", false, false) == 0u);
            CHECK(precomputeTextQuadCount("A B", false, false) == 2u);
            CHECK(precomputeTextQuadCount("A\tB", false, false) == 2u);

            // Newlines are not glyphs
            CHECK(precomputeTextQuadCount("\n", false, false) == 0u);
            CHECK(precomputeTextQuadCount("A\nB", false, false) == 2u);

            // Carriage returns are skipped
            CHECK(precomputeTextQuadCount("\r", false, false) == 0u);
            CHECK(precomputeTextQuadCount("A\rB", false, false) == 2u);
        }

        SECTION("precomputeTextQuadCount with underline")
        {
            // Single line: 1 line emitted at the end + glyph quads
            CHECK(precomputeTextQuadCount("A", true, false) == 2u);
            CHECK(precomputeTextQuadCount("Abc", true, false) == 4u);

            // Whitespace-only string still gets a trailing decoration line
            CHECK(precomputeTextQuadCount(" ", true, false) == 1u);

            // Two non-empty lines -> 2 underline lines + glyphs
            CHECK(precomputeTextQuadCount("A\nB", true, false) == 4u);

            // Consecutive newlines do not double-emit
            CHECK(precomputeTextQuadCount("A\n\nB", true, false) == 4u);

            // Trailing newline (line ends empty) -> only one underline (for the "A" line)
            CHECK(precomputeTextQuadCount("A\n", true, false) == 2u);
        }

        SECTION("precomputeTextQuadCount with strikethrough")
        {
            // Same shape as underline-only, but emitted via the strikethrough branch
            CHECK(precomputeTextQuadCount("A", false, true) == 2u);
            CHECK(precomputeTextQuadCount("A\nB", false, true) == 4u);
            CHECK(precomputeTextQuadCount("A\n\nB", false, true) == 4u);
        }

        SECTION("precomputeTextQuadCount with both decorations")
        {
            // Both underline and strikethrough emit one line each per non-empty line
            CHECK(precomputeTextQuadCount("A", true, true) == 3u);
            CHECK(precomputeTextQuadCount("A\nB", true, true) == 6u);
            CHECK(precomputeTextQuadCount("A\n\nB", true, true) == 6u);
        }

        SECTION("precomputeTextQuadCount TextData overload")
        {
            // The overload returns 0 for an empty string instead of asserting
            CHECK(precomputeTextQuadCount(za::TextData{}) == 0u);

            CHECK(precomputeTextQuadCount(za::TextData{.string = "Abc", .characterSize = 18u}) == 3u);
            CHECK(precomputeTextQuadCount(za::TextData{.string = "Abc", .characterSize = 18u, .underlined = true}) == 4u);
            CHECK(precomputeTextQuadCount(za::TextData{.string = "A\nB", .characterSize = 18u, .strikeThrough = true}) ==
                  4u);
            CHECK(precomputeTextQuadCount(
                      za::TextData{.string = "A\nB", .characterSize = 18u, .underlined = true, .strikeThrough = true}) ==
                  6u);
        }

        SECTION("precomputeTextLocalBounds")
        {
            // Empty string -> empty rect (early return)
            CHECK(za::TextUtils::precomputeTextLocalBounds(font, za::TextData{}) == za::Rect2f{});

            // Matches what `za::Text::getLocalBounds` returns for the same inputs
            const za::TextData td{.string = "Test", .characterSize = 18u};
            const za::Text     text(font, td);
            CHECK(za::TextUtils::precomputeTextLocalBounds(font, td) == text.getLocalBounds());
        }

        SECTION("precomputeTextGlobalBounds")
        {
            // Empty text -> empty rect transformed is still empty rect
            CHECK(za::TextUtils::precomputeTextGlobalBounds(font, za::TextData{}) == za::Rect2f{});

            // Without any transform, global bounds equal local bounds
            const za::TextData td{.string = "Test", .characterSize = 18u};
            CHECK(za::TextUtils::precomputeTextGlobalBounds(font, td) == za::TextUtils::precomputeTextLocalBounds(font, td));

            // With a translation, global bounds shift accordingly
            za::TextData translated = td;
            translated.position     = {100.f, 200.f};

            const za::Rect2f local         = za::TextUtils::precomputeTextLocalBounds(font, translated);
            const za::Rect2f globalShifted = za::TextUtils::precomputeTextGlobalBounds(font, translated);
            CHECK(globalShifted.position == local.position + za::Vec2f{100.f, 200.f});
            CHECK(globalShifted.size == local.size);

            // Matches what `za::Text::getGlobalBounds` returns for the same inputs
            za::Text text(font, td);
            text.position = {100.f, 200.f};
            CHECK(za::TextUtils::precomputeTextGlobalBounds(font, translated) == text.getGlobalBounds());
        }

        SECTION("computeAnchorOrigin")
        {
            // For an empty string, bounds is {} so any anchor returns {0, 0}
            CHECK(za::TextUtils::computeAnchorOrigin(font, za::TextData{}, {0.5f, 0.5f}) == za::Vec2f{});

            const za::TextData td{.string = "Test", .characterSize = 18u};
            const za::Rect2f   bounds = za::TextUtils::precomputeTextLocalBounds(font, td);

            // {0, 0} -> top-left anchor (== bounds.position)
            CHECK(za::TextUtils::computeAnchorOrigin(font, td, {0.f, 0.f}) == bounds.position);

            // {1, 1} -> bottom-right anchor
            CHECK(za::TextUtils::computeAnchorOrigin(font, td, {1.f, 1.f}) == bounds.position + bounds.size);

            // {0.5, 0} -> top-center anchor (matches Rect2 helper)
            CHECK(za::TextUtils::computeAnchorOrigin(font, td, {0.5f, 0.f}) == bounds.getTopCenter());

            // {0.5, 0.5} -> center anchor (matches Rect2 helper)
            CHECK(za::TextUtils::computeAnchorOrigin(font, td, {0.5f, 0.5f}) == bounds.getCenter());
        }

        SECTION("anchored")
        {
            const za::TextData td{.position = {100.f, 200.f}, .string = "Test", .characterSize = 18u};

            // Returned TextData carries through every input field; only `origin` changes
            const za::TextData result = za::TextUtils::anchored(font, td, {0.5f, 0.5f});
            CHECK(result.position == td.position);
            CHECK(result.string == td.string);
            CHECK(result.characterSize == td.characterSize);

            // The origin matches what `computeAnchorOrigin` would return for the same inputs
            CHECK(result.origin == za::TextUtils::computeAnchorOrigin(font, td, {0.5f, 0.5f}));

            // Top-left anchor on an empty string yields the default-constructed origin
            CHECK(za::TextUtils::anchored(font, za::TextData{}, {0.f, 0.f}).origin == za::Vec2f{});
        }
    }

    SECTION("Get bounds")
    {
        za::Text text(font, {.string = "Test", .characterSize = 18u});
        text.position = {100, 200};
        CHECK(text.getLocalBounds() == za::Rect2f({1, 5}, {33, 13}));
        CHECK(text.getGlobalBounds() == za::Rect2f({101, 205}, {33, 13}));

        SECTION("Add underline")
        {
            text.setUnderlined(true);
            CHECK(text.getLocalBounds() == za::Rect2f({1, 5}, {33, 13}));
            CHECK(text.getGlobalBounds() == za::Rect2f({101, 205}, {33, 13}));
        }

        SECTION("Add strikethrough")
        {
            text.setStrikeThrough(true);
            CHECK(text.getLocalBounds() == za::Rect2f({1, 5}, {33, 13}));
            CHECK(text.getGlobalBounds() == za::Rect2f({101, 205}, {33, 13}));
        }

        SECTION("Change rotation")
        {
            text.rotation = za::degrees(180);
            CHECK(text.getLocalBounds() == za::Rect2f({1, 5}, {33, 13}));
            CHECK(text.getGlobalBounds() == Approx(za::Rect2f({66, 182}, {33, 13})));
        }
    }

#if defined(ZA_ENABLE_LIFETIME_TRACKING)
    SECTION("Lifetime tracking")
    {
        SECTION("Return local from function")
        {
            const auto badFunction = []
            {
                const auto localFont = za::Font::openFromFile("tuffy.ttf").value();
                return za::Text(localFont, {});
            };

            const za::priv::LifetimeDependee::TestingModeGuard guard{"Font"};
            CHECK(!guard.fatalErrorTriggered("Font"));

            badFunction();

            CHECK(guard.fatalErrorTriggered("Font"));
        }

        SECTION("Move struct holding both dependee and dependant")
        {
            struct BadStruct
            {
                BadStruct() : memberFont{za::Font::openFromFile("tuffy.ttf").value()}, memberText{memberFont, {}}
                {
                }

                za::Font memberFont;
                za::Text memberText;
            };

            const za::priv::LifetimeDependee::TestingModeGuard guard{"Font"};
            CHECK(!guard.fatalErrorTriggered("Font"));

            za::Optional<BadStruct> badStruct0;
            badStruct0.emplace();
            CHECK(!guard.fatalErrorTriggered("Font"));

            const BadStruct badStruct1 = ZA_MOVE(badStruct0.value());
            CHECK(!guard.fatalErrorTriggered("Font"));

            badStruct0.reset();
            CHECK(guard.fatalErrorTriggered("Font"));
        }

        SECTION("Optionals and move")
        {
            za::Optional<za::Text> t0;
            za::Optional<za::Text> t1;

            t0.emplace(za::Text{font, {}});
            t1.emplace(za::Text{font, {}});

            t1 = ZA_MOVE(t0);
            t0.reset();
        }

        SECTION("Dependee move assignment")
        {
            const za::priv::LifetimeDependee::TestingModeGuard guard{"Font"};
            CHECK(!guard.fatalErrorTriggered("Font"));

            auto sb0 = za::Font::openFromFile("tuffy.ttf").value();
            CHECK(!guard.fatalErrorTriggered("Font"));

            za::Text s0(sb0, {});
            CHECK(!guard.fatalErrorTriggered("Font"));

            sb0 = za::Font::openFromFile("tuffy.ttf").value();
            CHECK(!guard.fatalErrorTriggered("Font"));
        }

        SECTION("Copy assignment same dependee")
        {
            const za::priv::LifetimeDependee::TestingModeGuard guard{"Font"};
            CHECK(!guard.fatalErrorTriggered("Font"));

            auto     f = za::Font::openFromFile("tuffy.ttf").value();
            za::Text a(f, {});
            za::Text b(f, {});
            CHECK(!guard.fatalErrorTriggered("Font"));

            a = b;
            CHECK(!guard.fatalErrorTriggered("Font"));
        }

        SECTION("Copy assignment different dependee")
        {
            const za::priv::LifetimeDependee::TestingModeGuard guard{"Font"};
            CHECK(!guard.fatalErrorTriggered("Font"));

            auto     f1 = za::Font::openFromFile("tuffy.ttf").value();
            auto     f2 = za::Font::openFromFile("tuffy.ttf").value();
            za::Text a(f1, {});
            za::Text b(f2, {});
            CHECK(!guard.fatalErrorTriggered("Font"));

            a = b;
            CHECK(!guard.fatalErrorTriggered("Font"));
        }

        SECTION("Repeated copy assignment does not inflate count")
        {
            const za::priv::LifetimeDependee::TestingModeGuard guard{"Font"};
            CHECK(!guard.fatalErrorTriggered("Font"));

            auto     f = za::Font::openFromFile("tuffy.ttf").value();
            za::Text a(f, {});
            za::Text b(f, {});
            CHECK(!guard.fatalErrorTriggered("Font"));

            a = b;
            a = b;
            a = b;
            CHECK(!guard.fatalErrorTriggered("Font"));
        }
    }
#endif
}
