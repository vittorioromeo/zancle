#include "Aliases.hpp"
#include "BubbleIdleHelpers.hpp"
#include "BubbleIdleMain.hpp"
#include "BubbleIdleMainInline.hpp" // IWYU pragma: keep
#include "CatType.hpp"
#include "ComboState.hpp"
#include "Constants.hpp"
#include "ExampleProfiler/Profiler.hpp"
#include "FrameViewState.hpp"
#include "MainAtlasRects.hpp"
#include "MainShaders.hpp"
#include "PlayerInput.hpp"
#include "Playthrough.hpp"
#include "Profile.hpp"
#include "UIState.hpp"
#include "Version.hpp"

#include "ExampleUtils/Easing.hpp"
#include "ExampleUtils/MathUtils.hpp"
#include "ExampleUtils/Progress.hpp"

#include "Zancle/Graphics/BlendMode.hpp"
#include "Zancle/Graphics/CircleShapeData.hpp"
#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/RectangleShapeData.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/RoundedRectangleShapeData.hpp"
#include "Zancle/Graphics/Sprite.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/TextData.hpp"
#include "Zancle/Graphics/TextUtils.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/TextureAtlas.hpp"

#include "Zancle/Window/Mouse.hpp"

#include "Zancle/String/StringView.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Math/Remainder.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"

////////////////////////////////////////////////////////////
[[nodiscard]] FrameViewState Main::gameLoopComputeViews()
{
    FrameViewState out;
    out.resolution = getResolution();

    const auto screenShake = profile.enableScreenShake ? rngFast.getVec2f({-screenShakeAmount, -screenShakeAmount},
                                                                          {screenShakeAmount, screenShakeAmount})
                                                       : za::Vec2f{0.f, 0.f};

    nonScaledHUDView = {.center = out.resolution / 2.f, .size = out.resolution};
    scaledHUDView    = makeScaledHUDView(out.resolution, profile.hudScale);

    gameView                     = createScaledGameView(gameScreenSize, out.resolution);
    gameView.viewport.position.x = 0.f;
    gameView.center              = getViewCenter() + screenShake;

    const za::Vec2u backgroundResolution = gameView.size.toVec2u();
    if (rtBackground.getSize() != backgroundResolution)
        recreateBackgroundRenderTexture(backgroundResolution);

    out.scaledTopGameView        = createScaledTopGameView(gameScreenSize, out.resolution);
    out.scaledTopGameView.center = gameView.center -
                                   (gameView.viewport.position + gameView.viewport.size * 0.5f - za::Vec2f{0.5f, 0.5f})
                                       .componentWiseMul(out.scaledTopGameView.size);

    out.gameBackgroundView = {.center = getViewCenterWithoutScroll() + screenShake, .size = gameView.size};
    return out;
}

