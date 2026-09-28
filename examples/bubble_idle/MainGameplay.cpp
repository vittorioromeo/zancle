

#include "Aliases.hpp"
#include "BGMStorage.hpp"
#include "Bubble.hpp"
#include "BubbleIdleMain.hpp"
#include "BubbleIdleMainInline.hpp" // IWYU pragma: keep
#include "BubbleType.hpp"
#include "Cat.hpp"
#include "CatType.hpp"
#include "Collision.hpp"
#include "ComboState.hpp"
#include "Constants.hpp"
#include "Doll.hpp"
#include "ExampleProfiler/Profiler.hpp"
#include "HellPortal.hpp"
#include "HexSession.hpp"
#include "MainShaders.hpp"
#include "ParticleData.hpp"
#include "ParticleType.hpp"
#include "Playthrough.hpp"
#include "Profile.hpp"
#include "Shrine.hpp"
#include "ShrineType.hpp"
#include "SweepAndPrune.hpp"

#include "ExampleUtils/ControlFlow.hpp"
#include "ExampleUtils/Easing.hpp"
#include "ExampleUtils/HueColor.hpp"
#include "ExampleUtils/MathUtils.hpp"
#include "ExampleUtils/Progress.hpp"
#include "ExampleUtils/SoundManager.hpp"

#include "Zancle/Audio/Listener.hpp"
#include "Zancle/Audio/PlaybackDevice.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/TextData.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/Fmt/Fmt.hpp"
#include "Zancle/Fmt/FmtNumeric.hpp"

#include "Zancle/IO/IO.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/ToString.hpp"

#include "Zancle/Algorithm/AnyOf.hpp"
#include "Zancle/Algorithm/Count.hpp"
#include "Zancle/Algorithm/Erase.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/Ceil.hpp"
#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/MinMax.hpp"
#include "Zancle/Math/Pow.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/GetArraySize.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"

#include <climits>
#include <cstdio>

