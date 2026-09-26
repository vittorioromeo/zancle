#include "Tst/Tst.hpp"
#include "WindowUtil.hpp"

#include "Zancle/Graphics/GlyphMapping.hpp"

#include "Zancle/Graphics/FontFace.hpp"
#include "Zancle/Graphics/Glyph.hpp"
#include "Zancle/Graphics/GlyphMappedText.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/TextureAtlas.hpp"

#include "Zancle/Lifetime/LifetimeDependee.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Base/SizeT.hpp"


TEST_CASE("[Graphics] za::GlyphMapping" * tst::skip(skipDisplayTests))
{
    [[maybe_unused]] auto& graphicsContext = TST_CASE_SHARED(za::GraphicsContext::create().value());
    auto                   fontFace        = za::FontFace::openFromFile("tuffy.ttf").value();
    auto                   atlas = za::TextureAtlas(za::Texture::create({1024u, 1024u}, {.smooth = true}).value());

    static constexpr char32_t  testCodePoints[]    = U"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 ";
    static constexpr za::SizeT testCodePointsCount = (sizeof(testCodePoints) / sizeof(testCodePoints[0])) - 1u;

    SECTION("loadGlyphs basic")
    {
        auto result = fontFace.loadGlyphs(atlas,
                                          {
                                              .codePoints       = testCodePoints,
                                              .codePointCount   = testCodePointsCount,
                                              .characterSize    = 24,
                                              .bold             = false,
                                              .outlineThickness = 0.f,
                                          });

        REQUIRE(result.hasValue());

        const auto& mapping = *result;
        CHECK(mapping.characterSize == 24);
        CHECK(mapping.bold == false);
        CHECK(mapping.outlineThickness == 0.f);
        CHECK(mapping.cachedLineSpacing > 0.f);
        CHECK(mapping.cachedAscent > 0.f);
    }

    SECTION("getGlyph returns valid glyph")
    {
        auto mapping = fontFace
                           .loadGlyphs(atlas,
                                       {
                                           .codePoints       = testCodePoints,
                                           .codePointCount   = testCodePointsCount,
                                           .characterSize    = 24,
                                           .bold             = false,
                                           .outlineThickness = 0.f,
                                       })
                           .value();

        const auto& glyph = mapping.getGlyph(U'A', 24, false, 0.f);
        CHECK(glyph.advance > 0.f);
        CHECK(glyph.bounds.size.x > 0.f);
        CHECK(glyph.bounds.size.y > 0.f);
        CHECK(glyph.textureRect.size.x > 0.f);
        CHECK(glyph.textureRect.size.y > 0.f);
    }

    SECTION("getGlyph for unmapped code point returns placeholder instead of aborting")
    {
        auto mapping = fontFace
                           .loadGlyphs(atlas,
                                       {
                                           .codePoints       = testCodePoints,
                                           .codePointCount   = testCodePointsCount,
                                           .characterSize    = 24,
                                           .bold             = false,
                                           .outlineThickness = 0.f,
                                       })
                           .value();

        // U+20AC ('EUR') and U+2713 (check mark) are not in the loaded set:
        // both must resolve to the single shared `.notdef` placeholder
        const za::Glyph& fallback0 = mapping.getGlyph(U'\u20ac', 24, false, 0.f);
        const za::Glyph& fallback1 = mapping.getGlyph(U'\u2713', 24, false, 0.f);

        CHECK(&fallback0 == &fallback1);
        CHECK(&fallback0 == &mapping.fallbackFillGlyph);
    }

    SECTION("getGlyph for space has advance but no bounds")
    {
        auto mapping = fontFace
                           .loadGlyphs(atlas,
                                       {
                                           .codePoints       = testCodePoints,
                                           .codePointCount   = testCodePointsCount,
                                           .characterSize    = 24,
                                           .bold             = false,
                                           .outlineThickness = 0.f,
                                       })
                           .value();

        const auto& glyph = mapping.getGlyph(U' ', 24, false, 0.f);
        CHECK(glyph.advance > 0.f);
    }

    SECTION("loadGlyphs with outline")
    {
        auto result = fontFace.loadGlyphs(atlas,
                                          {.codePoints       = testCodePoints,
                                           .codePointCount   = testCodePointsCount,
                                           .characterSize    = 24,
                                           .bold             = false,
                                           .outlineThickness = 2.f});

        REQUIRE(result.hasValue());

        const auto& mapping = *result;
        CHECK(mapping.outlineThickness == 2.f);
        CHECK(!mapping.outlineGlyphs.empty());

        const auto glyphPair = mapping.getFillAndOutlineGlyph(U'A', 24, false, 2.f);
        CHECK(glyphPair.fillGlyph.advance > 0.f);
        CHECK(glyphPair.outlineGlyph.advance > 0.f);
        CHECK(glyphPair.outlineGlyph.bounds.size.x >= glyphPair.fillGlyph.bounds.size.x);
    }

    SECTION("getGlyph with outline thickness returns outline glyph")
    {
        auto mapping = fontFace
                           .loadGlyphs(atlas,
                                       {
                                           .codePoints       = testCodePoints,
                                           .codePointCount   = testCodePointsCount,
                                           .characterSize    = 24,
                                           .bold             = false,
                                           .outlineThickness = 2.f,
                                       })
                           .value();

        // outlineThickness=0 returns fill glyph
        const auto& fillGlyph = mapping.getGlyph(U'A', 24, false, 0.f);
        CHECK(fillGlyph.advance > 0.f);

        // outlineThickness!=0 returns outline glyph
        const auto& outlineGlyph = mapping.getGlyph(U'A', 24, false, 2.f);
        CHECK(outlineGlyph.advance > 0.f);

        // Outline glyph should be larger than fill glyph
        CHECK(outlineGlyph.bounds.size.x >= fillGlyph.bounds.size.x);
        CHECK(outlineGlyph.bounds.size.y >= fillGlyph.bounds.size.y);

        // They should have different texture rects (packed separately)
        CHECK(outlineGlyph.textureRect.position != fillGlyph.textureRect.position);
    }

    SECTION("Text with GlyphMapping and outline")
    {
        auto mapping = fontFace
                           .loadGlyphs(atlas,
                                       {
                                           .codePoints       = testCodePoints,
                                           .codePointCount   = testCodePointsCount,
                                           .characterSize    = 24,
                                           .bold             = false,
                                           .outlineThickness = 2.f,
                                       })
                           .value();

        const za::GlyphMappedText text(fontFace, atlas.getTexture(), mapping, {.string = "Outlined"});
        CHECK(text.getLocalBounds().size.x > 0.f);
        CHECK(text.getLocalBounds().size.y > 0.f);

        // Vertices should include both outline and fill quads
        const auto vertices = text.getVertices();
        CHECK(vertices.size() > 0u);
        CHECK(text.getFillVerticesStartIndex() > 0u);
        CHECK(text.getFillVerticesStartIndex() < vertices.size());
    }

    SECTION("Fill and outline glyphs for multiple characters")
    {
        auto mapping = fontFace
                           .loadGlyphs(atlas,
                                       {
                                           .codePoints       = testCodePoints,
                                           .codePointCount   = testCodePointsCount,
                                           .characterSize    = 18,
                                           .bold             = false,
                                           .outlineThickness = 3.f,
                                       })
                           .value();

        for (const char32_t cp : {U'B', U'g', U'1'})
        {
            const auto pair = mapping.getFillAndOutlineGlyph(cp, 18, false, 3.f);

            CHECK(pair.fillGlyph.advance > 0.f);
            CHECK(pair.outlineGlyph.advance > 0.f);
            CHECK(pair.outlineGlyph.bounds.size.x >= pair.fillGlyph.bounds.size.x);
        }
    }

    SECTION("GlyphMappedText getKerning delegates to FontFace")
    {
        auto mapping = fontFace
                           .loadGlyphs(atlas,
                                       {
                                           .codePoints       = testCodePoints,
                                           .codePointCount   = testCodePointsCount,
                                           .characterSize    = 24,
                                           .bold             = false,
                                           .outlineThickness = 0.f,
                                       })
                           .value();

        const za::GlyphMappedText text(fontFace, atlas.getTexture(), mapping, {.string = "AV"});

        const float kerning     = text.getKerning(U'A', U'V', 24, false);
        const float faceKerning = fontFace.getKerning(U'A', U'V', 24, false);

        CHECK(kerning == faceKerning);
    }

    SECTION("GlyphMappedText")
    {
        auto mapping = fontFace
                           .loadGlyphs(atlas,
                                       {
                                           .codePoints       = testCodePoints,
                                           .codePointCount   = testCodePointsCount,
                                           .characterSize    = 24,
                                           .bold             = false,
                                           .outlineThickness = 0.f,
                                       })
                           .value();

        const za::GlyphMappedText text(fontFace, atlas.getTexture(), mapping, {.string = "Hello World"});
        CHECK(text.getString() == "Hello World");
        CHECK(&text.getGlyphMapping() == &mapping);
        CHECK(&text.getTexture() == &atlas.getTexture());
        CHECK(text.getCharacterSize() == 24);
        CHECK(text.getOutlineThickness() == 0.f);
        CHECK(text.getLocalBounds().size.x > 0.f);
        CHECK(text.getLocalBounds().size.y > 0.f);
    }

    SECTION("atlas full returns nullOpt")
    {
        auto tinyAtlas = za::TextureAtlas(za::Texture::create({8u, 8u}, {.smooth = true}).value());

        auto result = fontFace.loadGlyphs(tinyAtlas,
                                          {
                                              .codePoints       = testCodePoints,
                                              .codePointCount   = testCodePointsCount,
                                              .characterSize    = 24,
                                              .bold             = false,
                                              .outlineThickness = 0.f,
                                          });

        CHECK(!result.hasValue());
    }

#if defined(ZA_ENABLE_LIFETIME_TRACKING)
    SECTION("Lifetime tracking")
    {
        SECTION("Return local from function")
        {
            const auto badFunction = [&]
            {
                auto localMapping = fontFace
                                        .loadGlyphs(atlas,
                                                    {
                                                        .codePoints       = testCodePoints,
                                                        .codePointCount   = testCodePointsCount,
                                                        .characterSize    = 24,
                                                        .bold             = false,
                                                        .outlineThickness = 0.f,
                                                    })
                                        .value();

                return za::GlyphMappedText(fontFace, atlas.getTexture(), localMapping, {.string = "Test"});
            };

            const za::priv::LifetimeDependee::TestingModeGuard guard{"GlyphMapping"};
            CHECK(!guard.fatalErrorTriggered("GlyphMapping"));

            badFunction();

            CHECK(guard.fatalErrorTriggered("GlyphMapping"));
        }

        SECTION("setGlyphMapping updates tracking")
        {
            auto mapping1 = fontFace
                                .loadGlyphs(atlas,
                                            {
                                                .codePoints       = testCodePoints,
                                                .codePointCount   = testCodePointsCount,
                                                .characterSize    = 24,
                                                .bold             = false,
                                                .outlineThickness = 0.f,
                                            })
                                .value();

            auto mapping2 = fontFace
                                .loadGlyphs(atlas,
                                            {
                                                .codePoints       = testCodePoints,
                                                .codePointCount   = testCodePointsCount,
                                                .characterSize    = 18,
                                                .bold             = false,
                                                .outlineThickness = 0.f,
                                            })
                                .value();

            const za::priv::LifetimeDependee::TestingModeGuard guard{"GlyphMapping"};
            CHECK(!guard.fatalErrorTriggered("GlyphMapping"));

            za::GlyphMappedText text(fontFace, atlas.getTexture(), mapping1, {.string = "Test"});
            CHECK(!guard.fatalErrorTriggered("GlyphMapping"));

            text.setGlyphMapping(fontFace, atlas.getTexture(), mapping2);
            CHECK(!guard.fatalErrorTriggered("GlyphMapping"));
        }

        SECTION("FontFace outlived by GlyphMappedText triggers error")
        {
            auto mapping = fontFace
                               .loadGlyphs(atlas,
                                           {
                                               .codePoints       = testCodePoints,
                                               .codePointCount   = testCodePointsCount,
                                               .characterSize    = 24,
                                               .bold             = false,
                                               .outlineThickness = 0.f,
                                           })
                               .value();

            const auto badFunction = [&]
            {
                auto localFontFace = za::FontFace::openFromFile("tuffy.ttf").value();
                return za::GlyphMappedText(localFontFace, atlas.getTexture(), mapping, {.string = "Test"});
            };

            const za::priv::LifetimeDependee::TestingModeGuard guard{"FontFace"};
            CHECK(!guard.fatalErrorTriggered("FontFace"));

            badFunction();

            CHECK(guard.fatalErrorTriggered("FontFace"));
        }

        SECTION("Texture outlived by GlyphMappedText triggers error")
        {
            auto mapping = fontFace
                               .loadGlyphs(atlas,
                                           {
                                               .codePoints       = testCodePoints,
                                               .codePointCount   = testCodePointsCount,
                                               .characterSize    = 24,
                                               .bold             = false,
                                               .outlineThickness = 0.f,
                                           })
                               .value();

            const auto badFunction = [&]
            {
                auto localTexture = za::Texture::create({64u, 64u}).value();
                return za::GlyphMappedText(fontFace, localTexture, mapping, {.string = "Test"});
            };

            const za::priv::LifetimeDependee::TestingModeGuard guard{"Texture"};
            CHECK(!guard.fatalErrorTriggered("Texture"));

            badFunction();

            CHECK(guard.fatalErrorTriggered("Texture"));
        }

        SECTION("Valid lifetime does not trigger errors")
        {
            auto mapping = fontFace
                               .loadGlyphs(atlas,
                                           {
                                               .codePoints       = testCodePoints,
                                               .codePointCount   = testCodePointsCount,
                                               .characterSize    = 24,
                                               .bold             = false,
                                               .outlineThickness = 0.f,
                                           })
                               .value();

            const za::priv::LifetimeDependee::TestingModeGuard fontFaceGuard{"FontFace"};
            const za::priv::LifetimeDependee::TestingModeGuard textureGuard{"Texture"};
            const za::priv::LifetimeDependee::TestingModeGuard mappingGuard{"GlyphMapping"};

            CHECK(!fontFaceGuard.fatalErrorTriggered("FontFace"));
            CHECK(!textureGuard.fatalErrorTriggered("Texture"));
            CHECK(!mappingGuard.fatalErrorTriggered("GlyphMapping"));

            {
                za::GlyphMappedText text(fontFace, atlas.getTexture(), mapping, {.string = "Test"});
                CHECK(!fontFaceGuard.fatalErrorTriggered("FontFace"));
                CHECK(!textureGuard.fatalErrorTriggered("Texture"));
                CHECK(!mappingGuard.fatalErrorTriggered("GlyphMapping"));
            }

            // Text destroyed before dependees -- no errors
            CHECK(!fontFaceGuard.fatalErrorTriggered("FontFace"));
            CHECK(!textureGuard.fatalErrorTriggered("Texture"));
            CHECK(!mappingGuard.fatalErrorTriggered("GlyphMapping"));
        }
    }
#endif
}