////////////////////////////////////////////////////////////
void Main::gameLoopRenderFrame(const float             deltaTimeMs,
                               const bool              shouldDrawUI,
                               const za::U8            shouldDrawUIAlpha,
                               const FrameInput&       frameInput,
                               const FrameUpdateState& frameUpdate,
                               const FrameViewState&   frameViews)
{
    rtGame.clear(za::Color::Transparent);

    gameLoopUpdateAndDrawFixedMenuBackground(deltaTimeMs, frameUpdate.elapsedUs);
    gameLoopUpdateAndDrawBackground(deltaTimeMs, frameViews.gameBackgroundView);

    bubbleDrawableBatch.clear();
    starBubbleDrawableBatch.clear();
    bombBubbleDrawableBatch.clear();

    if (profile.showBubbles)
        gameLoopDrawBubbles();

    if (profile.useBubbleShader)
        gameLoopDisplayBubblesWithShader();
    else
        gameLoopDisplayBubblesWithoutShader();

    cpuCloudDrawableBatch.clear();

    {
        za::Vec2f p_min{pt->getMapLimit(), 0.f};
        za::Vec2f p_max{pt->getMapLimit() + frameViews.resolution.x, frameViews.resolution.y};

        cpuCloudDrawableBatch.add(za::RectangleShapeData{
            .position  = p_min.addX(10.f),
            .fillColor = za::Color::White,
            .size      = p_max - p_min,
        });

        p_min.x += 40.f;

        drawCloudFrame({
            .time              = shaderTime,
            .mins              = p_min,
            .maxs              = p_max,
            .xSteps            = 6,
            .ySteps            = 12,
            .scaleMult         = 4.f,
            .outwardOffsetMult = 1.f,
            .batch             = &cpuCloudDrawableBatch,
        });
    }

    gameLoopDrawCursorTrail(frameInput.mousePos);
    gameLoopDrawMinimapIcons();

    cpuTopCloudDrawableBatch.clear();
    cpuDrawableBatchBeforeCats.clear();
    cpuDrawableBatch.clear();
    cpuDrawableBatchAfterCats.clear();
    cpuDrawableBatchAdditive.clear();
    cpuTopDrawableBatch.clear();
    catTextDrawableBatch.clear();
    catTextTopDrawableBatch.clear();
    cpuCloudHudDrawableBatch.clear();
    hexedCatDrawCommands.clear();

    if (pt->multiPopEnabled && playerInputState.draggedCats.empty())
    {
        const auto range = pt->psvPPMultiPopRange.currentValue() * 0.9f;

        cpuDrawableBatchBeforeCats.add(za::CircleShapeData{
            .position           = frameInput.mousePos,
            .origin             = {range, range},
            .outlineTextureRect = atlasRects.txrWhiteDot,
            .fillColor          = za::Color::Transparent,
            .outlineColor       = (outlineHueColor.withAlpha(105u).withLightness(0.75f)),
            .outlineThickness   = 1.5f,
            .radius             = range,
            .pointCount         = static_cast<unsigned int>(range / 2.f),
        });
    }

    gameLoopDrawEvents();
    gameLoopDrawHellPortals();
    gameLoopDrawCats(frameInput.mousePos, deltaTimeMs);
    gameLoopDrawShrines(frameInput.mousePos);
    gameLoopDrawDolls(frameInput.mousePos);
    gameLoopDrawParticles();
    gameLoopDrawTextParticles();
    gameLoopDisplayCloudBatch(cpuCloudDrawableBatch, gameView);
    drawBatch(cpuDrawableBatchBeforeCats,
              {.view = gameView, .texture = &textureAtlas.getTexture(), .shader = &shaders.shader});
    drawBatch(cpuDrawableBatch, {.view = gameView, .texture = &textureAtlas.getTexture(), .shader = &shaders.shader});
    drawHexedCatDrawCommands(gameView, /* top */ false);
    gameLoopDrawComboBubbleBurstingCoins();
    drawBatch(cpuDrawableBatchAfterCats,
              {.view = gameView, .texture = &textureAtlas.getTexture(), .shader = &shaders.shader});
    drawBatch(cpuDrawableBatchAdditive,
              {.blendMode = za::BlendAdd, .view = gameView, .texture = &textureAtlas.getTexture(), .shader = &shaders.shader});
    drawBatch(catTextDrawableBatch, {.view = gameView, .texture = &textureAtlas.getTexture(), .shader = &shaders.shader});

    gameLoopDrawScrollArrowHint(deltaTimeMs);

    if (const auto dragRect = getAoEDragRect(frameInput.mousePos); dragRect.hasValue())
        rtGame.draw(za::RectangleShapeData{.position         = dragRect->position,
                                           .origin           = {0.f, 0.f},
                                           .fillColor        = za::Color::whiteWithAlpha(64u),
                                           .outlineColor     = za::Color::whiteWithAlpha(176u),
                                           .outlineThickness = 4.f,
                                           .size             = dragRect->size},
                    {.view = gameView});

    const float yBelowMinimap = pt->mapPurchased ? (boundaries.y / profile.minimapScale) + 12.f : 0.f;

    if (shouldDrawUI)
    {
        za::Vec2f mins{10.f, 10.f};
        za::Vec2f maxs{20.f + static_cast<float>(moneyText.getString().byteSize()) * 15.f, pt->comboPurchased ? 60.f : 40.f};

        mins.y += yBelowMinimap;
        maxs.y += yBelowMinimap;

        drawCloudFrame({
            .time              = shaderTime,
            .mins              = mins,
            .maxs              = maxs,
            .xSteps            = 16,
            .ySteps            = 8,
            .scaleMult         = 1.f,
            .outwardOffsetMult = 1.f,
            .batch             = &cpuCloudHudDrawableBatch,
        });

        hudBottomDrawableBatch.clear();
        gameLoopDrawHUDBottomParticles();
        drawBatch(hudBottomDrawableBatch,
                  {.view = nonScaledHUDView, .texture = &textureAtlas.getTexture(), .shader = &shaders.shader});
    }

    if (shouldDrawUI)
    {
        hudDrawableBatch.clear();

        if (!uiState.debugHideUI)
            gameLoopDrawHUDParticles();

        gameLoopDrawEarnedCoinParticles();
        drawBatch(hudDrawableBatch,
                  {.view = scaledHUDView, .texture = &textureAtlas.getTexture(), .shader = &shaders.shader});
    }

    if constexpr (isDemoVersion)
    {
        const float xStartOverlay = gameView.viewport.size.x * frameViews.resolution.x / profile.hudScale;

        demoText.setGlobalTopRight({xStartOverlay - 15.f, 15.f});
        demoText.setOutlineColor(outlineHueColor);
        rtGame.draw(demoText, {.view = scaledHUDView});

        za::TextData demoInfoTextData{.position         = {},
                                      .string           = "",
                                      .characterSize    = 24u,
                                      .fillColor        = za::Color::White,
                                      .outlineColor     = outlineHueColor,
                                      .outlineThickness = 2.f};

        const float lineSpacing = fontSuperBakery.getLineSpacing(demoInfoTextData.characterSize);

        const za::StringView lines[3] = {"Only one prestige and two shrines",
                                         "Full version available on Steam",
                                         "Your progress will carry over!"};

        for (za::SizeT i = 0u; i < 3u; ++i)
        {
            demoInfoTextData.string = lines[i].data();
            demoInfoTextData.position = demoText.getGlobalBottomRight().addY(10.f + (static_cast<float>(i) * lineSpacing));

            rtGame.draw(fontSuperBakery,
                        za::TextUtils::anchored(fontSuperBakery, demoInfoTextData, {1.f, 0.f}),
                        {.view = scaledHUDView});
        }
    }

    gameLoopUpdateMoneyText(deltaTimeMs, yBelowMinimap);
    gameLoopUpdateSpentMoneyEffect(deltaTimeMs);

    za::TextData     comboTextData   = gameLoopUpdateComboText(deltaTimeMs, yBelowMinimap);
    const za::Rect2f comboTextBounds = pt->comboPurchased
                                           ? za::TextUtils::precomputeTextGlobalBounds(fontSuperBakery, comboTextData)
                                           : za::Rect2f{};

    if (isDevilcatHellsingedActive() && pt->buffCountdownsPerType[asIdx(CatType::Devil)].time > 0.f)
    {
        if (portalStormTimer.tickLooping(deltaTimeMs, 10.f) == LoopResult::Looped &&
            rng.getF(0.f, 100.f) <= pt->psvPPDevilRitualBuffPercentage.currentValue())
        {
            const float offset    = 64.f;
            const auto  portalPos = rng.getVec2f({offset, offset}, {pt->getMapLimit() - offset, boundaries.y - offset});

            pt->hellPortals.pushBack({
                .position = portalPos,
                .life     = Countdown{.time = 1750.f},
                .catIdx   = 100'000u,
            });

            sounds.makeBomb.settings.position = {portalPos.x, portalPos.y};
            playSound(sounds.portalon);
        }
    }

    za::TextData     buffTextData   = gameLoopUpdateBuffText(comboTextBounds);
    const za::Rect2f buffTextBounds = za::TextUtils::precomputeTextGlobalBounds(fontSuperBakery, buffTextData);

    if (!buffTextData.string.empty())
    {
        const za::Vec2f offset{10.f, 10.f};

        auto mins = buffTextBounds.position + offset;
        auto maxs = buffTextBounds.position + buffTextBounds.size - offset - za::Vec2{0.f, 20.f};

        mins = (mins.toVec2i() / 20 * 20).toVec2f();
        maxs = (maxs.toVec2i() / 20 * 20).toVec2f().addX(20.f).addY(5.f);

        drawCloudFrame({
            .time              = shaderTime,
            .mins              = mins,
            .maxs              = maxs,
            .xSteps            = 16,
            .ySteps            = 8,
            .scaleMult         = 1.f,
            .outwardOffsetMult = 1.f,
            .batch             = &cpuCloudHudDrawableBatch,
        });
    }

    if (shouldDrawUI && !uiState.debugHideUI)
        if (comboState.comboCountdown.time > 25.f)
            rtGame.draw(
                za::RoundedRectangleShapeData{
                    .position     = {comboTextBounds.position.x + comboTextBounds.size.x + 3.f, yBelowMinimap + 51.f},
                    .fillColor    = za::Color{75, 75, 75, 255},
                    .size         = {100.f * comboState.comboCountdown.time / 700.f, 20.f},
                    .cornerRadius = 6.f,
                },
                {.view = scaledHUDView});

    if (!uiState.debugHideUI && pt->mapPurchased)
        drawMinimap(/* back */ true, rtGame, scaledHUDView, frameViews.resolution, shouldDrawUIAlpha);

    gameLoopDisplayCloudBatch(cpuCloudUiDrawableBatch, nonScaledHUDView);
    gameLoopDisplayCloudBatch(cpuCloudHudDrawableBatch, scaledHUDView);

    if (!uiState.debugHideUI && pt->mapPurchased)
    {
        drawMinimap(/* back */ false, rtGame, scaledHUDView, frameViews.resolution, shouldDrawUIAlpha);

        const auto p = scaledHUDView.screenToWorld(frameInput.windowSpaceMouseOrFingerPos.toVec2f(),
                                                   window.getSize().toVec2f());

        if (uiState.minimapRect.contains(p) && !uiState.minimapZoomButtonsRect.contains(p) &&
            mBtnDown(za::Mouse::Button::Left, /* penetrateUI */ true))
        {
            const auto minimapPos   = p - uiState.minimapRect.position;
            playerInputState.scroll = minimapPos.x * 0.5f * pt->getMapLimit() / uiState.minimapRect.size.x -
                                      gameView.size.x * 0.25f;
        }
    }

    if (!uiState.debugHideUI)
    {
        moneyText.setFillColorAlpha(shouldDrawUIAlpha);
        moneyText.setOutlineColorAlpha(shouldDrawUIAlpha);
        rtGame.draw(moneyText, {.view = scaledHUDView});
    }

    if (!uiState.debugHideUI && pt->comboPurchased)
    {
        comboTextData.fillColor.a    = shouldDrawUIAlpha;
        comboTextData.outlineColor.a = shouldDrawUIAlpha;
        rtGame.draw(fontSuperBakery, comboTextData, {.view = scaledHUDView});
    }

    if (!uiState.debugHideUI)
    {
        buffTextData.fillColor.a    = shouldDrawUIAlpha;
        buffTextData.outlineColor.a = shouldDrawUIAlpha;
        rtGame.draw(fontSuperBakery, buffTextData, {.view = scaledHUDView});
    }

    gameLoopDrawImGui(shouldDrawUIAlpha);

    gameLoopDisplayCloudBatch(cpuTopCloudDrawableBatch, frameViews.scaledTopGameView);
    drawBatch(cpuTopDrawableBatch,
              {.view = frameViews.scaledTopGameView, .texture = &textureAtlas.getTexture(), .shader = &shaders.shader});
    drawHexedCatDrawCommands(frameViews.scaledTopGameView, /* top */ true);
    drawBatch(catTextTopDrawableBatch,
              {.view = frameViews.scaledTopGameView, .texture = &textureAtlas.getTexture(), .shader = &shaders.shader});

    if (shouldDrawUI)
        gameLoopUpdatePurchaseUnlockedEffects(deltaTimeMs);

    if (shouldDrawUI)
    {
        hudTopDrawableBatch.clear();
        gameLoopDrawHUDTopParticles();
        drawBatch(hudTopDrawableBatch,
                  {.view = nonScaledHUDView, .texture = &textureAtlas.getTexture(), .shader = &shaders.shader});
    }

    gameLoopDrawCursor(deltaTimeMs, frameUpdate.cursorGrow);
    gameLoopDrawCursorComboText(deltaTimeMs, frameUpdate.cursorGrow);
    gameLoopDrawCursorComboBar();

    if (splashCountdown.time > 0.f)
        drawSplashScreen(rtGame, scaledHUDView, frameViews.resolution, profile.hudScale);

    if (victoryTC.hasValue())
    {
        if (victoryTC->tick(deltaTimeMs) == TickResult::JustFinished)
        {
            cdLetterAppear.time = 4000.f;
            delayedActions.emplaceBack(Countdown{.time = 4000.f}, [this] { playSound(sounds.paper); });
        }

        if (victoryTC->isDone())
        {
            if (cdLetterAppear.tick(deltaTimeMs) == TickResult::JustFinished)
                cdLetterText.time = 10'000.f;

            const float progress = cdLetterAppear.asProgress(4000.f).getBounce();

            rtGame.draw(za::Sprite{.position    = frameViews.resolution / 2.f / profile.hudScale,
                                   .scale       = za::Vec2f{0.9f, 0.9f} * (0.35f + 0.65f * easeInOutQuint(progress)) /
                                                  profile.hudScale * 2.f,
                                   .origin      = txLetter.getSize().toVec2f() / 2.f,
                                   .textureRect = txLetter.getRect(),
                                   .color = za::Color::whiteWithAlpha(static_cast<U8>(easeInOutQuint(progress) * 255.f))},
                        {.view = scaledHUDView, .texture = &txLetter});
        }

        (void)cdLetterText.tick(deltaTimeMs);

        const float textProgress = cdLetterText.time > 9000.f   ? remap(cdLetterText.time, 9000.f, 10'000.f, 1.f, 0.f)
                                   : cdLetterText.time < 1000.f ? cdLetterText.time / 1000.f
                                                                : 1.f;

        rtGame.draw(za::Sprite{.position    = frameViews.resolution / 2.f / profile.hudScale,
                               .scale       = za::Vec2f{0.9f, 0.9f} * (0.35f + 0.65f * easeInOutQuint(textProgress)) /
                                              profile.hudScale * 1.45f,
                               .origin      = txLetterText.getSize().toVec2f() / 2.f,
                               .textureRect = txLetterText.getRect(),
                               .color = za::Color::whiteWithAlpha(static_cast<U8>(easeInOutQuint(textProgress) * 255.f))},
                    {.view = scaledHUDView, .texture = &txLetterText});
    }

    if (!pt->hexSessions.empty())
        gameLoopDrawDollParticleBorder(0.f);

    if (!pt->copyHexSessions.empty())
        gameLoopDrawDollParticleBorder(180.f);

    gameLoopTips(deltaTimeMs);
    gameLoopPrestigeAvailableReminder();
    gameLoopReminderBuyCombo();
    gameLoopReminderSpendPPs();
    gameLoopUpdateDpsSampler(frameUpdate.elapsedUs);

    if (shouldDrawUI)
        gameLoopUpdateNotificationQueue(deltaTimeMs);
}

////////////////////////////////////////////////////////////
void Main::gameLoopPresentFrame(const FrameViewState& frameViews)
{
    if (flushBeforeDisplay)
        rtGame.invokeGlFlush();

    if (finishBeforeDisplay)
        rtGame.invokeGlFinish();

    rtGame.display();

    if (flushAfterDisplay)
        rtGame.invokeGlFlush();

    if (finishAfterDisplay)
        rtGame.invokeGlFinish();

    window.clear();

    auto gameViewNoScroll   = gameView;
    gameViewNoScroll.center = getViewCenterWithoutScroll();

    {
        const float ratio         = frameViews.resolution.x / 1250.f;
        const float fixedBgScroll = txFixedBg.getSize().toVec2f().x * 0.5f * za::truncatedRemainder(fixedBgSlide, 3.f);

        window.draw(txFixedBg,
                    {.position    = {0.f, 0.f},
                     .scale       = {ratio, ratio},
                     .textureRect = {{fixedBgScroll - playerInputState.actualScroll / 20.f, 0.f},
                                     {frameViews.resolution.x / ratio, frameViews.resolution.y / ratio}},
                     .color       = za::Color::White},
                    {.view = nonScaledHUDView});
    }

    window.draw(rtBackgroundProcessed.getTexture(),
                {.textureRect{{0.f, 0.f}, gameViewNoScroll.size}},
                {.view = gameViewNoScroll});

    drawActivatedShrineBackgroundEffects(window, gameViewNoScroll, gameView.center);

    setPostProcessUniforms(profile.ppSVibrance,
                           profile.ppSSaturation,
                           profile.ppSLightness,
                           profile.ppSSharpness,
                           profile.ppSBlur);

    constexpr za::BlendMode premultipliedAlphaBlend(za::BlendMode::Factor::One,
                                                    za::BlendMode::Factor::OneMinusSrcAlpha,
                                                    za::BlendMode::Equation::Add,
                                                    za::BlendMode::Factor::One,
                                                    za::BlendMode::Factor::OneMinusSrcAlpha,
                                                    za::BlendMode::Equation::Add);

    window.draw(rtGame.getTexture(), {.blendMode = premultipliedAlphaBlend, .shader = &shaders.shaderPostProcess});

    if (flushBeforeDisplay)
        rtGame.invokeGlFlush();

    if (finishBeforeDisplay)
        rtGame.invokeGlFinish();

    {
        SFEX_PROFILE_SCOPE("window.display()");
        window.display();
    }

    if (flushAfterDisplay)
        rtGame.invokeGlFlush();

    if (finishAfterDisplay)
        rtGame.invokeGlFinish();
}