////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCombo(const float                   deltaTimeMs,
                               const bool                    anyBubblePoppedByClicking,
                               const za::Vec2f               mousePos,
                               const za::Optional<za::Vec2f> clickPosition)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    // Mousecat combo
    checkComboEnd(deltaTimeMs, pt->mouseCatCombo, pt->mouseCatComboCountdown);

    // Combo failure countdown for red text effect
    (void)comboState.comboFailCountdown.tick(deltaTimeMs);

    // Player combo data
    const auto playerLastCombo      = comboState.combo;
    bool       playerJustEndedCombo = false;

    // Player combo failure due to timer end
    if (checkComboEnd(deltaTimeMs, comboState.combo, comboState.comboCountdown))
        playerJustEndedCombo = true;


    // Player combo failure due to missed click
    if (!anyBubblePoppedByClicking && clickPosition.hasValue() && !pt->laserPopEnabled)
    {
        if (comboState.combo > 1)
        {
            playSound(sounds.scratch);
            comboState.comboFailCountdown.time = 250.f;
        }

        comboState.combo               = 0;
        comboState.comboCountdown.time = 0.f;

        playerJustEndedCombo = true;
    }

    if (playerJustEndedCombo)
    {
        if (playerLastCombo > 2)
        {
            comboState.comboAccReward = static_cast<int>(za::pow(static_cast<float>(comboState.comboNOthers), 1.25f));
            comboState.comboAccStarReward = comboState.comboNStars;
        }
        else
        {
            comboState.comboAccReward     = 0;
            comboState.comboAccStarReward = 0;
        }

        comboState.iComboAccReward     = 0;
        comboState.iComboAccStarReward = 0;
        comboState.comboNStars         = 0;
        comboState.comboNOthers        = 0;
    }

    if (profile.accumulatingCombo)
    {
        if (comboState.iComboAccReward < comboState.comboAccReward &&
            comboState.accComboDelay.tickLooping(deltaTimeMs, 35.f) == LoopResult::Looped)
        {
            ++comboState.iComboAccReward;
            comboState.accComboDelay.time = 35.f;

            if (spawnEarnedCoinParticle(fromWorldToHud(mousePos)))
            {
                earnedCoinParticles.back().startPosition += rngFast.getVec2f({-25.f, -25.f}, {25.f, 25.f});

                const za::Vec2f viewSize           = getCurrentGameViewSize();
                const za::Vec2f viewCenter         = getViewCenter();
                sounds.coindelay.settings.position = {viewCenter.x - viewSize.x / 2.f + 25.f,
                                                      viewCenter.y - viewSize.y / 2.f + 25.f};
                sounds.coindelay.settings.pitch    = 0.8f + static_cast<float>(comboState.iComboAccReward) * 0.04f;
                sounds.coindelay.settings.volume   = profile.sfxVolume / 100.f;

                playSound(sounds.coindelay, /* maxOverlap */ 64);
            }
        }

        if (comboState.iComboAccStarReward < comboState.comboAccStarReward &&
            comboState.accComboStarDelay.tickLooping(deltaTimeMs, 75.f) == LoopResult::Looped)
        {
            ++comboState.iComboAccStarReward;

            sounds.shine3.settings.position = {mousePos.x, mousePos.y};
            sounds.shine3.settings.pitch    = 0.75f + static_cast<float>(comboState.iComboAccStarReward) * 0.075f;
            playSound(sounds.shine3);

            spawnParticle(ParticleData{.position      = mousePos,
                                       .velocity      = {0.f, 0.f},
                                       .scale         = rngFast.getF(0.08f, 0.27f) * 1.f,
                                       .scaleDecay    = 0.f,
                                       .accelerationY = -0.002f,
                                       .opacity       = 1.f,
                                       .opacityDecay  = rngFast.getF(0.00025f, 0.002f),
                                       .rotation      = rngFast.getF(0.f, za::tau),
                                       .torque        = rngFast.getF(-0.002f, 0.002f)},
                          0.f,
                          ParticleType::Star);
        }
    }
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCollisionsBubbleBubble(const float deltaTimeMs)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    auto func = [&] [[gnu::always_inline]] (const SizeT bubbleIdxI, const SizeT bubbleIdxJ)
    {
        // TODO P2: technically this is a data race
        handleBubbleCollision(deltaTimeMs, pt->bubbles[bubbleIdxI], pt->bubbles[bubbleIdxJ]);
    };

    sweepAndPrune->forEachUniqueIndexPair(threadPool, func);
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCollisionsCatCat(const float deltaTimeMs)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    for (SizeT i = 0u; i < pt->cats.size(); ++i)
        for (SizeT j = i + 1; j < pt->cats.size(); ++j)
        {
            Cat& iCat = pt->cats[i];
            Cat& jCat = pt->cats[j];

            if (isCatBeingDragged(iCat) || isCatBeingDragged(jCat))
                continue;

            const auto applyAstroInspireAndIgnore = [this](Cat& catA, Cat& catB)
            {
                if (!catA.isAstroAndInFlight())
                    return false;

                if (pt->perm.astroCatInspirePurchased && catB.type != CatType::Astro &&
                    detectCollision(catA.position, catB.position, catA.getRadius(), catB.getRadius()))
                {
                    catB.inspiredCountdown.time = pt->getComputedInspirationDuration();

                    pt->achAstrocatInspireByType[asIdx(catB.type)] = true;
                }

                return true;
            };

            if (applyAstroInspireAndIgnore(iCat, jCat))
                continue;

            // NOLINTNEXTLINE(readability-suspicious-call-argument)
            if (applyAstroInspireAndIgnore(jCat, iCat))
                continue;

            handleCatCollision(deltaTimeMs, pt->cats[i], pt->cats[j]);
        }
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCollisionsCatShrine(const float deltaTimeMs) const
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    for (Cat& cat : pt->cats)
    {
        if (cat.isAstroAndInFlight())
            continue;

        if (isCatBeingDragged(cat))
            continue;

        for (Shrine& shrine : pt->shrines)
            handleCatShrineCollision(deltaTimeMs, cat, shrine);
    }
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCollisionsCatDoll()
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    const auto checkCollisionWithDoll = [&](Doll& d, auto collectFn)
    {
        for (const Cat& cat : pt->cats)
        {
            if (!cat.isAstroAndInFlight())
                continue;

            if (pt->perm.witchCatBuffOrbitalDolls && d.isActive() && !d.tcDeath.hasValue() &&
                detectCollision(cat.position, d.position, cat.getRadius(), d.getRadius()))
            {
                collectFn(d);
            }
        }
    };

    for (HexSession& session : pt->hexSessions)
        for (Doll& doll : session.dolls)
            checkCollisionWithDoll(doll, [&](Doll& d) { collectDoll(d, session); });

    for (HexSession& session : pt->copyHexSessions)
        for (Doll& copyDoll : session.dolls)
            checkCollisionWithDoll(copyDoll, [&](Doll& d) { collectCopyDoll(d, session); });
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCollisionsBubbleHellPortal()
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    if (!frameProcThisFrame)
        return;

    const float hellPortalRadius        = pt->getComputedRangeByCatType(CatType::Devil) * 1.25f;
    const float hellPortalRadiusSquared = hellPortalRadius * hellPortalRadius;

    for (const HellPortal& hellPortal : pt->hellPortals)
    {
        Cat* linkedCat = hellPortal.catIdx < pt->cats.size() ? &pt->cats[hellPortal.catIdx] : nullptr;

        forEachBubbleInRadiusSquared(hellPortal.position,
                                     hellPortalRadiusSquared,
                                     [&](Bubble& bubble)
        {
            if (bubble.type == BubbleType::Combo)
                return ControlFlow::Continue;

            const MoneyType reward = computeFinalReward(/* bubble     */ bubble,
                                                        /* multiplier */ 50.f,
                                                        /* comboMult  */ 1.f,
                                                        /* popperCat  */ linkedCat);

            statHellPortalRevenue(reward);

            popWithRewardAndReplaceBubble({
                .reward          = reward,
                .bubble          = bubble,
                .xCombo          = 1,
                .popSoundOverlap = rngFast.getF(0.f, 1.f) > 0.75f,
                .popperCat       = linkedCat,
                .multiPop        = false,
            });

            if (linkedCat != nullptr)
                linkedCat->textStatusShakeEffect.bump(rngFast, 1.5f);

            return ControlFlow::Continue;
        });
    }
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateScreenShake(const float deltaTimeMs)
{
    if (screenShakeTimer > 0.f)
    {
        screenShakeTimer -= deltaTimeMs;
        screenShakeTimer = za::max(0.f, screenShakeTimer);
    }

    const bool anyShrineDying = za::anyOf(pt->shrines.begin(), pt->shrines.end(), [](const Shrine& shrine) {
        return shrine.tcDeath.hasValue();
    });

    if (!anyShrineDying && screenShakeTimer <= 0.f && screenShakeAmount > 0.f)
    {
        screenShakeAmount -= deltaTimeMs * 0.05f;
        screenShakeAmount = za::max(0.f, screenShakeAmount);
    }
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateParticlesAndTextParticles(const float deltaTimeMs)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    const auto resolution = getResolution();

    const auto updateParticleLike = [&](auto& particleLikeVec)
    {
        for (auto& p : particleLikeVec)
        {
            p.velocity.y += p.accelerationY * deltaTimeMs;
            p.position += p.velocity * deltaTimeMs;

            p.rotation += p.torque * deltaTimeMs;

            p.opacity = za::clamp(p.opacity - p.opacityDecay * deltaTimeMs, 0.f, 1.f);
            p.scale   = za::max(p.scale - p.scaleDecay * deltaTimeMs, 0.f);
        }

        za::vectorEraseIf(particleLikeVec, [](const auto& particleLike) { return particleLike.opacity <= 0.f; });
    };

    updateParticleLike(particles);
    updateParticleLike(spentCoinParticles);
    updateParticleLike(hudTopParticles);
    updateParticleLike(hudBottomParticles);
    updateParticleLike(textParticles);

    za::vectorEraseIf(spentCoinParticles,
                      [&](const auto& p)
    {
        return p.type == ParticleType::Coin &&
               (p.position.x > (gameView.viewport.size.x * resolution.x) || p.position.x < 0.f);
    });

    for (auto& earnedCoinParticle : earnedCoinParticles)
        (void)earnedCoinParticle.progress.advance(deltaTimeMs * 0.0015f);

    za::vectorEraseIf(earnedCoinParticles, [&](const auto& p) { return p.progress.isAtEnd(); });
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateSounds(const float deltaTimeMs, const za::Vec2f mousePos)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

#ifndef BUBBLEBYTE_NO_AUDIO
    const float volumeMult = profile.playAudioInBackground || window.hasFocus() ? 1.f : 0.f;

    listener.position = {za::clamp(mousePos.x, 0.f, pt->getMapLimit()), za::clamp(mousePos.y, 0.f, boundaries.y), 0.f};

    listener.volume = profile.masterVolume / 100.f * volumeMult;

    (void)playbackDevice.applyListener(listener);

    auto& optCurrentMusic = getCurrentBGMBuffer();
    auto& optNextMusic    = getNextBGMBuffer();

    if (!bgmTransition.isDone())
    {
        ZA_ASSERT(optNextMusic.hasValue());

        const auto processMusic = [&](za::Optional<BGMBuffer>& optMusic, const float transitionMult)
        {
            if (!optMusic.hasValue())
                return;

            optMusic->music.setPosition(listener.position);
            optMusic->music.setVolume(profile.musicVolume / 100.f * volumeMult * transitionMult);

            if (soundManager.countPlayingPooled(sounds.prestige) > 0u)
                optMusic->music.setVolume(0.f);
        };

        processMusic(optCurrentMusic, bgmTransition.asProgress(1000.f).getRemaining());
        processMusic(optNextMusic, bgmTransition.asProgress(1000.f).getElapsed());

        if (bgmTransition.tick(deltaTimeMs) == TickResult::JustFinished)
        {
            optCurrentMusic.reset();
            ++currentBGMBufferIdx;
        }
    }
    else
    {
        const auto processMusic = [&](za::Optional<BGMBuffer>& optMusic)
        {
            if (!optMusic.hasValue())
                return;

            optMusic->music.setPosition(listener.position);
            optMusic->music.setVolume(profile.musicVolume / 100.f * volumeMult);

            if (soundManager.countPlayingPooled(sounds.prestige) > 0u)
                optMusic->music.setVolume(0.f);
        };

        processMusic(optCurrentMusic);
        processMusic(optNextMusic);
    }
#else
    (void)deltaTimeMs;
    (void)mousePos;
#endif
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateTimePlayed(const za::I64 elapsedUs)
{
    playedUsAccumulator += elapsedUs;

    while (playedUsAccumulator > 1'000'000)
    {
        playedUsAccumulator -= 1'000'000;
        statSecondsPlayed();
    }
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateAutosave(const za::I64 elapsedUs)
{
    if (inSpeedrunPlaythrough())
        return;

    autosaveUsAccumulator += elapsedUs;

    if (autosaveUsAccumulator >= 180'000'000) // 3 min
    {
        autosaveUsAccumulator = 0;
        za::printLn("Autosaving...");
        saveMainPlaythroughToFile();
    }
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateAndDrawFixedMenuBackground(const float deltaTimeMs, const za::I64 elapsedUs)
{
    fixedBgSlideAccumulator += elapsedUs;

    if (fixedBgSlideAccumulator > 60'000'000) // change slide every 60s
    {
        fixedBgSlideAccumulator = 0;

        fixedBgSlideTarget += 1.f;

        if (fixedBgSlideTarget >= 3.f)
            fixedBgSlideTarget = 0.f;
    }

    fixedBgSlide = exponentialApproach(fixedBgSlide, fixedBgSlideTarget, deltaTimeMs, 1000.f);
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateAndDrawBackground(const float deltaTimeMs, const za::View& gameBackgroundView)
{
    static float backgroundScroll = 0.f;
    backgroundScroll += deltaTimeMs * 0.01f;

    rtBackground.clear(outlineHueColor);

    const auto getAlpha = [&](const float mult) { return static_cast<za::U8>(profile.backgroundOpacity / 100.f * mult); };

    ////////////////////////////////////////////////////////////
    const za::Texture* const chunkTx[] = {
        &txBackgroundChunk,            // Normal
        &txBackgroundChunkDesaturated, // Voodoo
        &txBackgroundChunk,            // Magic
        &txBackgroundChunkDesaturated, // Clicking
        &txBackgroundChunk,            // Automation
        &txBackgroundChunk,            // Repulsion
        &txBackgroundChunkDesaturated, // Attraction
        &txBackgroundChunk,            // Camouflage
        &txBackgroundChunk,            // Victory
    };

    static_assert(za::getArraySize(chunkTx) == nShrineTypes + 1u);

    ////////////////////////////////////////////////////////////
    const za::Texture* const detailTx[] = {
        &txClouds,          // Normal
        &txBgSwamp,         // Voodoo
        &txBgObservatory,   // Magic
        &txBgAimTraining,   // Clicking
        &txBgFactory,       // Automation
        &txBgWindTunnel,    // Repulsion
        &txBgMagnetosphere, // Attraction
        &txBgAuditorium,    // Camouflage
        &txClouds,          // Victory
    };

    static_assert(za::getArraySize(detailTx) == nShrineTypes + 1u);

    ////////////////////////////////////////////////////////////
    const auto      idx = profile.selectedBackground;
    const za::Vec2f chunkScale{1.f, 1.f};
    const za::Vec2f detailScale{1.f, 1.f};
    const za::Vec2f chunkTextureRectSize  = gameBackgroundView.size.componentWiseDiv(chunkScale);
    const za::Vec2f detailTextureRectSize = gameBackgroundView.size.componentWiseDiv(detailScale);

    targetBackgroundHue  = za::radians(za::degrees(backgroundHues[idx]).asRadians()).wrapUnsigned();
    currentBackgroundHue = currentBackgroundHue.rotatedTowards(targetBackgroundHue, deltaTimeMs * 0.01f).wrapUnsigned();
    outlineHueColor      = colorBlueOutline.withRotatedHue(currentBackgroundHue.asDegrees());

    rtBackground.draw(*chunkTx[idx],
                      {
                          .scale = chunkScale,
                          .textureRect = {{playerInputState.actualScroll + backgroundScroll * 0.25f, 0.f}, chunkTextureRectSize},
                          .color = hueColor(currentBackgroundHue.asDegrees(), getAlpha(255.f)),
                      },
                      {.view = gameBackgroundView, .shader = &shaders.shader});


    static float firstCloudTimer       = 0.f;
    const float  firstCloudTimerTarget = pt->comboPurchased ? 1.f : 0.f;

    firstCloudTimer = exponentialApproach(firstCloudTimer, firstCloudTimerTarget, deltaTimeMs, 1000.f);

    rtBackground.draw(txClouds,
                      {
                          .scale       = {detailScale.x, detailScale.y},
                          .origin      = {0.f, 0.f},
                          .textureRect = {{playerInputState.actualScroll * 1.5f + backgroundScroll * 1.5f, 0.f},
                                          detailTextureRectSize},
                          .color       = za::Color::whiteWithAlpha(getAlpha(255.f * easeInOutSine(firstCloudTimer))),
                      },
                      {.view = gameBackgroundView});

    static float firstDrawingTimer       = 0.f;
    const float  firstDrawingTimerTarget = pt->getCatCountByType(CatType::Normal) > 0 ? 1.f : 0.f;

    firstDrawingTimer = exponentialApproach(firstDrawingTimer, firstDrawingTimerTarget, deltaTimeMs, 1000.f);

    if (idx == 0u || profile.alwaysShowDrawings)
        rtBackground.draw(txDrawings,
                          {
                              .textureRect = {{playerInputState.actualScroll * 2.f, 0.f},
                                              txBackgroundChunk.getSize().toVec2f() * 2.f},
                              .color = za::Color::whiteWithAlpha(getAlpha(200.f * easeInOutSine(firstDrawingTimer))),
                          },
                          {.view = gameBackgroundView});


    if (idx != 0u)
        rtBackground.draw(*detailTx[idx],
                          {
                              .scale       = detailScale,
                              .textureRect = {{playerInputState.actualScroll * 2.f + backgroundScroll * 0.5f, 0.f},
                                              detailTextureRectSize},
                              .color       = za::Color::whiteWithAlpha(getAlpha(190.f)),
                          },
                          {.view = gameBackgroundView});

    rtBackground.display();
    updateProcessedBackground();
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateMoneyText(const float deltaTimeMs, const float yBelowMinimap)
{
    moneyText.setString("$" + za::String(toStringWithSeparators(pt->money + spentMoney)));

    moneyText.setOutlineColor(outlineHueColor);
    moneyText.scale  = {0.5f, 0.5f};
    moneyText.origin = moneyText.getLocalBounds().size / 2.f;

    moneyText.setGlobalTopLeft({10.f, 70.f});
    moneyTextShakeEffect.update(deltaTimeMs);
    moneyTextShakeEffect.applyToText(moneyText);
    moneyText.scale *= 0.5f;

    moneyText.position.y = yBelowMinimap + 25.f;
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateSpentMoneyEffect(const float deltaTimeMs)
{
    if (spentMoney == 0u || spentMoneyTimer.advanceLooping(deltaTimeMs * 0.08f) == LoopResult::Running)
        return;

    if (profile.showCoinParticles)
    {
        playSound(sounds.coin, /* maxOverlap */ 64);

        spawnSpentCoinParticle(
            {.position      = moneyText.getGlobalCenterRight().addY(rngFast.getF(-12.f, 12.f)),
             .velocity      = za::Vec2f{3.f, 0.f},
             .scale         = 0.35f,
             .scaleDecay    = 0.f,
             .accelerationY = 0.f,
             .opacity       = 0.f,
             .opacityDecay  = -0.015f,
             .rotation      = rngFast.getF(0.f, za::tau),
             .torque        = 0.f});
    }

    if (spentMoney > 5u)
    {
        const auto spentMoneyAsFloat = static_cast<float>(spentMoney);
        spentMoney -= static_cast<MoneyType>(za::max(1.f, za::ceil(spentMoneyAsFloat / 10.f)));
    }
    else
    {
        --spentMoney;
    }
}


////////////////////////////////////////////////////////////
za::TextData Main::gameLoopUpdateComboText(const float deltaTimeMs, const float yBelowMinimap)
{
    za::TextData td{};

    if (!pt->comboPurchased)
        return td;

    comboState.comboTextShakeEffect.update(deltaTimeMs);

    td = {
        .position         = {comboState.baseTextPosition.x, yBelowMinimap + 45.f},
        .string           = "x" + za::toString(comboState.combo + 1),
        .characterSize    = 48u,
        .fillColor        = za::Color::White,
        .outlineColor     = outlineHueColor,
        .outlineThickness = 3.f,
    };

    comboState.comboTextShakeEffect.applyToText(td);
    td.scale *= 0.5f;

    return td;
}


////////////////////////////////////////////////////////////
za::TextData Main::gameLoopUpdateBuffText(const za::Rect2f& comboBounds)
{
    const char* devilBuffName = (isDevilcatHellsingedActive()) ? "Portal Storm (Scales With Bomb Spawn Chance)"
                                                               : "Explosive Downpour (Bomb Spawn Chance)";

    const char* const buffNames[] = {
        "Midas Paws (x5 Cat Reward)",          // Normal
        "Shooting Stars (Star Spawn Chance) ", // Uni
        devilBuffName,                         // Devil
        "Endless Flight (Looping Astrocats)",  // Astro
        "Wide Awake (Faster Wakeups)",         // Warden -- TODO P1: actually wire the buff effect

        "N/A",                                         // Witch
        "Mana Overload (x3.5 Mana Regen)",             // Wizard
        "Click Fever (x10 Click Reward)",              // Mouse
        "Global Maintenance (x2 Faster Cooldown)",     // Engi
        "Bubble Hurricane (x2 Bubble Count + Wind)",   // Repulso
        "Demonic Attraction (Magnetic Bombs/Portals)", // Attracto
        "N/A",                                         // Copy
        "N/A",                                         // Duck
    };

    static_assert(za::getArraySize(buffNames) == nCatTypes);

    char  buffStrBuffer[1024]{};
    SizeT writeIdx = 0u;

    const auto countPendingDolls = [](const za::Vector<HexSession>& sessions)
    {
        SizeT count = 0u;

        for (const HexSession& session : sessions)
            count += za::countIf(session.dolls.begin(), session.dolls.end(), [](const Doll& doll) {
                return !doll.tcDeath.hasValue();
            });

        return count;
    };

    const SizeT nDollsToClick     = countPendingDolls(pt->hexSessions);
    const SizeT nCopyDollsToClick = countPendingDolls(pt->copyHexSessions);

    if (nDollsToClick > 0u)
        writeIdx += static_cast<SizeT>(
            std::snprintf(buffStrBuffer + writeIdx, sizeof(buffStrBuffer) - writeIdx, "Dolls to collect: %zu\n", nDollsToClick));

    if (nCopyDollsToClick > 0u)
        writeIdx += static_cast<SizeT>(
            std::snprintf(buffStrBuffer + writeIdx,
                          sizeof(buffStrBuffer) - writeIdx,
                          "Dolls (copy) to collect: %zu\n",
                          nCopyDollsToClick));

    if (pt->mewltiplierAuraTimer > 0.f)
        writeIdx += static_cast<SizeT>(
            std::snprintf(buffStrBuffer + writeIdx,
                          sizeof(buffStrBuffer) - writeIdx,
                          "Mewltiplier Aura (x%.1f Any Reward): %.2fs\n",
                          static_cast<double>(pt->psvMewltiplierMult.currentValue()),
                          static_cast<double>(pt->mewltiplierAuraTimer / 1000.f)));

    if (pt->stasisFieldTimer > 0.f)
        writeIdx += static_cast<SizeT>(
            std::snprintf(buffStrBuffer + writeIdx,
                          sizeof(buffStrBuffer) - writeIdx,
                          "Stasis Field (Bubbles Stuck In Time): %.2fs\n",
                          static_cast<double>(pt->stasisFieldTimer / 1000.f)));

    for (SizeT i = 0u; i < nCatTypes; ++i)
    {
        const float buffTime = pt->buffCountdownsPerType[i].time;

        if (buffTime == 0.f)
            continue;

        writeIdx += static_cast<SizeT>(
            std::snprintf(buffStrBuffer + writeIdx,
                          sizeof(buffStrBuffer) - writeIdx,
                          "%s: %.2fs\n",
                          buffNames[i],
                          static_cast<double>(buffTime / 1000.f)));
    }

    return {
        .position         = {comboState.baseTextPosition.x, comboBounds.position.y + comboBounds.size.y + 10.f},
        .scale            = {0.5f, 0.5f},
        .string           = buffStrBuffer,
        .characterSize    = 48u,
        .fillColor        = za::Color::White,
        .outlineColor     = outlineHueColor,
        .outlineThickness = 3.f,
    };
}


////////////////////////////////////////////////////////////
void Main::gameLoopPrestigeAvailableReminder()
{
    if (!wasPrestigeAvailableLastFrame && pt->canBuyNextPrestige())
    {
        pushNotification("Prestige available!", "Purchase through the \"Prestige\" menu!");

        if (pt->psvBubbleValue.nPurchases == 0u)
            doTip("You can now prestige for the first time!");
    }

    wasPrestigeAvailableLastFrame = pt->canBuyNextPrestige();
}


////////////////////////////////////////////////////////////
void Main::gameLoopReminderBuyCombo()
{
    if (pt->comboPurchased || inPrestigeTransition)
        return;

    const auto handPoppedBubbles = pt->statsSession.getTotalNBubblesHandPopped();

    if (handPoppedBubbles >= 25u && buyReminder == 0)
    {
        buyReminder = 1;
        doTip("Remember to buy the combo upgrade!", /* maxPrestigeLevel */ UINT_MAX);
    }
    else if (handPoppedBubbles >= 50u && buyReminder == 1)
    {
        buyReminder = 2;
        doTip("You should really buy the upgrade now!", /* maxPrestigeLevel */ UINT_MAX);
    }
    else if (handPoppedBubbles >= 100u && buyReminder == 2)
    {
        buyReminder = 3;
        doTip("What are you trying to prove...?", /* maxPrestigeLevel */ UINT_MAX);
    }
    else if (handPoppedBubbles >= 200u && buyReminder == 3)
    {
        buyReminder = 4;
        doTip("There is no achievement for doing this!", /* maxPrestigeLevel */ UINT_MAX);
    }
    else if (handPoppedBubbles >= 300u && buyReminder == 4)
    {
        buyReminder = 5;
        doTip("Fine, have it your way!\nHere's your dumb achievement!\nAnd now buy the upgrade!",
              /* maxPrestigeLevel */ UINT_MAX);
    }
}


////////////////////////////////////////////////////////////
void Main::gameLoopReminderSpendPPs()
{
    if (!pt->spendPPTipShown && pt->psvBubbleValue.nPurchases == 1u && pt->prestigePoints > 0u && pt->money > 500u)
    {
        pt->spendPPTipShown = true;
        doTip("Remember that you have some\nprestige points to spend!");
    }
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateDpsSampler(const za::I64 elapsedUs)
{
    moneyGainedUsAccumulator += elapsedUs;
    while (moneyGainedUsAccumulator >= 1'000'000)
    {
        moneyGainedUsAccumulator -= 1'000'000;

        samplerMoneyPerSecond.record(static_cast<float>(moneyGainedLastSecond));
        moneyGainedLastSecond = 0u;

        statHighestDPS(static_cast<za::U64>(samplerMoneyPerSecond.getAverageAs<double>()));
    }
}
