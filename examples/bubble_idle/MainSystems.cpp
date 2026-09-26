

#include "Aliases.hpp"
#include "Bubble.hpp"
#include "BubbleIdleHelpers.hpp"
#include "BubbleIdleMain.hpp"
#include "BubbleIdleMainInline.hpp"
#include "BubbleType.hpp"
#include "Cat.hpp"
#include "CatType.hpp"
#include "ComboState.hpp"
#include "Constants.hpp"
#include "Doll.hpp"
#include "ExampleProfiler/Profiler.hpp"
#include "GameConstants.hpp"
#include "GameEvent.hpp"
#include "HellPortal.hpp"
#include "HexSession.hpp"
#include "InputHelper.hpp"
#include "MainAtlasRects.hpp"
#include "MainBombStorage.hpp"
#include "ParticleData.hpp"
#include "ParticleType.hpp"
#include "Playthrough.hpp"
#include "Profile.hpp"
#include "Shrine.hpp"
#include "ShrineType.hpp"
#include "Stats.hpp"
#include "TextParticle.hpp"

#include "ExampleUtils/ControlFlow.hpp"
#include "ExampleUtils/Easing.hpp"
#include "ExampleUtils/HueColor.hpp"
#include "ExampleUtils/MathUtils.hpp"
#include "ExampleUtils/Progress.hpp"
#include "ExampleUtils/SoundManager.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Sprite.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Window/Keyboard.hpp"

#include "Zancle/Algorithm/AllOf.hpp"
#include "Zancle/Algorithm/AnyOf.hpp"
#include "Zancle/Algorithm/Count.hpp"
#include "Zancle/Algorithm/Erase.hpp"
#include "Zancle/Algorithm/MaxElement.hpp"

#include "Zancle/Chrono/Clock.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/OverloadSet.hpp"

#include "Zancle/Math/Ceil.hpp"
#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Cos.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/MinMax.hpp"
#include "Zancle/Math/Pow.hpp"
#include "Zancle/Math/Remainder.hpp"
#include "Zancle/Math/Sin.hpp"
#include "Zancle/Math/Sqrt.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/GetArraySize.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"

#include <utility>

#include <cstdio>

////////////////////////////////////////////////////////////
void Main::gameLoopUpdateScrolling(const float deltaTimeMs, const za::Vector<za::Vec2f>& downFingers)
{
    // Reset map scrolling
    if (keyDown(za::Keyboard::Key::LShift) || (downFingers.size() != 2u && !mBtnDown(getRMB(), /* penetrateUI */ true)))
        playerInputState.dragPosition.reset();

    //
    // Scrolling
    playerInputState.scroll = za::clamp(playerInputState.scroll,
                                        0.f,
                                        za::max(0.f,
                                                za::min(pt->getMapLimit() / 2.f - getCurrentGameViewSize().x / 2.f,
                                                        (boundaries.x - getCurrentGameViewSize().x) / 2.f)));

    playerInputState.actualScroll = exponentialApproach(playerInputState.actualScroll, playerInputState.scroll, deltaTimeMs, 75.f);
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateTransitions(const float deltaTimeMs)
{
    // Compute screen count
    constexpr auto nMaxScreens       = boundaries.x / gameScreenSize.x;
    const auto     nPurchasedScreens = static_cast<SizeT>(pt->getMapLimit() / gameScreenSize.x) + 1u;

    // Compute total target bubble count
    const auto targetBubbleCountPerScreen = static_cast<SizeT>(pt->psvBubbleCount.currentValue() / nMaxScreens);

    auto targetBubbleCount = targetBubbleCountPerScreen * nPurchasedScreens;

    const bool repulsoBuffActive = pt->buffCountdownsPerType[asIdx(CatType::Repulso)].time > 0.f;

    if (repulsoBuffActive)
        targetBubbleCount *= 2u;

    // Helper functions
    const auto playReversePopAt = [this](const za::Vec2f position)
    {
        // TODO P2: refactor into function for any sound and reuse
        sounds.reversePop.settings.position = {position.x, position.y};
        playSound(sounds.reversePop, /* maxOverlap */ 1u);
    };

    // If we are still displaying the splash screen, exit early
    if (splashCountdown.tick(deltaTimeMs) != TickResult::AlreadyFinished)
        return;

    // Spawn bubbles and shrines during normal gmaeplay
    if (!inPrestigeTransition)
    {
        // Spawn shrines if required
        pt->spawnAllShrinesIfNeeded();

        // The bubble cap only governs regular (non-ephemeral) bubbles. Event
        // bubbles like Bubblefall and the invincible Combo bubble are bonuses
        // that don't count toward the cap and are never trimmed.
        const SizeT nonEphemeralCount = za::countIf(pt->bubbles.begin(), pt->bubbles.end(), [](const Bubble& b) {
            return !b.ephemeral;
        });

        if (nonEphemeralCount < targetBubbleCount)
        {
            const SizeT times = (targetBubbleCount - nonEphemeralCount) > 500u ? 25u : 1u;

            for (SizeT i = 0; i < times; ++i)
            {
                auto& bubble    = pt->bubbles.emplaceBack(makeRandomBubble(*pt, rng, pt->getMapLimit(), boundaries.y));
                const auto bPos = bubble.position;

                if (repulsoBuffActive)
                    bubble.velocity += {0.18f, 0.18f};

                spawnParticles(8, bPos, ParticleType::Bubble, 0.5f, 0.5f);
                playReversePopAt(bPos);
            }
        }
        else if (nonEphemeralCount > targetBubbleCount)
        {
            const SizeT excess = nonEphemeralCount - targetBubbleCount;
            const SizeT times  = excess > 500u ? 25u : 1u;

            for (SizeT i = 0; i < times; ++i)
            {
                // Find the rearmost non-ephemeral bubble; ephemerals are
                // skipped entirely so event bubbles are never popped.
                SizeT idx        = pt->bubbles.size();
                bool  foundIndex = false;
                while (idx > 0u)
                {
                    --idx;
                    if (!pt->bubbles[idx].ephemeral)
                    {
                        foundIndex = true;
                        break;
                    }
                }

                if (!foundIndex)
                    break;

                const auto bPos = pt->bubbles[idx].position;

                // Swap-to-back so the cheap popBack still works regardless of
                // where the non-ephemeral was found in the vector.
                if (idx != pt->bubbles.size() - 1u)
                {
                    Bubble tmp         = pt->bubbles.back();
                    pt->bubbles.back() = pt->bubbles[idx];
                    pt->bubbles[idx]   = tmp;
                }
                pt->bubbles.popBack();

                spawnParticles(8, bPos, ParticleType::Bubble, 0.5f, 0.5f);
                playReversePopAt(bPos);
            }
        }

        return;
    }

    ZA_ASSERT(inPrestigeTransition);

    // Despawn cats, dolls, copydolls, and shrines
    if (catRemoveTimer.tickLooping(deltaTimeMs) == LoopResult::Looped)
    {
        if (!pt->cats.empty())
        {
            for (auto& cat : pt->cats)
            {
                cat.astroState.reset();
                cat.cooldown.time = 100.f;
            }

            // Find rightmost cat
            auto* const rightmostIt = za::maxElement(pt->cats.begin(), pt->cats.end(), [](const Cat& a, const Cat& b) {
                return a.position.x < b.position.x;
            });

            const float targetScroll = (rightmostIt->position.x - getCurrentGameViewSize().x / 2.f) / 2.f;
            playerInputState.scroll  = exponentialApproach(playerInputState.scroll, targetScroll, deltaTimeMs, 15.f);

            if (rightmostIt != pt->cats.end())
                std::swap(*rightmostIt, pt->cats.back());

            const auto cPos = pt->cats.back().position;
            pt->cats.popBack();

            spawnParticle({.position      = cPos.addY(29.f),
                           .velocity      = {0.f, 0.f},
                           .scale         = 0.2f,
                           .scaleDecay    = 0.f,
                           .accelerationY = -0.00015f,
                           .opacity       = 1.f,
                           .opacityDecay  = 0.0002f,
                           .rotation      = 0.f,
                           .torque        = rngFast.getF(-0.0002f, 0.0002f)},
                          /* hue */ 0.f,
                          ParticleType::CatSoul);

            spawnParticles(24, cPos, ParticleType::Star, 1.f, 0.5f);
            playReversePopAt(cPos);
        }

        if (!pt->shrines.empty())
        {
            const auto cPos = pt->shrines.back().position;
            pt->shrines.popBack();

            spawnParticles(24, cPos, ParticleType::Star, 1.f, 0.5f);
            playReversePopAt(cPos);
        }

        const auto popOneDollOrSession = [&](za::Vector<HexSession>& sessions)
        {
            while (!sessions.empty() && sessions.back().dolls.empty())
                sessions.popBack();

            if (sessions.empty())
                return;

            auto&      session = sessions.back();
            const auto cPos    = session.dolls.back().position;
            session.dolls.popBack();

            if (session.dolls.empty())
                sessions.popBack();

            spawnParticles(24, cPos, ParticleType::Star, 1.f, 0.5f);
            playReversePopAt(cPos);
        };

        popOneDollOrSession(pt->hexSessions);
        popOneDollOrSession(pt->copyHexSessions);

        if (!pt->hellPortals.empty())
        {
            const auto cPos = pt->hellPortals.back().position;
            pt->hellPortals.popBack();

            spawnParticles(24, cPos, ParticleType::Star, 1.f, 0.5f);
            playReversePopAt(cPos);
        }
    }

    const bool gameElementsRemoved = pt->cats.empty() && pt->shrines.empty() && pt->hexSessions.empty() &&
                                     pt->copyHexSessions.empty() && pt->hellPortals.empty();

    // Reset map extension and scroll, and remove bubbles outside of view
    if (gameElementsRemoved)
    {
        pt->mapPurchased               = false;
        pt->psvMapExtension.nPurchases = 0u;

        playerInputState.scroll = 0.f;

        za::vectorEraseIf(pt->bubbles, [&](const Bubble& b) { return b.position.x > pt->getMapLimit() + 128.f; });
    }

    // Despawn bubbles after other things
    if (gameElementsRemoved && !pt->bubbles.empty() && bubbleSpawnTimer.tickLooping(deltaTimeMs) == LoopResult::Looped)
    {
        const SizeT times = pt->bubbles.size() > 500u ? 25u : 1u;

        for (SizeT i = 0; i < times; ++i)
        {
            const auto bPos = pt->bubbles.back().position;
            pt->bubbles.popBack();

            spawnParticles(8, bPos, ParticleType::Bubble, 0.5f, 0.5f);
            playReversePopAt(bPos);
        }
    }

    // End prestige transition
    if (gameElementsRemoved && pt->bubbles.empty())
    {
        pt->statsSession = Stats{};

        inPrestigeTransition = false;
        pt->money            = pt->perm.starterPackPurchased ? 1000u : 0u;

        resetAllDraggedCats();

        spentMoney               = 0u;
        moneyGainedLastSecond    = 0u;
        moneyGainedUsAccumulator = 0u;
        samplerMoneyPerSecond.clear();
        bombStorage->bombIdxToCatIdx.clear();

        splashCountdown.restart();
        playSound(sounds.byteMeow);
    }
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateBubbles(const float deltaTimeMs)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    constexpr float maxVelocityMagnitude  = 2.f;
    constexpr float windMult[4]           = {0.f, 0.00009f, 0.00018f, 0.0005f};
    constexpr float windStartVelocityY[4] = {0.07f, 0.18f, 0.25f, 0.55f};

    for (Bubble& bubble : pt->bubbles)
    {
        // Pending transform (Unicat star, Devilcat bomb, ...): the bubble is
        // frozen mid-air while converging particles fall in. When the timer
        // hits 0 we swap the type and emit the resolution burst. No motion /
        // wind / gravity during the animation, so bail out of the usual
        // update for this bubble.
        if (bubble.pendingTransformMs > 0.f)
        {
            bubble.pendingTransformMs -= deltaTimeMs;

            if (bubble.pendingTransformMs <= 0.f)
            {
                bubble.pendingTransformMs         = 0.f;
                const auto targetType             = static_cast<BubbleType>(bubble.pendingTransformTargetType);
                const auto creditCatIdx           = bubble.pendingTransformCatIdx;
                bubble.pendingTransformTargetType = 0u;
                bubble.pendingTransformCatIdx     = 0xFF'FFu;

                turnBubbleInto(bubble, targetType);

                if (targetType == BubbleType::Bomb)
                {
                    spawnParticles(8u, bubble.position, ParticleType::Fire, 1.25f, 0.35f);
                    bubble.velocity.y += rng.getF(0.1f, 0.2f);

                    // Credit the originating Devilcat so the bomb's kill
                    // payouts flow back to the correct cat. Uses the current
                    // slot index because the vector may have reshuffled
                    // during the freeze window.
                    if (creditCatIdx != 0xFF'FFu)
                    {
                        const auto bubbleIdx                    = static_cast<za::SizeT>(&bubble - pt->bubbles.data());
                        bombStorage->bombIdxToCatIdx[bubbleIdx] = static_cast<za::SizeT>(creditCatIdx);
                    }
                }
                else
                {
                    const bool aoe            = pt->perm.unicatTranscendenceAOEPurchased;
                    const auto nStarParticles = aoe ? 1u : 4u;
                    spawnParticles(nStarParticles, bubble.position, ParticleType::Star, 0.5f, 0.35f);
                    bubble.velocity.y -= 0.2f;
                }
            }

            continue;
        }

        if (bubble.velocity.lengthSquared() > maxVelocityMagnitude * maxVelocityMagnitude)
            bubble.velocity = bubble.velocity.normalized() * maxVelocityMagnitude;

        bubble.rotation += deltaTimeMs * bubble.torque;

        // Keep long-lived spinning bubbles' angles small (Star/Nova bubbles use `rotation` as absorption progress)
        if (bubble.type != BubbleType::Star && bubble.type != BubbleType::Nova)
            bubble.rotation = za::remainder(bubble.rotation, za::tau);

        if (bubble.type == BubbleType::Star || bubble.type == BubbleType::Nova)
            bubble.hueMod += deltaTimeMs * 0.125f;

        // Combo bubbles ignore wind, gravity and stasis: they drift down with
        // their initial velocity for a slow, predictable fall, and their
        // vertical speed is hard-capped so even out-of-range JSON tunings
        // can't make them blaze across the screen.
        const bool isCombo = bubble.type == BubbleType::Combo;

        if (isCombo)
        {
            const auto& cfg = gameConstants.events.invincibleBubble;

            if (bubble.velocity.y > cfg.maxVelocityY)
                bubble.velocity.y = cfg.maxVelocityY;

            // Bespoke per-bubble combo timer: starts ticking after the first
            // click. When it expires the bubble pops with payout.
            if (bubble.comboClickCount > 0u && bubble.comboTimerMs > 0.f)
            {
                bubble.comboTimerMs -= deltaTimeMs;
                if (bubble.comboTimerMs <= 0.f)
                {
                    bubble.comboTimerMs = 0.f;
                    popComboBubble(bubble);
                }
            }

            // Ambient repulsion of nearby bubbles so the Combo bubble visibly
            // stands out as it drifts.
            const float repelR = cfg.ambientRepelRadius;
            if (repelR > 0.f && cfg.ambientRepelStrength > 0.f)
            {
                const float repelRSq = repelR * repelR;
                for (Bubble& other : pt->bubbles)
                {
                    if (&other == &bubble || other.type == BubbleType::Combo)
                        continue;

                    const auto  diff   = other.position - bubble.position;
                    const float distSq = diff.lengthSquared();
                    if (distSq <= 0.0001f || distSq > repelRSq)
                        continue;

                    const float dist    = za::sqrt(distSq);
                    const float falloff = 1.f - (dist / repelR);
                    other.velocity += (diff / dist) * (cfg.ambientRepelStrength * falloff * deltaTimeMs);
                }
            }
        }
        else
        {
            float windVelocity = windMult[pt->windStrength] * (bubble.type == BubbleType::Bomb ? 0.01f : 0.9f);

            if (pt->buffCountdownsPerType[asIdx(CatType::Repulso)].time > 0.f)
                windVelocity += 0.00015f;

            if (windVelocity > 0.f)
            {
                bubble.velocity.x += (windVelocity * 0.5f) * deltaTimeMs;
                bubble.velocity.y += windVelocity * deltaTimeMs;
            }

            if (isBubbleInStasisField(bubble))
                bubble.velocity = {0.f, 0.f};
        }

        bubble.position += bubble.velocity * deltaTimeMs;

        // X-axis wraparound
        if (bubble.position.x - bubble.radius > pt->getMapLimit())
            bubble.position.x = -bubble.radius;
        else if (bubble.position.x + bubble.radius < 0.f)
            bubble.position.x = pt->getMapLimit() + bubble.radius;

        // Y-axis below and above screen
        if (bubble.position.y - bubble.radius > boundaries.y)
        {
            // Event-spawned bubbles despawn instead of being recycled at the
            // top -- the post-loop `vectorEraseIf` removes them.
            if (bubble.ephemeral)
            {
                // A Combo bubble that was clicked at least once still pays out
                // when it slips past the bottom -- only the never-clicked one
                // disappears silently. (Normally the per-bubble combo timer
                // fires before this happens; this is the safety net.)
                if (bubble.type == BubbleType::Combo && bubble.comboClickCount > 0u)
                    popComboBubble(bubble);

                continue;
            }

            bubble.position.x = rng.getF(0.f, pt->getMapLimit());
            bubble.position.y = -bubble.radius * rng.getF(1.f, 2.f);

            bubble.velocity.y = windStartVelocityY[pt->windStrength];

            if (za::fabs(bubble.velocity.x) > 0.04f)
                bubble.velocity.x = 0.04f;

            const bool uniBuffEnabled       = pt->buffCountdownsPerType[asIdx(CatType::Uni)].time > 0.f;
            const bool devilBombBuffEnabled = !isDevilcatHellsingedActive() &&
                                              pt->buffCountdownsPerType[asIdx(CatType::Devil)].time > 0.f;

            const bool willBeStar = uniBuffEnabled &&
                                    rng.getF(0.f, 100.f) <= pt->psvPPUniRitualBuffPercentage.currentValue();
            const bool willBeBomb = devilBombBuffEnabled &&
                                    rng.getF(0.f, 100.f) <= pt->psvPPDevilRitualBuffPercentage.currentValue();

            const auto starType = isUnicatTranscendenceActive() ? BubbleType::Nova : BubbleType::Star;

            if (!willBeStar && !willBeBomb)
                turnBubbleInto(bubble, BubbleType::Normal);
            else if (willBeBomb && willBeStar)
            {
                turnBubbleInto(bubble, rng.getF(0.f, 1.f) > 0.5f ? starType : BubbleType::Bomb);
            }
            else if (willBeBomb)
            {
                turnBubbleInto(bubble, BubbleType::Bomb);
            }
            else if (willBeStar)
            {
                turnBubbleInto(bubble, starType);
            }
        }
        else if (bubble.position.y + bubble.radius < -128.f)
        {
            turnBubbleInto(bubble, BubbleType::Normal);
        }

        if (!isCombo)
            bubble.velocity.y += 0.00005f * deltaTimeMs;

        (void)bubble.repelledCountdown.tick(deltaTimeMs);
        (void)bubble.attractedCountdown.tick(deltaTimeMs);
    }

    // Ephemeral bubbles that have fallen off the bottom stay in `pt->bubbles`
    // for the remainder of the frame: `sweepAndPrune` already captured their
    // indices and the collision passes below rely on them staying stable.
    // `gameLoopReapEphemeralBubbles` at end-of-frame removes them.
}


////////////////////////////////////////////////////////////
void Main::gameLoopReapEphemeralBubbles()
{
    za::vectorEraseIf(pt->bubbles,
                      [&](const Bubble& b) { return b.ephemeral && (b.position.y - b.radius > boundaries.y); });
}


////////////////////////////////////////////////////////////
void Main::popComboBubble(Bubble& bubble)
{
    ZA_ASSERT(bubble.type == BubbleType::Combo);

    // Idempotency: a bubble can be popped from multiple paths (timer, max
    // clicks, bottom-despawn). Skip if we've already paid out.
    if (bubble.comboClickCount == 0u && bubble.comboTimerMs == 0.f && bubble.radius < 0.f)
        return;

    const auto& cfg = gameConstants.events.invincibleBubble;

    const auto clicks = bubble.comboClickCount;
    if (clicks == 0u)
    {
        // Defensive: pop with no payout (shouldn't normally be reached).
        bubble.position.y = boundaries.y * 2.f;
        bubble.radius     = -1.f;
        return;
    }

    // Reward scales super-linearly with click count and tracks the prestige
    // bubble value so it stays meaningful late game.
    const float baseReward = cfg.rewardScalePerClick * za::pow(static_cast<float>(clicks), cfg.rewardClickExponent);
    const auto reward = static_cast<MoneyType>(baseReward) * static_cast<MoneyType>(pt->psvBubbleValue.currentValue() + 1.f);

    addMoney(reward);
    moneyTextShakeEffect.bump(rngFast, 1.f + static_cast<float>(clicks) * 0.1f);

    if (profile.showTextParticles)
    {
        auto& tp = makeRewardTextParticle(bubble.position);

        tp.velocity *= 0.5f;
        tp.opacityDecay *= 0.5f;
        tp.accelerationY *= 0.35f;
        tp.scale *= 1.3f;

        std::snprintf(tp.buffer, sizeof(tp.buffer), "+$%llu", reward);
    }

    // Big visual burst at the bubble.
    spawnParticles(64, bubble.position, ParticleType::Bubble, 0.8f, 0.6f);
    spawnParticles(32, bubble.position, ParticleType::Star, 0.6f, 0.45f);

    sounds.buffon.settings.position = {bubble.position.x, bubble.position.y};
    playSound(sounds.buffon, /* maxOverlap */ 4u);

    sounds.glassbreak.settings.position = {bubble.position.x, bubble.position.y};
    playSound(sounds.glassbreak, /* maxOverlap */ 4u);

    // One-shot radial impulse on nearby bubbles for a punchy "blowout".
    const float popR = cfg.popRepelRadius;
    if (popR > 0.f && cfg.popRepelImpulse > 0.f)
    {
        const float popRSq = popR * popR;
        for (Bubble& other : pt->bubbles)
        {
            if (&other == &bubble)
                continue;

            const auto  diff   = other.position - bubble.position;
            const float distSq = diff.lengthSquared();
            if (distSq <= 0.0001f || distSq > popRSq)
                continue;

            const float dist    = za::sqrt(distSq);
            const float falloff = 1.f - (dist / popR);
            other.velocity += (diff / dist) * (cfg.popRepelImpulse * falloff);
        }
    }

    // Queue the burst-then-collect coin spew. Capped to avoid pathological cases.
    const SizeT coinCount = za::min(static_cast<SizeT>(cfg.payoutMaxCoins),
                                    static_cast<SizeT>(clicks) * static_cast<SizeT>(cfg.payoutCoinsPerClick));

    PendingComboBubblePayout payout{
        .coins           = {},
        .coinsCollected  = 0u,
        .settleCountdown = Countdown{.time = cfg.burstSettleDelayMs},
        .collectDelay    = {},
    };
    payout.coins.reserve(coinCount);

    for (SizeT i = 0u; i < coinCount; ++i)
    {
        // Spread coins evenly around a circle (with jitter) and give them a
        // randomized outward speed so the explosion looks organic.
        const float angle = static_cast<float>(i) * (za::tau / static_cast<float>(coinCount)) + rngFast.getF(-0.25f, 0.25f);
        const float speed = rngFast.getF(cfg.burstSpeedMin, cfg.burstSpeedMax);

        payout.coins.pushBack(BurstingComboCoin{
            .position  = bubble.position,
            .velocity  = {za::cos(angle) * speed, za::sin(angle) * speed},
            .collected = false,
        });
    }

    pendingComboBubblePayouts.pushBack(ZA_MOVE(payout));

    // Despawn: push past the bottom and zero the radius so the ephemeral
    // reaper takes it next frame, and so re-entering this function is a no-op.
    bubble.position.y      = boundaries.y * 2.f;
    bubble.radius          = -1.f;
    bubble.comboClickCount = 0u;
    bubble.comboTimerMs    = 0.f;
    bubble.ephemeral       = true;
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateComboBubblePayouts(const float deltaTimeMs)
{
    if (pendingComboBubblePayouts.empty())
        return;

    const auto& cfg         = gameConstants.events.invincibleBubble;
    const float coinDelayMs = cfg.payoutCoinDelayMs <= 0.f ? 35.f : cfg.payoutCoinDelayMs;

    // Per-frame velocity multiplier from the per-second damping factor.
    const float dampingPerSec = za::clamp(cfg.burstDampingPerSec, 0.f, 1.f);
    const float dampingFrame  = za::pow(dampingPerSec, deltaTimeMs * 0.001f);

    for (PendingComboBubblePayout& p : pendingComboBubblePayouts)
    {
        // Phase 1: burst -- every coin drifts outward, decelerating each frame.
        for (BurstingComboCoin& c : p.coins)
        {
            if (c.collected)
                continue;

            c.position += c.velocity * deltaTimeMs;
            c.velocity *= dampingFrame;
        }

        // Phase 2: settle delay -- coins keep drifting/damping but no
        // collection happens yet so the player sees the explosion settle.
        if (p.settleCountdown.tick(deltaTimeMs) != TickResult::AlreadyFinished && p.settleCountdown.time > 0.f)
            continue;

        // Phase 3: collect -- every `coinDelayMs`, take the next settled coin,
        // route it to the HUD via the existing `EarnedCoinParticle` path, and
        // play the rising-pitch coin chime. Pitch climbs with `coinsCollected`.
        if (p.collectDelay.tickLooping(deltaTimeMs, coinDelayMs) != LoopResult::Looped)
            continue;

        p.collectDelay.time = coinDelayMs;

        BurstingComboCoin* next = nullptr;
        for (BurstingComboCoin& c : p.coins)
            if (!c.collected)
            {
                next = &c;
                break;
            }

        if (next == nullptr)
            continue;

        const za::Vec2f hudOrigin = fromWorldToHud(next->position);
        if (spawnEarnedCoinParticle(hudOrigin))
        {
            const za::Vec2f viewSize           = getCurrentGameViewSize();
            const za::Vec2f viewCenter         = getViewCenter();
            sounds.coindelay.settings.position = {viewCenter.x - viewSize.x / 2.f + 25.f,
                                                  viewCenter.y - viewSize.y / 2.f + 25.f};
            sounds.coindelay.settings.pitch    = 0.8f + static_cast<float>(p.coinsCollected) * 0.04f;
            sounds.coindelay.settings.volume   = profile.sfxVolume / 100.f;

            playSound(sounds.coindelay, /* maxOverlap */ 64u);
        }

        next->collected = true;
        ++p.coinsCollected;
    }

    za::vectorEraseIf(pendingComboBubblePayouts,
                      [](const PendingComboBubblePayout& p)
    {
        for (const BurstingComboCoin& c : p.coins)
            if (!c.collected)
                return false;
        return true;
    });
}


////////////////////////////////////////////////////////////
void Main::gameLoopDrawComboBubbleBurstingCoins()
{
    if (pendingComboBubblePayouts.empty() || !profile.showParticles)
        return;

    const float spinPhase = shaderTime * 0.005f;

    for (const PendingComboBubblePayout& p : pendingComboBubblePayouts)
    {
        for (const BurstingComboCoin& c : p.coins)
        {
            if (c.collected)
                continue;

            cpuDrawableBatchAfterCats.add(za::Sprite{
                .position    = c.position,
                .scale       = {0.18f, 0.18f},
                .origin      = atlasRects.txrCoin.size / 2.f,
                .rotation    = za::radians(spinPhase),
                .textureRect = atlasRects.txrCoin,
                .color       = za::Color::White,
            });
        }
    }
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateAttractoBuff(const float deltaTimeMs) const
{
    if (pt->buffCountdownsPerType[asIdx(CatType::Attracto)].time <= 0.f)
        return;

    const auto sqAttractoRange = pt->getComputedSquaredRangeByCatType(CatType::Attracto);
    const auto attractoRange   = ZA_MATH_SQRTF(sqAttractoRange);

    static thread_local za::Vector<Bubble*> bombs;
    bombs.clear();

    for (Bubble& bubble : pt->bubbles)
        if (bubble.type == BubbleType::Bomb)
            bombs.pushBack(&bubble);

    const auto attract = [&](const za::Vec2f pos, Bubble& bubble)
    {
        const auto diff     = (pos - bubble.position);
        const auto sqLength = diff.lengthSquared();

        if (sqLength > sqAttractoRange)
            return;

        const float length = ZA_MATH_SQRTF(sqLength);

        const auto strength = (attractoRange - length) * 0.000017f;
        bubble.velocity += (diff / length * strength * getWindAttractionMult()) * 1.f * deltaTimeMs;

        bubble.attractedCountdown.time = za::max(bubble.attractedCountdown.time, 750.f);
    };

    for (Bubble& bubble : pt->bubbles)
    {
        if (bubble.type == BubbleType::Bomb)
            continue;

        for (const HellPortal& hp : pt->hellPortals)
            attract(hp.position, bubble);

        for (const Bubble* bomb : bombs)
            attract(bomb->position, bubble);
    }
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool Main::gameLoopUpdateBubbleClick(za::Optional<za::Vec2f>& clickPosition)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    if (!clickPosition.hasValue())
        return false;

    const auto clickPos = gameView.screenToWorld(*clickPosition, window.getSize().toVec2f());

    if (!particleCullingBoundaries.isInside(clickPos))
    {
        clickPosition.reset();
        return false;
    }

    Bubble* firstClickedBubble           = nullptr;
    bool    firstClickedBubbleByMultiPop = false;

    forEachBubbleInRadius(clickPos,
                          128.f,
                          [&](Bubble& bubble)
    {
        if ((clickPos - bubble.position).lengthSquared() > bubble.getRadiusSquared())
            return ControlFlow::Continue;

        // Bubbles mid transform (star / bomb / ...) are inert: don't consume
        // the click.
        if (bubble.pendingTransformMs > 0.f)
            return ControlFlow::Continue;

        // Prevent clicks around shrine of automation
        for (const Shrine& shrine : pt->shrines)
            if (shrine.type == ShrineType::Automation && shrine.isInRange(clickPos))
            {
                sounds.failpop.settings.position = {clickPos.x, clickPos.y};
                playSound(sounds.failpop);

                return ControlFlow::Break;
            }

        firstClickedBubble = &bubble;
        return ControlFlow::Break;
    });

    if (firstClickedBubble == nullptr && pt->multiPopEnabled && !pt->laserPopEnabled)
        forEachBubbleInRadius(clickPos,
                              pt->psvPPMultiPopRange.currentValue(),
                              [&](Bubble& b)
        {
            if (b.pendingTransformMs > 0.f)
                return ControlFlow::Continue;

            firstClickedBubble           = &b;
            firstClickedBubbleByMultiPop = true;

            return ControlFlow::Break;
        });

    if (firstClickedBubble != nullptr)
    {
        // Combo (invincible) bubbles use a bespoke per-bubble timer and reward
        // path. Clicking one does not extend the player's regular combo, but
        // it counts as "popped something" so the regular combo doesn't break.
        if (firstClickedBubble->type == BubbleType::Combo)
        {
            const auto& cfg = gameConstants.events.invincibleBubble;

            ++firstClickedBubble->comboClickCount;
            firstClickedBubble->comboTimerMs = za::max(cfg.comboTimerMaxMs -
                                                           static_cast<float>(firstClickedBubble->comboClickCount) * 25.f,
                                                       100.f);

            // Slight per-click size growth, capped, so the bubble visibly
            // "fattens" as the player builds the combo.
            const float currentGrowth = firstClickedBubble->radius - cfg.maxRadius;
            if (currentGrowth < cfg.radiusGrowthMax)
                firstClickedBubble->radius += za::min(cfg.radiusGrowthPerClick,
                                                      cfg.radiusGrowthMax - za::max(0.f, currentGrowth));

            sounds.pop.settings.position = {firstClickedBubble->position.x, firstClickedBubble->position.y};
            sounds.pop.settings.pitch = remap(static_cast<float>(firstClickedBubble->comboClickCount), 1.f, 30.f, 1.2f, 2.5f);
            playSound(sounds.pop, /* maxOverlap */ 64u);

            sounds.glasshit.settings.position = {firstClickedBubble->position.x, firstClickedBubble->position.y};
            sounds.glasshit.settings.pitch = 1.f + static_cast<float>(firstClickedBubble->comboClickCount - 1u) * 0.03f;
            playSound(sounds.glasshit, /* maxOverlap */ 64u);

            // Randomize rotation on every click so the crystal looks like it's cracking.
            firstClickedBubble->rotation = rngFast.getF(0.f, za::tau);

            // Gentle nudge: random horizontal wobble + slight downward push per click.
            // Near a map edge, clamp the horizontal range so the nudge always points
            // back toward the center (prevents knocking the bubble off the playfield).
            constexpr float edgeZone = 200.f;
            const float     mapLimit = pt->getMapLimit();

            float hLo = -0.15f;
            float hHi = 0.15f;
            if (firstClickedBubble->position.x < edgeZone)
                hLo = 0.f;
            else if (firstClickedBubble->position.x > mapLimit - edgeZone)
                hHi = 0.f;

            firstClickedBubble->velocity += rngFast.getVec2f({hLo, 0.05f}, {hHi, 0.2f}) * 0.8f;

            // Glass shards: pop upward from the bubble, arc and fall under gravity.
            for (za::SizeT i = 0u; i < 6u; ++i)
            {
                spawnParticle({.position      = firstClickedBubble->position,
                               .velocity      = rngFast.getVec2f({-0.4f, -1.f}, {0.4f, -0.2f}),
                               .scale         = rngFast.getF(0.15f, 0.3f),
                               .scaleDecay    = 0.f,
                               .accelerationY = 0.0025f,
                               .opacity       = 1.f,
                               .opacityDecay  = 0.001f,
                               .rotation      = rngFast.getF(0.f, za::tau),
                               .torque        = rngFast.getF(-0.02f, 0.02f)},
                              /* hue */ 0.f,
                              ParticleType::Glass);
            }

            // spawnParticles(8, firstClickedBubble->position, ParticleType::Star, 0.4f, 0.4f);

            // Absorption ring: a handful of particles spawn around the bubble
            // and fly inward into it, fading just before they reach the
            // center.
            const za::Vec2f bubbleCenter = firstClickedBubble->position;
            const float     ringRadius   = za::max(cfg.clickAbsorbRadius, firstClickedBubble->radius * 1.5f);

            for (za::SizeT i = 0u; i < 8u; ++i)
            {
                const float     angle   = rngFast.getF(0.f, za::tau);
                const za::Vec2f outward = {za::cos(angle), za::sin(angle)};
                const za::Vec2f start   = bubbleCenter + outward * ringRadius * rngFast.getF(0.8f, 1.3f);

                spawnParticle({.position      = start,
                               .velocity      = -outward * cfg.clickAbsorbSpeed,
                               .scale         = rngFast.getF(0.009f, 0.016f) * 5.f,
                               .scaleDecay    = 0.0001f, // shrink as it converges
                               .accelerationY = 0.f,
                               .opacity       = 1.f,
                               .opacityDecay  = 0.0035f,
                               .rotation      = rngFast.getF(0.f, za::tau),
                               .torque        = rngFast.getF(-0.005f, 0.005f)},
                              /* hue */ 0.f, // gold to match the bubble
                              ParticleType::Star);
            }

            if (firstClickedBubble->comboClickCount >= cfg.maxClicks)
                popComboBubble(*firstClickedBubble);

            if (!pt->speedrunStartTime.hasValue())
                pt->speedrunStartTime.emplace(za::Clock::now());

            return true;
        }

        if (!firstClickedBubbleByMultiPop && pt->comboPurchased)
        {
            if (!pt->laserPopEnabled)
            {
                addCombo(comboState.combo, comboState.comboCountdown);
                comboState.comboTextShakeEffect.bump(rngFast, 1.f + static_cast<float>(comboState.combo) * 0.2f);
            }
            else
            {
                ++comboState.laserCursorCombo;

                if (comboState.combo == 0 || comboState.laserCursorCombo >= 10)
                {
                    addCombo(comboState.combo, comboState.comboCountdown);
                    comboState.comboTextShakeEffect.bump(rngFast, 0.01f + static_cast<float>(comboState.combo) * 0.002f);

                    comboState.comboCountdown.time = za::min(comboState.comboCountdown.time,
                                                             pt->psvComboStartTime.currentValue() * 100.f);

                    comboState.combo = za::min(comboState.combo, 998);
                }
            }
        }
        else
        {
            comboState.combo = 1;
        }

        const MoneyType
            reward = computeFinalReward(/* bubble     */ *firstClickedBubble,
                                        /* multiplier */ 1.f,
                                        /* comboMult  */ getComboValueMult(comboState.combo, pt->laserPopEnabled ? playerComboDecayLaser : playerComboDecay),
                                        /* popperCat  */ nullptr);

        popWithRewardAndReplaceBubble({
            .reward          = reward,
            .bubble          = *firstClickedBubble,
            .xCombo          = comboState.combo,
            .popSoundOverlap = true,
            .popperCat       = nullptr,
            .multiPop        = false,
        });

        if (!pt->speedrunStartTime.hasValue())
            pt->speedrunStartTime.emplace(za::Clock::now());

        if (pt->multiPopEnabled && !pt->laserPopEnabled)
            forEachBubbleInRadius(clickPos,
                                  pt->psvPPMultiPopRange.currentValue(),
                                  [&](Bubble& otherBubble)
            {
                if (&otherBubble == firstClickedBubble)
                    return ControlFlow::Continue;

                popWithRewardAndReplaceBubble({
                    .reward          = reward,
                    .bubble          = otherBubble,
                    .xCombo          = comboState.combo,
                    .popSoundOverlap = false,
                    .popperCat       = nullptr,
                    .multiPop        = true,
                });

                return ControlFlow::Continue;
            });
    }

    return firstClickedBubble != nullptr;
}


void Main::gameLoopUpdateCatActionNormal(const float /* deltaTimeMs */, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    const auto maxCooldown = pt->getComputedCooldownByCatType(cat.type);
    const auto range       = pt->getComputedRangeByCatType(cat.type);
    const auto [cx, cy]    = getCatRangeCenter(cat);

    const auto normalCatPopBubble = [&](Bubble& bubble)
    {
        cat.pawPosition = bubble.position;
        cat.pawOpacity  = 255.f;
        cat.pawRotation = (bubble.position - cat.position).angle() + za::degrees(45);

        const float squaredMouseCatRange = pt->getComputedSquaredRangeByCatType(CatType::Mouse);

        const Cat* mouseCat = getMouseCat();
        const Cat* copyCat  = getCopyCat();

        const bool inMouseCatRange = mouseCat != nullptr &&
                                     (mouseCat->position - cat.position).lengthSquared() <= squaredMouseCatRange;

        const bool inCopyMouseCatRange = copyCat != nullptr && pt->copycatCopiedCatType == CatType::Mouse &&
                                         (copyCat->position - cat.position).lengthSquared() <= squaredMouseCatRange;

        const int comboMult = (inMouseCatRange || inCopyMouseCatRange) ? pt->mouseCatCombo : 1;

        const MoneyType reward = computeFinalReward(/* bubble     */ bubble,
                                                    /* multiplier */ 1.f,
                                                    /* comboMult  */ getComboValueMult(comboMult, mouseCatComboDecay),
                                                    /* popperCat  */ &cat);

        popWithRewardAndReplaceBubble({
            .reward          = reward,
            .bubble          = bubble,
            .xCombo          = comboMult,
            .popSoundOverlap = true,
            .popperCat       = &cat,
            .multiPop        = false,
        });

        cat.textStatusShakeEffect.bump(rngFast, 1.5f);
        ++cat.hits;

        cat.cooldown.time = maxCooldown;
    };

    // Combo bubbles are player-only; cats must never target or pop them.
    const auto pickRandomNonCombo = [&] -> Bubble*
    {
        return pickRandomBubbleInRadiusMatching({cx, cy}, range, [&](const Bubble& b) {
            return b.type != BubbleType::Combo;
        });
    };

    if (!pt->perm.smartCatsPurchased)
    {
        if (Bubble* b = pickRandomNonCombo())
            normalCatPopBubble(*b);

        return;
    }

    const auto pickAny = [&](const auto... types) -> Bubble*
    {
        return pickRandomBubbleInRadiusMatching({cx, cy}, range, [&](const Bubble& b) {
            return (... || (b.type == types));
        });
    };

    if (!pt->perm.geniusCatsPurchased)
    {
        if (Bubble* specialBubble = pickAny(BubbleType::Nova, BubbleType::Star, BubbleType::Bomb))
            normalCatPopBubble(*specialBubble);
        else if (Bubble* b = pickRandomNonCombo())
            normalCatPopBubble(*b);

        return;
    }

    if (Bubble* bBomb = pickAny(BubbleType::Bomb); bBomb != nullptr && !pt->geniusCatIgnoreBubbles.bomb)
        normalCatPopBubble(*bBomb);
    else if (Bubble* bNova = pickAny(BubbleType::Nova); bNova != nullptr && !pt->geniusCatIgnoreBubbles.star)
        normalCatPopBubble(*bNova);
    else if (Bubble* bStar = pickAny(BubbleType::Star); bStar != nullptr && !pt->geniusCatIgnoreBubbles.star)
        normalCatPopBubble(*bStar);
    else if (Bubble* bNormal = pickAny(BubbleType::Normal); bNormal != nullptr && !pt->geniusCatIgnoreBubbles.normal)
        normalCatPopBubble(*bNormal);
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActionUni(const float deltaTimeMs, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    const auto starBubbleType = isUnicatTranscendenceActive() ? BubbleType::Nova : BubbleType::Star;

    constexpr float freezeMs = 450.f;

    const auto transformBubble = [&](Bubble& bToTransform)
    {
        // Freeze the bubble in mid-air and spawn inward-converging star
        // particles; the actual type swap runs when `pendingTransformMs`
        // ticks to zero in `gameLoopUpdateBubbles`, so the transformation
        // reads as "particles finish falling in → bubble pops into a star".
        // Storing the state on the bubble itself means it survives
        // saves/loads and doesn't rely on any index or pointer being stable.
        constexpr float convergeRadius = 60.f;
        const float     convergeSpeed  = convergeRadius / freezeMs;

        bToTransform.velocity                   = {0.f, 0.f};
        bToTransform.pendingTransformMs         = freezeMs;
        bToTransform.pendingTransformTargetType = static_cast<za::U8>(asIdx(starBubbleType));
        bToTransform.pendingTransformCatIdx     = 0xFF'FFu;
        bToTransform.rotation                   = za::positiveRemainder(bToTransform.rotation + deltaTimeMs, za::tau);

        for (SizeT i = 0u; i < 8u; ++i)
        {
            const float     angle   = rngFast.getF(0.f, za::tau);
            const za::Vec2f outward = {za::cos(angle), za::sin(angle)};

            spawnParticle({.position      = bToTransform.position + outward * convergeRadius * rngFast.getF(0.8f, 1.2f),
                           .velocity      = -outward * convergeSpeed,
                           .scale         = rngFast.getF(0.14f, 0.26f) * 0.25f,
                           .scaleDecay    = -0.0003f,
                           .accelerationY = 0.f,
                           .opacity       = 1.f,
                           .opacityDecay  = 1.f / freezeMs * 0.9f,
                           .rotation      = rngFast.getF(0.f, za::tau),
                           .torque        = rngFast.getF(-0.003f, 0.003f)},
                          /* hue */ 0.f,
                          ParticleType::Star);
        }

        ++cat.hits;
    };

    const auto maxCooldown = pt->getComputedCooldownByCatType(cat.type);
    const auto range       = pt->getComputedRangeByCatType(cat.type);

    if (pt->perm.unicatTranscendenceAOEPurchased)
    {
        Bubble* firstBubble = nullptr;

        forEachBubbleInRadius(getCatRangeCenter(cat),
                              range,
                              [&](Bubble& bubble)
        {
            if (bubble.type != BubbleType::Normal || bubble.pendingTransformMs > 0.f)
                return ControlFlow::Continue;

            if (firstBubble == nullptr)
                firstBubble = &bubble;

            transformBubble(bubble);
            return ControlFlow::Continue;
        });

        if (firstBubble == nullptr)
            return;

        cat.pawPosition = exponentialApproach(cat.pawPosition,
                                              firstBubble->position.addY(firstBubble->radius - 5.f),
                                              deltaTimeMs,
                                              5.f);
        cat.pawOpacity  = 255.f;
        cat.pawRotation = (firstBubble->position - cat.position).angle() + za::degrees(45);
        cat.pawHoldMs   = 250.f;

        sounds.shine2.settings.position = {firstBubble->position.x, firstBubble->position.y};
        playSound(sounds.shine2);
    }
    else
    {
        Bubble* b = pickRandomBubbleInRadiusMatching(getCatRangeCenter(cat), range, [&](const Bubble& bubble) {
            return bubble.type == BubbleType::Normal && bubble.pendingTransformMs <= 0.f;
        });

        if (b == nullptr)
            return;

        transformBubble(*b);

        cat.pawPosition = exponentialApproach(cat.pawPosition, b->position.addY(b->radius - 5.f), deltaTimeMs, 5.f);
        cat.pawOpacity  = 255.f;
        cat.pawRotation = (b->position - cat.position).angle() + za::degrees(45);
        cat.pawHoldMs   = 250.f;

        sounds.shine.settings.position = {b->position.x, b->position.y};
        playSound(sounds.shine);
    }

    cat.textStatusShakeEffect.bump(rngFast, 1.5f);
    cat.cooldown.time = maxCooldown;
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActionDevil(const float deltaTimeMs, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    const auto maxCooldown = pt->getComputedCooldownByCatType(cat.type);
    const auto range       = pt->getComputedRangeByCatType(cat.type);

    if (!isDevilcatHellsingedActive())
    {
        Bubble* b = pickRandomBubbleInRadiusMatching(getCatRangeCenter(cat), range, [&](const Bubble& bubble) {
            return bubble.type != BubbleType::Combo && bubble.pendingTransformMs <= 0.f;
        });
        if (b == nullptr)
            return;

        Bubble& bubble = *b;

        // Freeze the bubble in mid-air and spawn inward-converging fire
        // particles; the type swap + bomb credit happen in
        // `gameLoopUpdateBubbles` when the timer reaches zero. Same shape as
        // Unicat's transform so the feel matches.
        constexpr float freezeMs       = 450.f;
        constexpr float convergeRadius = 60.f;
        const float     convergeSpeed  = convergeRadius / freezeMs;

        const auto catIdx = static_cast<za::SizeT>(&cat - pt->cats.data());

        bubble.velocity                   = {0.f, 0.f};
        bubble.pendingTransformMs         = freezeMs;
        bubble.pendingTransformTargetType = static_cast<za::U8>(asIdx(BubbleType::Bomb));
        bubble.pendingTransformCatIdx     = static_cast<za::U16>(catIdx);
        bubble.rotation                   = za::positiveRemainder(bubble.rotation + deltaTimeMs, za::tau);

        for (SizeT i = 0u; i < 8u; ++i)
        {
            const float     angle   = rngFast.getF(0.f, za::tau);
            const za::Vec2f outward = {za::cos(angle), za::sin(angle)};

            spawnParticle({.position      = bubble.position + outward * convergeRadius * rngFast.getF(0.8f, 1.2f),
                           .velocity      = -outward * convergeSpeed,
                           .scale         = rngFast.getF(0.14f, 0.26f) * 0.25f,
                           .scaleDecay    = -0.0003f,
                           .accelerationY = 0.f,
                           .opacity       = 1.f,
                           .opacityDecay  = 1.f / freezeMs * 0.9f,
                           .rotation      = rngFast.getF(0.f, za::tau),
                           .torque        = rngFast.getF(-0.003f, 0.003f)},
                          /* hue */ 0.f,
                          ParticleType::Fire);
        }

        cat.pawOpacity  = 255.f;
        cat.pawRotation = (bubble.position - cat.position).angle() - za::degrees(45);

        sounds.makeBomb.settings.position = {bubble.position.x, bubble.position.y};
        playSound(sounds.makeBomb);
    }
    else
    {
        const auto portalPos = getCatRangeCenter(cat);

        pt->hellPortals.pushBack({
            .position = portalPos,
            .life     = Countdown{.time = 1750.f},
            .catIdx   = static_cast<za::SizeT>(&cat - pt->cats.data()),
        });

        sounds.makeBomb.settings.position = {portalPos.x, portalPos.y};
        playSound(sounds.portalon);
    }

    cat.textStatusShakeEffect.bump(rngFast, 1.5f);
    ++cat.hits;

    cat.cooldown.time = maxCooldown;
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActionAstro(const float /* deltaTimeMs */, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    const auto [cx, cy] = getCatRangeCenter(cat);

    if (cat.astroState.hasValue() || pt->disableAstrocatFlight)
        return;

    sounds.launch.settings.position = {cx, cy};
    playSound(sounds.launch);

    ++cat.hits;
    cat.astroState.emplace(/* startX */ cat.position.x, /* velocityX */ 0.f, /* wrapped */ false);
    --cat.position.x;
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActionWarden(const float /* deltaTimeMs */, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    const auto maxCooldown  = getComputedCooldownByCatTypeOrCopyCat(cat.type);
    const auto range        = getComputedRangeByCatTypeOrCopyCat(cat.type);
    const auto rangeSquared = range * range;

    // Skip if a previous bonk is still resolving -- prevents stacking actions.
    if (cat.wardenBonk.hasValue())
        return;

    // Pick the most-deeply-asleep cat in range (favors the one closest to
    // fully napping, so the warden keeps the dorm orderly).
    Cat*  bestTarget    = nullptr;
    SizeT bestTargetIdx = 0u;
    float bestScore     = -1.f;

    for (SizeT i = 0u; i < pt->cats.size(); ++i)
    {
        Cat& otherCat = pt->cats[i];

        if (&otherCat == &cat)
            continue;

        if (!otherCat.isNapping())
            continue;

        if (otherCat.napTransition->value < 0.9f)
            continue;

        // Already in the wake-fade -- don't double-trigger.
        if (otherCat.napTransition->reversed)
            continue;

        if ((otherCat.position - cat.position).lengthSquared() > rangeSquared)
            continue;

        const float score = otherCat.napTransition->value;
        if (score > bestScore)
        {
            bestScore     = score;
            bestTarget    = &otherCat;
            bestTargetIdx = i;
        }
    }

    if (bestTarget == nullptr)
        return;

    // Start the windup: emplace the bonk-state container in Windup phase
    // and remember the target so the strike fires when the phase countdown
    // expires (in the per-frame paw update). All start/end poses are
    // derived on the fly per phase, so no snapshots are stored.
    auto& bonk   = cat.wardenBonk.emplace();
    bonk.phase   = Cat::WardenBonkState::Phase::Windup;
    bonk.phaseMs = gameConstants.wardenCatBatonWindupMs;
    bonk.pendingTargetIdx.emplace(bestTargetIdx);

    cat.pawOpacity    = 255.f;
    cat.cooldown.time = maxCooldown;
}


////////////////////////////////////////////////////////////
void Main::resolveWardenBonkStrike(Cat& cat)
{
    if (!cat.wardenBonk.hasValue() || !cat.wardenBonk->pendingTargetIdx.hasValue())
    {
        // Bad call site (no state to resolve). Drop the container if it
        // exists so the per-frame tick doesn't keep hitting this path.
        cat.wardenBonk.reset();
        return;
    }

    const SizeT targetIdx = *cat.wardenBonk->pendingTargetIdx;
    cat.wardenBonk->pendingTargetIdx.reset();

    // Target invalidated during the windup (cats vector mutated, target
    // already woke up, hexed away, ...): abort the bonk cleanly. Without
    // this reset we'd softlock -- phase stays Windup, phaseMs stays 0, and
    // the per-frame tick would call us again every frame forever.
    const bool targetGone = targetIdx >= pt->cats.size() || &pt->cats[targetIdx] == &cat ||
                            !pt->cats[targetIdx].isNapping() || pt->cats[targetIdx].napTransition->reversed;

    if (targetGone)
    {
        cat.wardenBonk.reset();
        return;
    }

    Cat& target = pt->cats[targetIdx];

    // Wake them up: kick the fade backwards and clear the sleep countdown.
    target.napTransition->reversed = true;
    target.napSleepCountdown.reset();
    target.napShakeProgress = 0.f;

    // Forced wake-up (wardencat bonk) grants the Power Nap cooldown boost.
    // TODO P0: apply only on bonk and rename to "rude awakening"
    applyPowerNapBoost(target);

    sounds.bonk.settings.position = {target.position.x, target.position.y};
    playSound(sounds.bonk, /* maxOverlap */ 4u);

    sounds.napWake.settings.position = {target.position.x, target.position.y};
    playSound(sounds.napWake, /* maxOverlap */ 2u);

    if (profile.showTextParticles)
    {
        auto& tp = textParticles.emplaceBack(TextParticle{
            {.position      = target.position.addY(-40.f),
             .velocity      = rngFast.getVec2f({-0.1f, -1.65f}, {0.1f, -1.35f}) * 0.395f,
             .scale         = 0.8f,
             .scaleDecay    = 0.f,
             .accelerationY = -0.0035f,
             .opacity       = 1.f,
             .opacityDecay  = 0.0025f,
             .rotation      = 0.f,
             .torque        = rngFast.getF(-0.002f, 0.002f)}});

        tp.velocity *= 0.5f;
        tp.opacityDecay *= 0.5f;
        tp.accelerationY *= 0.35f;
        tp.scale *= 1.3f;

        (void)std::snprintf(tp.buffer, sizeof(tp.buffer), "BONK!");
    }

    // Set the strike target pose; the renderer's Travel branch lerps from
    // the (recomputed) windup-end pose toward this. The renderer's Hold
    // branch stays parked here, and the Return branch lerps from here back
    // toward idle.
    cat.pawPosition = target.position - za::Vec2f{0.f, 35.f};
    cat.pawOpacity  = 255.f;
    cat.pawRotation = (target.position - cat.position).angle() + za::degrees(45);

    // Advance to the Travel phase. Subsequent phase transitions
    // (Travel → Hold → Return → reset) happen in the per-frame paw update.
    cat.wardenBonk->phase   = Cat::WardenBonkState::Phase::Travel;
    cat.wardenBonk->phaseMs = gameConstants.wardenCatBatonTravelMs;

    // Pendulum-style impact reaction on the target cat: rocks side-to-side
    // for ~500ms, synced roughly with the baton reaching the target.
    target.bonkImpactMs = 500.f;

    cat.textStatusShakeEffect.bump(rngFast, 1.5f);
    ++cat.hits;
}


////////////////////////////////////////////////////////////
[[nodiscard]] Cat* Main::getSessionTargetCat(const HexSession& session) const
{
    if (session.catIdx >= pt->cats.size())
        return nullptr;

    Cat& cat = pt->cats[session.catIdx];

    // Guard against index aliasing during prestige transition (cats get
    // swap-to-back popped): a live session must point at an actually hexed cat.
    if (!cat.isHexedOrCopyHexed())
        return nullptr;

    return &cat;
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool Main::anyCatHexedOrCopyHexed() const
{
    return za::anyOf(pt->cats.begin(), pt->cats.end(), [](const Cat& cat) { return cat.isHexedOrCopyHexed(); });
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool Main::canHexMore() const
{
    return pt->hexSessions.size() < maxConcurrentHexes;
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool Main::canCopyHexMore() const
{
    return pt->copyHexSessions.size() < maxConcurrentHexes;
}


////////////////////////////////////////////////////////////
void Main::hexCat(Cat& cat, const SizeT /* catIdx */, const bool copy)
{
    if (isCatBeingDragged(cat))
        stopDraggingCat(cat);

    sounds.soulsteal.settings.position = {cat.position.x, cat.position.y};
    playSound(sounds.soulsteal);

    screenShakeAmount = 3.5f;
    screenShakeTimer  = 600.f;

    (copy ? cat.hexedCopyTimer : cat.hexedTimer).emplace(Transition{});

    cat.wobbleRadians = 0.f;

    spawnParticle({.position      = cat.getDrawPosition(profile.enableCatBobbing).addY(29.f),
                   .velocity      = {0.f, 0.f},
                   .scale         = 0.2f,
                   .scaleDecay    = 0.f,
                   .accelerationY = -0.00015f,
                   .opacity       = 1.f,
                   .opacityDecay  = 0.0002f,
                   .rotation      = 0.f,
                   .torque        = rngFast.getF(-0.0002f, 0.0002f)},
                  /* hue */ 0.f,
                  ParticleType::CatSoul);
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool Main::canCatNap(const Cat& cat) const
{
    if (cat.isNapping())
        return false;

    if (cat.isHexedOrCopyHexed())
        return false;

    // Witchcat: refuse nap while any hex sessions are active or while the
    // ritual is winding up / cast (cooldown in the ritual band).
    if (cat.type == CatType::Witch)
        if (!pt->hexSessions.empty() || cat.cooldown.time <= 10'000.f)
            return false;

    // Copycat mimicking the witch: same guards against copy-side rituals.
    if (cat.type == CatType::Copy && pt->copycatCopiedCatType == CatType::Witch)
        if (!pt->copyHexSessions.empty() || cat.cooldown.time <= 10'000.f)
            return false;

    return true;
}


////////////////////////////////////////////////////////////
void Main::beginCatNap(Cat& cat, const float sleepDurationMs)
{
    if (isCatBeingDragged(cat))
        stopDraggingCat(cat);

    cat.napTransition.emplace(Transition{});

    // A non-positive duration means "sleep indefinitely until shaken awake":
    // leave `napSleepCountdown` unset so the natural-wakeup branch never fires.
    if (sleepDurationMs > 0.f)
        cat.napSleepCountdown.emplace(Countdown{.time = sleepDurationMs});
    else
        cat.napSleepCountdown.reset();

    cat.napShakeProgress = 0.f;
    cat.napWakeWobble    = 0.f;
}


////////////////////////////////////////////////////////////
void Main::applyPowerNapBoost(Cat& cat)
{
    if (!pt->perm.powerNapPurchased)
        return;

    cat.napBoostCountdown.time = pt->psvPPPowerNapDuration.currentValue();
    cat.napBoostMultiplier     = 1.f + pt->psvPPPowerNapStrength.currentValue();
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateNapScheduler(const float deltaTimeMs)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    if (inPrestigeTransition || splashCountdown.time > 0.f)
        return;

    // Once the player has prestiged at least once, the nap system skips both
    // the "need 3 cats" minimum and the scripted tutorial -- random naps kick
    // in right away.
    const bool prestigedBefore = pt->psvBubbleValue.nPurchases > 0u;

    if (!prestigedBefore)
    {
        if (pt->cats.size() < 3u)
            return;

        // One-shot tutorial nap: 5s after first reaching 3 cats, put the
        // oldest cat to sleep indefinitely and display a tip.
        if (!pt->scriptedNapDone)
        {
            if (!pt->scriptedNapPendingCountdown.hasValue())
                pt->scriptedNapPendingCountdown.emplace(Countdown{.time = 5'000.f});

            if (pt->scriptedNapPendingCountdown->tick(deltaTimeMs) == TickResult::JustFinished)
            {
                pt->scriptedNapPendingCountdown.reset();
                pt->scriptedNapDone = true;

                if (!pt->cats.empty() && canCatNap(pt->cats[0]))
                    beginCatNap(pt->cats[0], /* indefinite */ -1.f);

                if (!pt->napTipShown)
                {
                    pt->napTipShown = true;
                    doTip("One of your cats is taking a nap!\nGrab it and shake it around to wake it up.");
                }
            }

            // Don't start random naps until the scripted one has run.
            return;
        }
    }

    // Per-cat scheduler: each cat owns its own countdown that ticks down
    // independently and triggers a nap when it expires. The countdown resets
    // to a fresh random value in [120s, 300s].
    constexpr float minNextNapMs = 120'000.f;
    constexpr float maxNextNapMs = 300'000.f;

    for (Cat& cat : pt->cats)
    {
        // Lazy-init: brand-new cats have a 0 countdown; pick one on first
        // visit so they don't all fire on the same frame.
        if (cat.napScheduleCountdownMs <= 0.f)
            cat.napScheduleCountdownMs = rng.getF(minNextNapMs, maxNextNapMs);

        // Pause the schedule countdown while the cat is napping or otherwise
        // ineligible -- it'll resume once the cat is awake again.
        if (!canCatNap(cat))
            continue;

        cat.napScheduleCountdownMs -= deltaTimeMs;

        if (cat.napScheduleCountdownMs > 0.f)
            continue;

        cat.napScheduleCountdownMs = rng.getF(minNextNapMs, maxNextNapMs);
        beginCatNap(cat, /* sleepDurationMs */ 20'000.f);
    }
}


void Main::gameLoopUpdateCatActionWitchImpl(const float /* deltaTimeMs */,
                                            Cat&                    cat,
                                            za::Vector<HexSession>& sessionsToUse,
                                            const SizeT             nCatsToHex)
{
    const auto maxCooldown = getComputedCooldownByCatTypeOrCopyCat(cat.type);
    const auto range       = getComputedRangeByCatTypeOrCopyCat(cat.type);

    const bool copy = &sessionsToUse == &pt->copyHexSessions;

    // Collect all eligible candidates in range.
    za::Vector<SizeT> candidateIndices;

    for (SizeT i = 0u; i < pt->cats.size(); ++i)
    {
        const Cat& otherCat = pt->cats[i];

        if (otherCat.type == CatType::Duck)
            continue;

        if (otherCat.type == CatType::Witch)
            continue;

        if (otherCat.type == CatType::Copy && pt->copycatCopiedCatType == CatType::Witch)
            continue;

        if ((otherCat.position - cat.position).length() > range)
            continue;

        if (otherCat.isHexedOrCopyHexed())
            continue;

        candidateIndices.pushBack(i);
    }

    const SizeT otherCatCount = candidateIndices.size();

    if (otherCatCount == 0u)
    {
        wastedEffort      = true;
        cat.cooldown.time = maxCooldown;
        return;
    }

    // Partial Fisher-Yates shuffle to pick `nToHex` distinct random indices.
    const SizeT nToHex = za::min(nCatsToHex, otherCatCount);

    for (SizeT i = 0u; i < nToHex; ++i)
    {
        const auto j = rng.getI<SizeT>(i, otherCatCount - 1);
        if (j != i)
        {
            const SizeT tmp     = candidateIndices[i];
            candidateIndices[i] = candidateIndices[j];
            candidateIndices[j] = tmp;
        }
    }

    float buffPower = pt->psvPPWitchCatBuffDuration.currentValue();

    if (pt->perm.witchCatBuffPowerScalesWithNCats)
        buffPower += za::ceil(za::pow(static_cast<float>(otherCatCount), 0.9f)) * 0.5f;

    if (pt->perm.witchCatBuffPowerScalesWithMapSize)
    {
        const float nMapExtensions = (pt->mapPurchased ? 1.f : 0.f) + static_cast<float>(pt->psvMapExtension.nPurchases);

        buffPower += static_cast<float>(nMapExtensions) * 0.75f;
    }

    const auto nDollsToSpawn = za::max(SizeT{2u},
                                       static_cast<SizeT>(buffPower * (pt->perm.witchCatBuffFewerDolls ? 1.f : 2.f) / 4.f));

    const auto isPositionFarFromOtherDolls = [&](const za::Vec2f position) -> bool
    {
        for (const HexSession& s : sessionsToUse)
            for (const Doll& d : s.dolls)
                if ((d.position - position).lengthSquared() < (256.f * 256.f))
                    return false;

        return true;
    };

    const auto isOnTopOfAnyCat = [&](const za::Vec2f position) -> bool
    {
        for (const Cat& c : pt->cats)
            if ((c.position - position).lengthSquared() < c.getRadiusSquared())
                return true;

        return false;
    };

    const auto isOnTopOfAnyShrine = [&](const za::Vec2f position) -> bool
    {
        for (const Shrine& s : pt->shrines)
            if ((s.position - position).lengthSquared() < s.getRadiusSquared())
                return true;

        return false;
    };

    const auto rndDollPosition = [&]
    {
        constexpr float offset = 64.f;
        return rng.getVec2f({offset, offset}, {pt->getMapLimit() - offset - uiWindowWidth, boundaries.y - offset});
    };

    const auto pickDollPosition = [&]
    {
        constexpr unsigned int maxRetries = 16u;

        for (unsigned int retryCount = 0; retryCount < maxRetries; ++retryCount)
            if (const auto candidate = rndDollPosition();
                isPositionFarFromOtherDolls(candidate) && !isOnTopOfAnyCat(candidate) && !isOnTopOfAnyShrine(candidate))
                return candidate;

        return rndDollPosition();
    };

    for (SizeT s = 0u; s < nToHex; ++s)
    {
        const SizeT selectedIdx = candidateIndices[s];
        Cat&        selected    = pt->cats[selectedIdx];

        hexCat(selected, selectedIdx, copy);
        statRitual(selected.type);

        auto& session  = sessionsToUse.emplaceBack();
        session.catIdx = selectedIdx;
        session.dolls.reserve(nDollsToSpawn);

        for (SizeT i = 0u; i < nDollsToSpawn; ++i)
        {
            auto& d = session.dolls.emplaceBack(
                Doll{.position      = pickDollPosition(),
                     .wobbleRadians = rng.getF(0.f, za::tau),
                     .buffPower     = buffPower,
                     .catType       = selected.type == CatType::Copy ? pt->copycatCopiedCatType : selected.type,
                     .tcActivation  = {.duration = rng.getF(300.f, 600.f) * static_cast<float>(i + 1)},
                     .tcDeath       = {}});

            d.tcActivation.restart();
        }

        spawnParticlesWithHue(copy ? 180.f : 0.f, 128, selected.position, ParticleType::Hex, 0.5f, 0.35f);
    }

    cat.textStatusShakeEffect.bump(rngFast, 1.5f);
    cat.hits += 1u;

    if (!pt->dollTipShown)
    {
        pt->dollTipShown = true;
        doTip("Click on all the dolls to\nreceive a powerful timed buff!\nYou might need to scroll...");
    }

    cat.cooldown.time = maxCooldown;
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActionWitch(const float deltaTimeMs, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    ZA_ASSERT(canHexMore());
    gameLoopUpdateCatActionWitchImpl(deltaTimeMs, cat, pt->hexSessions, /* nCatsToHex */ 1u); // TODO P1: add PP for more cats to hex at once
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActionWizard(const float deltaTimeMs, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    if (!pt->absorbingWisdom)
        return;

    const auto maxCooldown  = getComputedCooldownByCatTypeOrCopyCat(cat.type);
    const auto range        = getComputedRangeByCatTypeOrCopyCat(cat.type);
    const auto [cx, cy]     = getCatRangeCenter(cat);
    const auto drawPosition = cat.getDrawPosition(profile.enableCatBobbing);

    Bubble* starBubble = nullptr;

    const auto findRotatedStarBubble = [&](Bubble& bubble) // TODO P0: change this to something sensible
    {
        if ((bubble.type != BubbleType::Star && bubble.type != BubbleType::Nova) || bubble.rotation == 0.f)
            return ControlFlow::Continue;

        starBubble = &bubble;
        return ControlFlow::Break;
    };

    forEachBubbleInRadius({cx, cy}, range, findRotatedStarBubble);

    if (starBubble == nullptr)
        starBubble = pickRandomBubbleInRadiusMatching({cx, cy}, range, [&](Bubble& bubble) {
            return bubble.type == BubbleType::Star || bubble.type == BubbleType::Nova;
        });

    if (starBubble == nullptr)
        return;

    Bubble& bubble = *starBubble;

    cat.pawPosition = bubble.position;
    cat.pawOpacity  = 255.f;
    cat.pawRotation = (bubble.position - cat.position).angle() + za::degrees(45);

    bubble.rotation += deltaTimeMs * 0.025f; // TODO P0: change this to something sensible
    spawnParticlesWithHue(230.f, 1, bubble.position, ParticleType::Star, 0.5f, 0.35f);

    if (bubble.rotation >= za::tau)
    {
        const auto wisdomReward = pt->getComputedRewardByBubbleType(bubble.type);

        if (profile.showTextParticles)
        {
            auto& tp = makeRewardTextParticle(drawPosition);
            std::snprintf(tp.buffer, sizeof(tp.buffer), "+%llu WP", wisdomReward);
        }

        sounds.absorb.settings.position = {bubble.position.x, bubble.position.y};
        playSound(sounds.absorb, /* maxOverlap */ 1u);

        spawnParticlesWithHue(230.f, 16, bubble.position, ParticleType::Star, 0.5f, 0.35f);

        pt->wisdom += wisdomReward;
        turnBubbleInto(bubble, BubbleType::Normal);

        cat.cooldown.time = maxCooldown;

        statAbsorbedStarBubble();
    }
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActionMouse(const float /* deltaTimeMs */, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    const auto maxCooldown = getComputedCooldownByCatTypeOrCopyCat(cat.type);
    const auto range       = getComputedRangeByCatTypeOrCopyCat(cat.type);

    Bubble* b = pickRandomBubbleInRadiusMatching(cat.position, range, [&](const Bubble& bubble) {
        return bubble.type != BubbleType::Combo;
    });
    if (b == nullptr)
        return;

    Bubble& bubble = *b;

    cat.pawPosition = bubble.position;
    cat.pawOpacity  = 255.f;

    addCombo(pt->mouseCatCombo, pt->mouseCatComboCountdown);
    pt->mouseCatCombo = za::min(pt->mouseCatCombo, 999); // cap at 999x

    const auto savedBubblePos = bubble.position;

    const MoneyType reward = computeFinalReward(/* bubble     */ bubble,
                                                /* multiplier */ pt->mouseCatCombo == 999 ? 5.f : 1.f,
                                                /* comboMult  */ getComboValueMult(pt->mouseCatCombo, mouseCatComboDecay),
                                                /* popperCat  */ &cat);

    popWithRewardAndReplaceBubble({
        .reward          = reward,
        .bubble          = bubble,
        .xCombo          = pt->mouseCatCombo,
        .popSoundOverlap = true,
        .popperCat       = &cat,
        .multiPop        = false,
    });

    if (pt->multiPopMouseCatEnabled)
        forEachBubbleInRadius(savedBubblePos,
                              pt->psvPPMultiPopRange.currentValue(),
                              [&](Bubble& otherBubble)
        {
            if (&otherBubble == &bubble)
                return ControlFlow::Continue;

            if (otherBubble.type == BubbleType::Combo)
                return ControlFlow::Continue;

            popWithRewardAndReplaceBubble({
                .reward          = reward,
                .bubble          = otherBubble,
                .xCombo          = pt->mouseCatCombo,
                .popSoundOverlap = false,
                .popperCat       = &cat,
                .multiPop        = true,
            });

            return ControlFlow::Continue;
        });

    cat.textStatusShakeEffect.bump(rngFast, 1.5f);
    ++cat.hits;

    cat.cooldown.time = maxCooldown;
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActionEngi(const float /* deltaTimeMs */, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    const auto maxCooldown  = getComputedCooldownByCatTypeOrCopyCat(cat.type);
    const auto range        = getComputedRangeByCatTypeOrCopyCat(cat.type);
    const auto rangeSquared = range * range;

    SizeT nCatsHit = 0u;

    for (Cat& otherCat : pt->cats)
    {
        if (otherCat.type == CatType::Engi)
            continue;

        if ((otherCat.position - cat.position).lengthSquared() > rangeSquared)
            continue;

        ++nCatsHit;

        spawnParticles(8, otherCat.getDrawPosition(profile.enableCatBobbing), ParticleType::Cog, 0.25f, 0.5f);

        sounds.maintenance.settings.position = {otherCat.position.x, otherCat.position.y};
        playSound(sounds.maintenance, /* maxOverlap */ 1u);

        otherCat.boostCountdown.time = 1500.f;
    }

    if (nCatsHit > 0)
    {
        cat.textStatusShakeEffect.bump(rngFast, 1.5f);
        cat.hits += static_cast<za::U32>(nCatsHit);

        statMaintenance(nCatsHit);
        statHighestSimultaneousMaintenances(nCatsHit);
    }

    cat.cooldown.time = maxCooldown;
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActionRepulso(const float /* deltaTimeMs */, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    const auto maxCooldown = getComputedCooldownByCatTypeOrCopyCat(cat.type);
    const auto range       = getComputedRangeByCatTypeOrCopyCat(cat.type);

    if (pt->repulsoCatConverterEnabled && !pt->repulsoCatIgnoreBubbles.normal)
    {
        Bubble* b = pickRandomBubbleInRadiusMatching(cat.position, range, [&](Bubble& bubble) {
            return bubble.type != BubbleType::Star && bubble.type != BubbleType::Nova && bubble.type != BubbleType::Combo;
        });

        if (b != nullptr && rng.getF(0.f, 100.f) < pt->psvPPRepulsoCatConverterChance.currentValue())
        {
            turnBubbleInto(*b, pt->perm.repulsoCatNovaConverterPurchased ? BubbleType::Nova : BubbleType::Star);

            spawnParticles(2, b->position, ParticleType::Star, 0.5f, 0.35f);

            cat.textStatusShakeEffect.bump(rngFast, 1.5f);
            ++cat.hits;
        }
    }

    cat.cooldown.time = maxCooldown;
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActionAttracto(const float /* deltaTimeMs */, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    const auto maxCooldown = getComputedCooldownByCatTypeOrCopyCat(cat.type);

    // TODO P1: ? maybe absorb all bubbles in range and give a reward based on the number of bubbles absorbed

    cat.cooldown.time = maxCooldown;
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActionCopy(const float deltaTimeMs, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    if (pt->copycatCopiedCatType == CatType::Witch)
    {
        ZA_ASSERT(canCopyHexMore());
        gameLoopUpdateCatActionWitchImpl(deltaTimeMs, cat, pt->copyHexSessions, /* nCatsToHex */ 1u);
    }
    else if (pt->copycatCopiedCatType == CatType::Wizard)
        gameLoopUpdateCatActionWizard(deltaTimeMs, cat);
    else if (pt->copycatCopiedCatType == CatType::Mouse)
        gameLoopUpdateCatActionMouse(deltaTimeMs, cat);
    else if (pt->copycatCopiedCatType == CatType::Engi)
        gameLoopUpdateCatActionEngi(deltaTimeMs, cat);
    else if (pt->copycatCopiedCatType == CatType::Repulso)
        gameLoopUpdateCatActionRepulso(deltaTimeMs, cat);
    else if (pt->copycatCopiedCatType == CatType::Attracto)
        gameLoopUpdateCatActionAttracto(deltaTimeMs, cat);
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActionDuck(const float deltaTimeMs, Cat& cat)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    (void)deltaTimeMs;
    (void)cat;
}


////////////////////////////////////////////////////////////
[[nodiscard]] float Main::getWindRepulsionMult() const
{
    constexpr float mults[4] = {1.f, 5.f, 10.f, 15.f};
    return mults[pt->windStrength];
}


////////////////////////////////////////////////////////////
[[nodiscard]] float Main::getWindAttractionMult() const
{
    constexpr float mults[4] = {1.f, 1.5f, 3.f, 4.5f};
    return mults[pt->windStrength];
}


////////////////////////////////////////////////////////////
auto Main::makeMagnetAction(
    const za::Vec2f    position,
    const CatType      catType,
    const float        deltaTimeMs,
    auto               countdownPm,
    const float        countdownTime,
    const float        strengthMult,
    const float        direction,
    BubbleIgnoreFlags& ignoreFlags)
{
    return [this, &ignoreFlags, deltaTimeMs, position, catType, countdownPm, countdownTime, strengthMult, direction](
               Bubble& bubble)
    {
        if (bubble.type == BubbleType::Combo)
            return ControlFlow::Continue;

        if (ignoreFlags.normal && bubble.type == BubbleType::Normal)
            return ControlFlow::Continue;

        if (ignoreFlags.star && (bubble.type == BubbleType::Star || bubble.type == BubbleType::Nova))
            return ControlFlow::Continue;

        if (ignoreFlags.bomb && bubble.type == BubbleType::Bomb)
            return ControlFlow::Continue;

        const auto bcDiff   = (position - bubble.position);
        const auto strength = (getComputedRangeByCatTypeOrCopyCat(catType) - bcDiff.length()) * 0.000017f;
        bubble.velocity += (bcDiff.normalized() * strength * strengthMult) * direction * deltaTimeMs;

        (bubble.*countdownPm).time = za::max((bubble.*countdownPm).time, countdownTime);
        return ControlFlow::Continue;
    };
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatActions(const float deltaTimeMs)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    (void)wizardcatSpin.tick(deltaTimeMs * 0.015f);

    for (SizeT catIdx = 0u; catIdx < pt->cats.size(); ++catIdx)
    {
        Cat& cat = pt->cats[catIdx];

        // Keep cat in boundaries
        const float catRadius = cat.getRadius();

        // Keep cats away from shrine of clicking
        // Buff cats in shrine of automation
        for (const Shrine& shrine : pt->shrines)
        {
            if (shrine.type == ShrineType::Clicking && shrine.isActive() && cat.type != CatType::Mouse)
            {
                const auto diff = (shrine.position - cat.position);
                if (diff.lengthSquared() < shrine.getRangeSquared() * 1.35f)
                {
                    const auto strength = (shrine.getRange() * 1.35f - diff.length()) * 0.00125f * deltaTimeMs;
                    cat.position -= diff.normalized() * strength;
                }
            }
            else if (shrine.type == ShrineType::Automation && shrine.isActive())
            {
                if (shrine.isInRange(cat.position))
                    cat.boostCountdown.time = 250.f;
            }
            else if (shrine.type == ShrineType::Voodoo && shrine.isActive())
            {
                if (shrine.isInRange(cat.position) && getWitchCat() == nullptr && !anyCatHexedOrCopyHexed() &&
                    !cat.isHexedOrCopyHexed())
                {
                    hexCat(cat, catIdx, /* copy */ false);
                }
            }
        }

        const bool allowOOBCat = &cat == playerInputState.catToPlace || cat.astroState.hasValue() ||
                                 (playerInputState.draggedCats.size() > 1u && isCatBeingDragged(cat));

        if (!allowOOBCat)
            cat.position = cat.position.componentWiseClamp({catRadius, catRadius},
                                                           {pt->getMapLimit() - catRadius, boundaries.y - catRadius});

        const auto maxCooldown  = pt->getComputedCooldownByCatType(cat.type);
        const auto range        = pt->getComputedRangeByCatType(cat.type);
        const auto rangeSquared = range * range;

        const auto drawPosition = cat.getDrawPosition(profile.enableCatBobbing);

        if (cat.bonkImpactMs > 0.f)
            cat.bonkImpactMs = za::max(0.f, cat.bonkImpactMs - deltaTimeMs);

        // Wardencat baton sequence: tick the current phase's countdown; on
        // expiry, advance to the next phase (Windup → Travel → Hold →
        // Return → reset). Done BEFORE the pawHoldMs branch so the wardencat
        // never falls through to the generic per-frame lerp while a bonk is
        // in progress.
        if (cat.wardenBonk.hasValue())
        {
            using Phase = Cat::WardenBonkState::Phase;

            // Self-healing: clear any inconsistent state so a single bad
            // frame can't softlock the cat forever
            const bool stuck = cat.type != CatType::Warden ||
                               (cat.wardenBonk->phase == Phase::Windup && !cat.wardenBonk->pendingTargetIdx.hasValue());
            if (stuck)
            {
                cat.wardenBonk.reset();
                continue;
            }

            auto& bonk = *cat.wardenBonk;

            bonk.phaseMs = za::max(0.f, bonk.phaseMs - deltaTimeMs);

            if (bonk.phaseMs <= 0.f)
            {
                switch (bonk.phase)
                {
                    case Phase::Windup:
                        // Strike sets phase = Travel + phaseMs = travel ms.
                        resolveWardenBonkStrike(cat);
                        break;
                    case Phase::Travel:
                        bonk.phase   = Phase::Hold;
                        bonk.phaseMs = gameConstants.wardenCatBatonHoldMs;
                        break;
                    case Phase::Hold:
                        bonk.phase   = Phase::Return;
                        bonk.phaseMs = gameConstants.wardenCatBatonReturnMs;
                        break;
                    case Phase::Return:
                        cat.wardenBonk.reset();
                        break;
                }
            }
        }
        else if (cat.pawHoldMs > 0.f)
        {
            // Generic paw-hold (Uni transform etc.) -- suspends the lerp
            // below for non-warden uses.
            cat.pawHoldMs -= deltaTimeMs;
        }
        else
        {
            auto diff = cat.pawPosition - drawPosition - za::Vec2f{-30.f, 30.f};
            cat.pawPosition -= diff * 0.01f * deltaTimeMs;
            cat.pawRotation = cat.pawRotation.rotatedTowards(za::degrees(-45.f), deltaTimeMs * 0.005f);

            if (isCatBeingDragged(cat) && (cat.pawPosition - drawPosition).length() > 16.f)
                cat.pawPosition = drawPosition + (cat.pawPosition - drawPosition).normalized() * 16.f;

            if (cat.cooldown.time == 0.f && cat.pawOpacity > 10.f)
            {
                cat.pawOpacity -= 0.5f * deltaTimeMs;
                cat.pawOpacity = za::max(cat.pawOpacity, 0.f);
            }
        }

        // Spawn effect
        const auto seStatus = cat.spawnEffectTimer.advance(deltaTimeMs * 0.002f);
        if (seStatus == TickResult::Running)
            continue;

        if (seStatus == TickResult::JustFinished)
        {
            spawnParticles(4, cat.position, ParticleType::Star, 0.5f, 0.75f);
        }

        cat.update(deltaTimeMs);

        // Per-frame kinematics derived from the change in position this frame.
        // Dragging runs before this pass, so `frameDelta.x` here is the drag
        // velocity when the cat is being dragged.
        const za::Vec2f frameDelta = cat.position - cat.lastFramePosition;
        cat.lastFramePosition      = cat.position;

        // Velocity-based body tilt: positive vx tilts the body over, easing
        // back to neutral when the player slows down or releases the grip.
        {
            constexpr float tiltPerVelocity = 0.25f; // radians per (px/ms)
            constexpr float maxTilt         = 0.55f;

            if (isCatBeingDragged(cat))
            {
                const float vx      = frameDelta.x / za::max(deltaTimeMs, 0.001f);
                const float target  = za::clamp(vx * tiltPerVelocity, -maxTilt, maxTilt);
                cat.dragTiltRadians = exponentialApproach(cat.dragTiltRadians, target, deltaTimeMs, 40.f);
            }
            else
            {
                cat.dragTiltRadians = exponentialApproach(cat.dragTiltRadians, 0.f, deltaTimeMs, 80.f);
            }
        }

        if (cat.isHexedOrCopyHexed())
        {
            const auto res = cat.getHexedTimer()->tick(deltaTimeMs * 0.001f);

            if (cat.getHexedTimer()->reversed && res == TickResult::JustFinished)
                cat.getHexedTimer().reset();
        }

        if (cat.isNapping())
        {
            auto& t = *cat.napTransition;

            // 500ms fade in/out.
            (void)t.tick(deltaTimeMs * 0.002f);

            const bool fullyAsleep = !t.reversed && t.value >= 1.f;

            if (fullyAsleep && cat.napSleepCountdown.hasValue())
                if (cat.napSleepCountdown->tick(deltaTimeMs) == TickResult::JustFinished)
                {
                    t.reversed = true;
                    cat.napSleepCountdown.reset();
                }

            // Shake-to-wake: accumulate drag motion while being dragged; drain
            // slowly otherwise so the player has to keep shaking.
            constexpr float requiredShakeDistance = 500.f;
            constexpr float perFrameDeltaCap      = 50.f; // ignore teleport-sized moves
            constexpr float idleDrainPerMs        = 1.f / 1'500.f;

            if (isCatBeingDragged(cat))
            {
                const float moved = za::min(frameDelta.length(), perFrameDeltaCap);

                cat.napShakeProgress = za::clamp(cat.napShakeProgress + moved / requiredShakeDistance, 0.f, 1.f);

                // Motion-driven wobble feedback.
                if (moved > 2.f)
                    cat.napWakeWobble = rngFast.getF(-0.35f, 0.35f) * za::min(moved / 20.f, 1.f);

                if (cat.napShakeProgress >= 1.f && fullyAsleep)
                {
                    t.reversed = true;
                    cat.napSleepCountdown.reset();
                    cat.napShakeProgress = 0.f;

                    // Forced wake-up (shake) grants the Power Nap cooldown boost.
                    applyPowerNapBoost(cat);

                    // sounds.napWake.settings.position = {cat.position.x, cat.position.y};
                    // playSound(sounds.napWake, /* maxOverlap */ 2u);
                }
            }
            else
            {
                cat.napShakeProgress = za::max(0.f, cat.napShakeProgress - idleDrainPerMs * deltaTimeMs);
            }

            // Wobble decays back to zero.
            cat.napWakeWobble *= za::pow(0.001f, deltaTimeMs * 0.001f);

            if (t.reversed && t.value <= 0.f)
            {
                cat.napTransition.reset();
                cat.napSleepCountdown.reset();
                cat.napShakeProgress = 0.f;
                cat.napWakeWobble    = 0.f;

                pt->anyCatEverWokenFromNap = true;

                // Trigger a yawn right as the cat finishes waking up, and push
                // the next natural yawn far out so it doesn't double-fire.
                cat.yawnAnimCountdown.time = 75.f * static_cast<float>(MainAtlasRects::nYawnRects);
                cat.yawnCountdown.time     = rngFast.getF(15'000.f, 25'000.f);
            }

            // Continuous per-cat sleep breath: each cat owns its own retrigger
            // countdown and refreshes itself when it elapses, so multiple
            // napping cats can play their own instance simultaneously.
            cat.sleepSoundCountdownMs -= deltaTimeMs;

            if (cat.isNapping() && t.value > 0.3f && cat.sleepSoundCountdownMs <= 0.f)
            {
                sounds.sleep.settings.position = {cat.position.x, cat.position.y};
                playSound(sounds.sleep, /* maxOverlap */ 8u);

                cat.sleepSoundCountdownMs = static_cast<float>(sounds.sleep.buffer.getDuration().asMilliseconds());
            }

            // Spawn a floating 'Z' once the eyes are mostly shut.
            if (cat.isNapping() && t.value > 0.3f && rngFast.getF(0.f, 1.f) < deltaTimeMs * 0.003f)
            {
                auto& tp = textParticles.emplaceBack(TextParticle{
                    {.position      = cat.getDrawPosition(profile.enableCatBobbing).addY(-24.f),
                     .velocity      = {rngFast.getF(-0.03f, 0.03f), rngFast.getF(-0.10f, -0.06f)},
                     .scale         = 0.5f,
                     .scaleDecay    = 0.f,
                     .accelerationY = 0.f,
                     .opacity       = 0.9f,
                     .opacityDecay  = 0.0008f,
                     .rotation      = 0.f,
                     .torque        = rngFast.getF(-0.0015f, 0.0015f)}});
                (void)std::snprintf(tp.buffer, sizeof(tp.buffer), "Z");
            }

            // Napping cats do not take any action.
            continue;
        }

        const auto doWitchBehavior = [&](const float hueMod, auto& soundRitual, auto& soundRitualEnd)
        {
            if (cat.cooldown.time < 100.f)
            {
                soundManager.stopPlayingAll(soundRitual);

                if (soundManager.countPlayingPooled(soundRitualEnd) == 0u)
                {
                    soundRitualEnd.settings.position = {cat.position.x, cat.position.y};

                    if (profile.playWitchRitualSounds)
                        playSound(soundRitualEnd);
                }
            }

            if (cat.cooldown.time < 10'000.f)
            {
                if (cat.cooldown.time > 100.f && soundManager.countPlayingPooled(soundRitual) == 0u)
                {
                    soundRitual.settings.position = {cat.position.x, cat.position.y};

                    if (profile.playWitchRitualSounds)
                        playSound(soundRitual);
                }

                const float intensity = remap(cat.cooldown.time, 0.f, 10'000.f, 1.f, 0.f);

                for (Cat& otherCat : pt->cats)
                {
                    if (otherCat.isHexedOrCopyHexed())
                        continue;

                    if (&otherCat == &cat)
                        cat.hue = za::sin(za::remainder(cat.cooldown.time / remap(cat.cooldown.time, 0.f, 10'000.f, 15.f, 150.f),
                                                        za::tau)) *
                                  50.f * intensity;

                    const auto diff2 = otherCat.position - cat.position;

                    if (diff2.lengthSquared() > rangeSquared)
                        continue;

                    if (rngFast.getF(0.f, 1.f) < intensity)
                        spawnParticle({.position   = otherCat.getDrawPosition(profile.enableCatBobbing) +
                                                     za::Vec2f{rngFast.getF(-catRadius, +catRadius), catRadius - 9.f},
                                       .velocity   = rngFast.getVec2f({-0.05f, -0.05f}, {0.05f, 0.05f}),
                                       .scale      = rngFast.getF(0.08f, 0.27f) * 0.5f,
                                       .scaleDecay = 0.f,
                                       .accelerationY = -0.0017f,
                                       .opacity       = 1.f,
                                       .opacityDecay  = rngFast.getF(0.00035f, 0.0025f),
                                       .rotation      = rngFast.getF(0.f, za::tau),
                                       .torque        = rngFast.getF(-0.002f, 0.002f)},
                                      /* hue */ wrapHue(rngFast.getF(-50.f, 50.f) + hueMod),
                                      ParticleType::Hex);
                }
            }
            else
            {
                soundManager.stopPlayingAll(sounds.ritual);
            }
        };

        if (cat.type == CatType::Witch && canHexMore())
            doWitchBehavior(/* hueMod */ 0.f, sounds.ritual, sounds.ritualend);

        if (cat.type == CatType::Copy && pt->copycatCopiedCatType == CatType::Witch && canCopyHexMore())
            doWitchBehavior(/* hueMod */ 180.f, sounds.copyritual, sounds.copyritualend);

        if (cat.hexedTimer.hasValue() || (cat.type == CatType::Witch && !pt->hexSessions.empty()))
        {
            if (rngFast.getI(0, 10) > 5)
                spawnParticle({.position = drawPosition + za::Vec2f{rngFast.getF(-catRadius, +catRadius), catRadius - 9.f},
                               .velocity      = rngFast.getVec2f({-0.05f, -0.05f}, {0.05f, 0.05f}),
                               .scale         = rngFast.getF(0.08f, 0.27f) * 0.5f,
                               .scaleDecay    = 0.f,
                               .accelerationY = -0.0017f,
                               .opacity       = 0.5f,
                               .opacityDecay  = rngFast.getF(0.00035f, 0.0025f) * 0.6f,
                               .rotation      = rngFast.getF(0.f, za::tau),
                               .torque        = rngFast.getF(-0.002f, 0.002f)},
                              /* hue */ wrapHue(rngFast.getF(-50.f, 50.f)),
                              ParticleType::Hex);

            continue;
        }

        if (cat.hexedCopyTimer.hasValue() ||
            (cat.type == CatType::Copy && pt->copycatCopiedCatType == CatType::Witch && !pt->copyHexSessions.empty()))
        {
            if (rngFast.getI(0, 10) > 5)
                spawnParticle({.position = drawPosition + za::Vec2f{rngFast.getF(-catRadius, +catRadius), catRadius - 9.f},
                               .velocity      = rngFast.getVec2f({-0.05f, -0.05f}, {0.05f, 0.05f}),
                               .scale         = rngFast.getF(0.08f, 0.27f) * 0.5f,
                               .scaleDecay    = 0.f,
                               .accelerationY = -0.0017f,
                               .opacity       = 0.5f,
                               .opacityDecay  = rngFast.getF(0.00035f, 0.0025f) * 0.6f,
                               .rotation      = rngFast.getF(0.f, za::tau),
                               .torque        = rngFast.getF(-0.002f, 0.002f)},
                              /* hue */ wrapHue(rngFast.getF(-50.f, 50.f) + 180.f),
                              ParticleType::Hex);

            continue;
        }

        if (pt->buffCountdownsPerType[asIdx(CatType::Normal)].time > 0.f && cat.pawOpacity >= 75.f)
        {
            spawnParticle({.position      = cat.pawPosition + rngFast.getVec2f({-12.f, -12.f}, {12.f, 12.f}),
                           .velocity      = rngFast.getVec2f({-0.015f, -0.015f}, {0.015f, 0.015f}),
                           .scale         = rngFast.getF(0.08f, 0.27f) * 0.1f,
                           .scaleDecay    = 0.f,
                           .accelerationY = 0.f,
                           .opacity       = 1.f,
                           .opacityDecay  = rngFast.getF(0.00025f, 0.0015f) * 1.5f,
                           .rotation      = rngFast.getF(0.f, za::tau),
                           .torque        = rngFast.getF(-0.002f, 0.002f)},
                          /* hue */ 0.f,
                          ParticleType::Star);
        }


        const auto [cx, cy] = getCatRangeCenter(cat);

        if (cat.inspiredCountdown.time > 0.f && rngFast.getF(0.f, 1.f) > 0.5f)
        {
            spawnParticle({.position      = drawPosition + za::Vec2f{rngFast.getF(-catRadius, +catRadius), catRadius},
                           .velocity      = rngFast.getVec2f({-0.05f, -0.05f}, {0.05f, 0.05f}),
                           .scale         = rngFast.getF(0.08f, 0.27f) * 0.2f,
                           .scaleDecay    = 0.f,
                           .accelerationY = -0.002f,
                           .opacity       = 1.f,
                           .opacityDecay  = rngFast.getF(0.00025f, 0.0015f),
                           .rotation      = rngFast.getF(0.f, za::tau),
                           .torque        = rngFast.getF(-0.002f, 0.002f)},
                          /* hue */ 0.f,
                          ParticleType::Star);
        }

        const float globalBoost = pt->buffCountdownsPerType[asIdx(CatType::Engi)].time;
        if ((globalBoost > 0.f || cat.boostCountdown.time > 0.f) && rngFast.getF(0.f, 1.f) > 0.75f)
        {
            spawnParticle({.position = drawPosition + za::Vec2f{rngFast.getF(-catRadius, +catRadius), catRadius - 25.f},
                           .velocity = rngFast.getVec2f({-0.025f, -0.015f}, {0.025f, 0.015f}),
                           .scale    = rngFast.getF(0.08f, 0.27f) * 0.15f,
                           .scaleDecay    = 0.f,
                           .accelerationY = -0.0015f,
                           .opacity       = 1.f,
                           .opacityDecay  = rngFast.getF(0.00055f, 0.0045f),
                           .rotation      = rngFast.getF(0.f, za::tau),
                           .torque        = rngFast.getF(-0.002f, 0.002f)},
                          /* hue */ 180.f,
                          ParticleType::Cog);
        }

        // Power Nap boost: mirror the engicat cog-spawn pattern, using Star
        // particles as a placeholder visual.
        // TODO P2: swap ParticleType::Star for a dedicated "Z" / dream-themed
        // particle type once that asset exists.
        if (cat.napBoostCountdown.time > 0.f && rngFast.getF(0.f, 1.f) > 0.75f)
        {
            spawnParticle({.position = drawPosition + za::Vec2f{rngFast.getF(-catRadius, +catRadius), catRadius - 25.f},
                           .velocity = rngFast.getVec2f({-0.025f, -0.015f}, {0.025f, 0.015f}),
                           .scale    = rngFast.getF(0.08f, 0.27f) * 0.2f,
                           .scaleDecay    = 0.f,
                           .accelerationY = -0.0015f,
                           .opacity       = 1.f,
                           .opacityDecay  = rngFast.getF(0.00055f, 0.0045f),
                           .rotation      = rngFast.getF(0.f, za::tau),
                           .torque        = rngFast.getF(-0.002f, 0.002f)},
                          /* hue */ 210.f,
                          ParticleType::Star);
        }

        if (cat.type == CatType::Uni)
            cat.hue += deltaTimeMs * (isUnicatTranscendenceActive() ? 0.25f : 0.1f);

        if (cat.type == CatType::Devil && isDevilcatHellsingedActive())
        {
            if (rngFast.getF(0.f, 1.f) > 0.75f)
            {
                spawnParticle({.position = drawPosition +
                                           za::Vec2f{rngFast.getF(-catRadius + 15.f, +catRadius - 5.f), catRadius - 20.f},
                               .velocity      = rngFast.getVec2f({-0.025f, -0.015f}, {0.025f, 0.015f}),
                               .scale         = rngFast.getF(0.08f, 0.27f) * 0.55f,
                               .scaleDecay    = -0.00025f,
                               .accelerationY = -0.0015f,
                               .opacity       = 1.f,
                               .opacityDecay  = rngFast.getF(0.00055f, 0.0045f),
                               .rotation      = rngFast.getF(0.f, za::tau),
                               .torque        = rngFast.getF(-0.002f, 0.002f)},
                              /* hue */ 0.f,
                              ParticleType::Fire2);

                spawnParticle({.position      = drawPosition + za::Vec2f{-52.f * 0.2f, -85.f * 0.2f},
                               .velocity      = rngFast.getVec2f({-0.025f, -0.015f}, {0.025f, 0.015f}),
                               .scale         = rngFast.getF(0.08f, 0.27f) * 0.55f,
                               .scaleDecay    = -0.00025f,
                               .accelerationY = -0.0015f,
                               .opacity       = 1.f,
                               .opacityDecay  = rngFast.getF(0.00055f, 0.0045f) * 2.f,
                               .rotation      = rngFast.getF(0.f, za::tau),
                               .torque        = rngFast.getF(-0.002f, 0.002f)},
                              /* hue */ 0.f,
                              ParticleType::Fire2);

                spawnParticle({.position      = drawPosition + za::Vec2f{-140.f * 0.2f, -90.f * 0.2f},
                               .velocity      = rngFast.getVec2f({-0.025f, -0.015f}, {0.025f, 0.015f}),
                               .scale         = rngFast.getF(0.08f, 0.27f) * 0.55f,
                               .scaleDecay    = -0.00025f,
                               .accelerationY = -0.0015f,
                               .opacity       = 1.f,
                               .opacityDecay  = rngFast.getF(0.00055f, 0.0045f) * 2.f,
                               .rotation      = rngFast.getF(0.f, za::tau),
                               .torque        = rngFast.getF(-0.002f, 0.002f)},
                              /* hue */ 0.f,
                              ParticleType::Fire2);
            }
        }

        if (cat.type == CatType::Astro && cat.astroState.hasValue())
        {
            auto& [startX, velocityX, particleTimer, wrapped] = *cat.astroState;

            particleTimer += deltaTimeMs;

            if (particleTimer >= 3.f && !cat.isCloseToStartX())
            {
                sounds.rocket.settings.position = {cx, cy};
                playSound(sounds.rocket, /* maxOverlap */ 1u);

                spawnParticles(1, drawPosition + za::Vec2f{46.f, 55.f}, ParticleType::Fire, 1.6f, 0.25f, 0.65f);

                if (rngFast.getI(0, 10) > 5)
                    spawnParticle(ParticleData{.position      = drawPosition + za::Vec2f{46.f, 55.f},
                                               .velocity      = {rngFast.getF(-0.15f, 0.15f), rngFast.getF(0.f, 0.1f)},
                                               .scale         = rngFast.getF(0.75f, 1.f) * 0.4f,
                                               .scaleDecay    = -0.00025f,
                                               .accelerationY = -0.00017f,
                                               .opacity       = 0.5f,
                                               .opacityDecay  = rngFast.getF(0.00065f, 0.00075f),
                                               .rotation      = rngFast.getF(0.f, za::tau),
                                               .torque        = rngFast.getF(-0.002f, 0.002f)},
                                  0.f,
                                  ParticleType::Smoke);

                particleTimer = 0.f;
            }

            if (frameProcThisFrame)
                forEachBubbleInRadius({cx, cy},
                                      range,
                                      [&](Bubble& bubble)
                {
                    if (bubble.type == BubbleType::Combo)
                        return ControlFlow::Continue;

                    const MoneyType reward = computeFinalReward(/* bubble     */ bubble,
                                                                /* multiplier */ 20.f,
                                                                /* comboMult  */ 1.f,
                                                                /* popperCat  */ &cat);

                    statFlightRevenue(reward);

                    if (bubble.type == BubbleType::Bomb)
                        pt->achAstrocatPopBomb = true;

                    popWithRewardAndReplaceBubble({
                        .reward          = reward,
                        .bubble          = bubble,
                        .xCombo          = 1,
                        .popSoundOverlap = rngFast.getF(0.f, 1.f) > 0.75f,
                        .popperCat       = &cat,
                        .multiPop        = false,
                    });

                    cat.textStatusShakeEffect.bump(rngFast, 1.5f);

                    return ControlFlow::Continue;
                });

            if (!cat.isCloseToStartX() && velocityX > -5.f)
                velocityX -= 0.00025f * deltaTimeMs;

            if (!cat.isCloseToStartX())
                cat.position.x += velocityX * deltaTimeMs;
            else
                cat.position.x = exponentialApproach(cat.position.x, startX - 10.f, deltaTimeMs, 500.f);

            if (!wrapped && cat.position.x + catRadius < 0.f)
            {
                cat.position.x = pt->getMapLimit() + catRadius;

                if (pt->buffCountdownsPerType[asIdx(CatType::Astro)].time == 0.f) // loop if astro buff active
                    wrapped = true;
            }

            if (wrapped && cat.position.x <= startX)
            {
                cat.astroState.reset();
                cat.position.x    = startX;
                cat.cooldown.time = maxCooldown;
            }

            continue;
        }

        if (cat.type == CatType::Wizard)
        {
            if (pt->absorbingWisdom)
            {
                static float absorbSin = 0.f;
                absorbSin += deltaTimeMs * 0.002f;

                cat.hue = wrapHue(za::sin(za::remainder(absorbSin, za::tau)) * 25.f);

                if (wizardcatAbsorptionRotation < 0.15f)
                    wizardcatAbsorptionRotation += deltaTimeMs * 0.0005f;
            }
            else
            {
                cat.hue = 0.f;

                if (wizardcatAbsorptionRotation > 0.f)
                    wizardcatAbsorptionRotation -= deltaTimeMs * 0.0005f;
            }

            if (isWizardBusy() && rngFast.getF(0.f, 1.f) > 0.5f)
            {
                spawnParticle({.position   = drawPosition + za::Vec2f{rngFast.getF(-catRadius, +catRadius), catRadius},
                               .velocity   = rngFast.getVec2f({-0.05f, -0.05f}, {0.05f, 0.05f}),
                               .scale      = rngFast.getF(0.08f, 0.27f) * 0.2f,
                               .scaleDecay = 0.f,
                               .accelerationY = -0.002f,
                               .opacity       = 1.f,
                               .opacityDecay  = rngFast.getF(0.00025f, 0.0015f),
                               .rotation      = rngFast.getF(0.f, za::tau),
                               .torque        = rngFast.getF(-0.002f, 0.002f)},
                              /* hue */ 225.f,
                              ParticleType::Star);
            }
        }

        if (cat.type == CatType::Mouse || (cat.type == CatType::Copy && pt->copycatCopiedCatType == CatType::Mouse))
        {
            for (const Cat& otherCat : pt->cats)
            {
                if (otherCat.type != CatType::Normal)
                    continue;

                if ((otherCat.position - cat.position).lengthSquared() > rangeSquared)
                    continue;

                if (rngFast.getF(0.f, 1.f) > 0.95f)
                    spawnParticle({.position      = otherCat.getDrawPosition(profile.enableCatBobbing) +
                                                    za::Vec2f{rngFast.getF(-catRadius, +catRadius), catRadius - 25.f},
                                   .velocity      = rngFast.getVec2f({-0.01f, -0.05f}, {0.01f, 0.05f}),
                                   .scale         = rngFast.getF(0.08f, 0.27f) * 0.4f,
                                   .scaleDecay    = 0.f,
                                   .accelerationY = -0.00015f,
                                   .opacity       = 1.f,
                                   .opacityDecay  = rngFast.getF(0.0003f, 0.002f),
                                   .rotation      = -0.6f,
                                   .torque        = 0.f},
                                  /* hue */ 0.f,
                                  ParticleType::Cursor);
            }
        }

        if (cat.type == CatType::Repulso || (cat.type == CatType::Copy && pt->copycatCopiedCatType == CatType::Repulso))
            forEachBubbleInRadius(cat.position,
                                  pt->getComputedRangeByCatType(CatType::Repulso),
                                  makeMagnetAction(cat.position,
                                                   cat.type,
                                                   deltaTimeMs,
                                                   &Bubble::repelledCountdown,
                                                   1500.f,
                                                   getWindRepulsionMult(),
                                                   -1.f,
                                                   pt->repulsoCatIgnoreBubbles));

        if (cat.type == CatType::Attracto || (cat.type == CatType::Copy && pt->copycatCopiedCatType == CatType::Attracto))
            forEachBubbleInRadius(cat.position,
                                  pt->getComputedRangeByCatType(CatType::Attracto),
                                  makeMagnetAction(cat.position,
                                                   cat.type,
                                                   deltaTimeMs,
                                                   &Bubble::attractedCountdown,
                                                   750.f,
                                                   getWindAttractionMult(),
                                                   1.f,
                                                   pt->attractoCatIgnoreBubbles));

        if (isCatBeingDragged(cat))
            continue;

        const float globalBoostMult = globalBoost > 0.f ? 2.f : 1.f;
        if (!cat.updateCooldown(deltaTimeMs * globalBoostMult))
            continue;

        using FnPtr = void (Main::*)(const float, Cat&);

        const FnPtr fnPtrs[]{
            &Main::gameLoopUpdateCatActionNormal,
            &Main::gameLoopUpdateCatActionUni,
            &Main::gameLoopUpdateCatActionDevil,
            &Main::gameLoopUpdateCatActionAstro,
            &Main::gameLoopUpdateCatActionWarden,

            &Main::gameLoopUpdateCatActionWitch,
            &Main::gameLoopUpdateCatActionWizard,
            &Main::gameLoopUpdateCatActionMouse,
            &Main::gameLoopUpdateCatActionEngi,
            &Main::gameLoopUpdateCatActionRepulso,
            &Main::gameLoopUpdateCatActionAttracto,
            &Main::gameLoopUpdateCatActionCopy,
            &Main::gameLoopUpdateCatActionDuck,
        };

        static_assert(za::getArraySize(fnPtrs) == nCatTypes);

        (this->*fnPtrs[asIdx(cat.type)])(deltaTimeMs, cat);
    }
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool Main::isCatDraggable(const Cat& cat) const
{
    if (cat.isAstroAndInFlight())
        return false;

    if (cat.isHexedOrCopyHexed())
        return false;

    if (cat.type == CatType::Witch && !pt->hexSessions.empty())
        return false;

    if (cat.type == CatType::Copy && pt->copycatCopiedCatType == CatType::Witch && !pt->copyHexSessions.empty())
        return false;

    if (cat.type == CatType::Witch && cat.cooldown.time <= 10'000.f)
        return false;

    if (cat.type == CatType::Copy && pt->copycatCopiedCatType == CatType::Witch && cat.cooldown.time <= 10'000.f)
        return false;

    return true;
}


[[nodiscard]] bool Main::isAOESelecting() const
{
    return (keyDown(za::Keyboard::Key::LShift) || keyDown(za::Keyboard::Key::LControl)) &&
           mBtnDown(getLMB(), /* penetrateUI */ true);
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCatDragging(const float deltaTimeMs, const SizeT countFingersDown, const za::Vec2f mousePos)
{
    const bool dragInputHeld = mBtnDown(getLMB(), /* penetrateUI */ true) || countFingersDown == 1u;

    if (inPrestigeTransition)
    {
        resetAllDraggedCats();
        return;
    }

    // Automatically scroll when dragging cats near the edge of the screen
    if ((!playerInputState.draggedCats.empty() || isAOESelecting()) && playerInputState.catToPlace == nullptr)
    {
        constexpr float offset = 48.f;

        if (mousePos.x < particleCullingBoundaries.left + offset * 1.f)
            playerInputState.scroll -= 8.f;
        else if (mousePos.x < particleCullingBoundaries.left + offset * 2.f)
            playerInputState.scroll -= 4.f;
        else if (mousePos.x < particleCullingBoundaries.left + offset * 3.f)
            playerInputState.scroll -= 2.f;

        if (mousePos.x > particleCullingBoundaries.right - offset * 1.f)
            playerInputState.scroll += 8.f;
        else if (mousePos.x > particleCullingBoundaries.right - offset * 2.f)
            playerInputState.scroll += 4.f;
        else if (mousePos.x > particleCullingBoundaries.right - offset * 3.f)
            playerInputState.scroll += 2.f;
    }

    if (isAOESelecting())
    {
        if (!playerInputState.catDragOrigin.hasValue())
            playerInputState.catDragOrigin.emplace(mousePos);
    }
    else if (playerInputState.catDragOrigin.hasValue())
    {
        const auto dragRect = getAoEDragRect(mousePos).value();
        playerInputState.catDragOrigin.reset();
        playerInputState.draggedCats.clear();
        playerInputState.draggedCatsStartedWithTouch        = false;
        playerInputState.draggedCatsStartedFromAOESelection = true;

        for (Cat& cat : pt->cats)
        {
            if (!isCatDraggable(cat))
                continue;

            if (!dragRect.contains(cat.position))
                continue;

            playerInputState.draggedCats.pushBack(&cat);
        }

        playSound(sounds.grab);
    }
    else
    {
        const bool shouldDropCats = [&]
        {
            if (playerInputState.catToPlace != nullptr)
                return bubbleCullingBoundaries.isInside(playerInputState.catToPlace->position) &&
                       inputHelper.wasMouseButtonJustPressed(getLMB());

            if (playerInputState.draggedCats.empty())
                return false;

            if (playerInputState.draggedCatsStartedWithTouch)
                return countFingersDown != 1u;

            if (playerInputState.draggedCatsStartedFromAOESelection)
                return inputHelper.wasMouseButtonJustPressed(getLMB());

            return !dragInputHeld;
        }();

        if (shouldDropCats)
        {
            if (!playerInputState.draggedCats.empty())
                playSound(sounds.drop);

            resetAllDraggedCats();
            return;
        }

        if (!playerInputState.draggedCats.empty())
        {
            const auto pivotCatIdx = pickDragPivotCatIndex();
            Cat&       pivotCat    = *playerInputState.draggedCats[pivotCatIdx];

            static thread_local za::Vector<za::Vec2f> relativeCatPositions;
            relativeCatPositions.clear();
            relativeCatPositions.reserve(playerInputState.draggedCats.size());

            for (const Cat* cat : playerInputState.draggedCats)
                relativeCatPositions.pushBack(cat->position - pivotCat.position);

            pivotCat.position = exponentialApproach(pivotCat.position, mousePos + za::Vec2f{-10.f, 13.f}, deltaTimeMs, 25.f);

            for (za::SizeT i = 0u; i < playerInputState.draggedCats.size(); ++i)
            {
                if (i == pivotCatIdx)
                    continue;

                playerInputState.draggedCats[i]->position = pivotCat.position + relativeCatPositions[i];
            }

            return;
        }

        if (ImGui::GetIO().WantCaptureMouse || !particleCullingBoundaries.isInside(mousePos))
        {
            resetAllDraggedCats();
            return;
        }

        if (!dragInputHeld)
        {
            playerInputState.catDragPressDuration = 0.f;
            return;
        }

        Cat* hoveredCat = nullptr;

        // Only check for hover targets during initial press phase
        if (playerInputState.catDragPressDuration <= profile.catDragPressDuration)
            for (Cat& cat : pt->cats)
            {
                if (!isCatDraggable(cat))
                    continue;

                if ((mousePos - cat.position).lengthSquared() > cat.getRadiusSquared())
                    continue;

                hoveredCat = &cat;
            }

        if (hoveredCat == nullptr)
        {
            playerInputState.catDragPressDuration = 0.f;
            return;
        }

        if (hoveredCat)
        {
            playerInputState.catDragPressDuration += deltaTimeMs;

            if (playerInputState.catDragPressDuration >= profile.catDragPressDuration)
            {
                playerInputState.draggedCats.clear();
                playerInputState.draggedCats.pushBack(hoveredCat);
                playerInputState.draggedCatsStartedWithTouch        = countFingersDown == 1u;
                playerInputState.draggedCatsStartedFromAOESelection = false;

                if (hoveredCat->type == CatType::Duck)
                {
                    sounds.quack.settings.position = {hoveredCat->position.x, hoveredCat->position.y};
                    playSound(sounds.quack);
                }
                else
                    playSound(sounds.grab);
            }
        }
    }
}


////////////////////////////////////////////////////////////
[[nodiscard]] constexpr CatType Main::shrineTypeToCatType(const ShrineType shrineType)
{
    return static_cast<CatType>(asIdx(shrineType) + asIdx(CatType::Witch));
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateShrines(const float deltaTimeMs)
{
    for (SizeT i = 0u; i < pt->psvShrineActivation.nPurchases; ++i)
    {
        for (Shrine& shrine : pt->shrines)
        {
            if (shrine.tcActivation.hasValue() || shrine.type != static_cast<ShrineType>(i))
                continue;

            shrine.tcActivation.emplace(TimedCountdown{.duration = 2000.f});
            shrine.tcActivation->restart();

            sounds.earthquakeFast.settings.position = {shrine.position.x, shrine.position.y};
            playSound(sounds.earthquakeFast);

            screenShakeAmount = 4.5f;
            screenShakeTimer  = 1000.f;
        }
    }

    // Should only be triggered in testing via cheats
    for (SizeT i = pt->psvShrineActivation.nPurchases; i < nShrineTypes; ++i)
        for (Shrine& shrine : pt->shrines)
            if (shrine.type == static_cast<ShrineType>(i))
                shrine.tcActivation.reset();

    for (Shrine& shrine : pt->shrines)
    {
        if (shrine.tcActivation.hasValue())
        {
            const auto cdStatus = shrine.tcActivation->tick(deltaTimeMs);

            if (cdStatus == TickResult::Running)
            {
                spawnParticlesWithHue(wrapHue(shrine.getHue() + 40.f),
                                      static_cast<SizeT>(1 + 12 * (1.f - shrine.tcActivation->asProgress().getElapsed())),
                                      shrine.getDrawPosition() + rngFast.getVec2f({-1.f, -1.f}, {1.f, 1.f}) * 32.f,
                                      ParticleType::Fire,
                                      rngFast.getF(0.25f, 1.f),
                                      0.75f);

                spawnParticlesWithHue(shrine.getHue(),
                                      static_cast<SizeT>(4 + 36 * (1.f - shrine.tcActivation->asProgress().getElapsed())),
                                      shrine.getDrawPosition() + rngFast.getVec2f({-1.f, -1.f}, {1.f, 1.f}) * 32.f,
                                      ParticleType::Shrine,
                                      rngFast.getF(0.35f, 1.2f),
                                      0.5f);
            }
            else if (cdStatus == TickResult::JustFinished)
            {
                playSound(sounds.woosh);

                const auto asCatType = asIdx(shrineTypeToCatType(shrine.type));
                if (asCatType >= asIdx(CatType::Count))
                    return;

                if (pt->perm.unsealedByType[asCatType])
                    spawnSpecialCat(shrine.position, static_cast<CatType>(asCatType));
            }
        }

        shrine.update(deltaTimeMs);

        if (!shrine.isActive())
            continue;

        forEachBubbleInRadius(shrine.position,
                              shrine.getRange(),
                              [&](Bubble& bubble)
        {
            const auto diff = (shrine.position - bubble.position);

            // `ShrineType::Voodoo` handled elsewhere
            // `ShrineType::Clicking` handled elsewhere
            // `ShrineType::Automation` handled elsewhere

            if (shrine.type == ShrineType::Magic)
            {
                if (bubble.type == BubbleType::Star || bubble.type == BubbleType::Nova)
                {
                    if (rngFast.getF(0.f, 1.f) > 0.85f)
                        spawnParticlesWithHue(230.f, 1, bubble.position, ParticleType::Star, 0.5f, 0.35f);

                    bubble.rotation += deltaTimeMs * 0.025f;

                    if (bubble.rotation >= za::tau)
                    {
                        sounds.absorb.settings.position = {bubble.position.x, bubble.position.y};
                        playSound(sounds.absorb, /* maxOverlap */ 1u);

                        turnBubbleInto(bubble, BubbleType::Normal);
                    }
                }
            }
            else if (shrine.type == ShrineType::Repulsion)
            {
                const auto strength = (shrine.getRange() - diff.length()) * 0.0000015f * deltaTimeMs;
                bubble.velocity -= diff.normalized() * strength * getWindRepulsionMult();
            }
            else if (shrine.type == ShrineType::Attraction)
            {
                const auto strength = (shrine.getRange() - diff.length()) * 0.0000025f * deltaTimeMs;
                bubble.velocity += diff.normalized() * strength * getWindAttractionMult();
            }
            else if (shrine.type == ShrineType::Camouflage)
            {
                // TODO P1: any effect?
            }
            else if (shrine.type == ShrineType::Victory)
            {
                // TODO P1: any effect?
            }

            return ControlFlow::Continue;
        });

        if (shrine.collectedReward >= pt->getComputedRequiredRewardByShrineType(shrine.type))
        {
            if (!shrine.tcDeath.hasValue())
            {
                shrine.tcDeath.emplace(TimedCountdown{.duration = 5000.f});
                shrine.tcDeath->restart();

                sounds.earthquake.settings.position = {shrine.position.x, shrine.position.y};
                playSound(sounds.earthquake);

                screenShakeAmount = 4.5f;
            }
            else
            {
                const auto cdStatus = shrine.tcDeath->tick(deltaTimeMs);

                if (cdStatus == TickResult::JustFinished)
                {
                    playSound(sounds.woosh);
                    ++pt->nShrinesCompleted;

                    const auto doShrineReward = [&](const CatType catType)
                    {
                        if (findFirstCatByType(catType) == nullptr)
                        {
                            spawnSpecialCat(shrine.position, catType);
                        }
                        else // unsealed
                        {
                            const auto unsealedReward = static_cast<MoneyType>(
                                static_cast<float>(shrine.collectedReward) * 1.5f);
                            addMoney(unsealedReward);

                            sounds.kaching.settings.position = {shrine.position.x, shrine.position.y};
                            playSound(sounds.kaching);

                            if (profile.showTextParticles)
                            {
                                auto& tp = makeRewardTextParticle(shrine.position);
                                std::snprintf(tp.buffer, sizeof(tp.buffer), "+$%llu", unsealedReward);
                            }
                        }
                    };

                    if (shrine.type == ShrineType::Voodoo)
                    {
                        doShrineReward(CatType::Witch);

                        if (!pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Witch)])
                            doTip(
                                "The Witchcat has been unsealed!\nThey perform voodoo rituals on nearby "
                                "cats,\ngiving you powerful timed buffs.");
                    }
                    else if (shrine.type == ShrineType::Magic)
                    {
                        doShrineReward(CatType::Wizard);

                        if (!pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Wizard)])
                            doTip(
                                "The Wizardcat has been unsealed!\nThey absorb star bubbles to learn "
                                "spells,\nwhich can be casted on demand.");
                    }
                    else if (shrine.type == ShrineType::Clicking)
                    {
                        doShrineReward(CatType::Mouse);

                        if (!pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Mouse)])
                            doTip(
                                "The Mousecat has been unsealed!\nThey combo-click bubbles, buff nearby "
                                "cats,\nand "
                                "provide a global click buff.");
                    }
                    else if (shrine.type == ShrineType::Automation)
                    {
                        doShrineReward(CatType::Engi);

                        if (!pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Engi)])
                            doTip(
                                "The Engicat has been unsealed!\nThey speed-up nearby cats and provide\na "
                                "global "
                                "cat buff.");
                    }
                    else if (shrine.type == ShrineType::Repulsion)
                    {
                        doShrineReward(CatType::Repulso);

                        if (!pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Repulso)])
                            doTip(
                                "The Repulsocat has been unsealed!\nNearby bubbles getting pushed away "
                                "from\nthem "
                                "gain a x2 multiplier.");
                    }
                    else if (shrine.type == ShrineType::Attraction)
                    {
                        doShrineReward(CatType::Attracto);

                        if (!pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Attracto)])
                            doTip(
                                "The Attractocat has been unsealed!\nNearby bubbles getting attracted to\nthem "
                                "gain a x2 multiplier.");
                    }
                    else if (shrine.type == ShrineType::Camouflage)
                    {
                        doShrineReward(CatType::Copy);

                        if (!pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Copy)])
                            doTip(
                                "The Copycat has been unsealed!\nThey can mimic other unique cats,\ngaining "
                                "their "
                                "powers!");
                    }
                    else if (shrine.type == ShrineType::Victory)
                    {
                        doShrineReward(CatType::Duck);

                        if (!pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Duck)])
                            doTip(
                                "It's... a duck. I am as confused\nas you are! But hey,\nyou won! "
                                "Congratulations!");

                        playSound(sounds.quack);

                        victoryTC.emplace(TimedCountdown{.duration = 6500.f});
                        victoryTC->restart();
                        delayedActions.emplaceBack(Countdown{.time = 7000.f}, [this] { playSound(sounds.letterchime); });
                    }

                    const auto catType = asIdx(shrineTypeToCatType(shrine.type));
                    if (!pt->perm.shrineCompletedOnceByCatType[catType])
                    {
                        pushNotification("New unlocks!", "A new background and BGM have been unlocked!");
                        pt->perm.shrineCompletedOnceByCatType[catType] = true;
                    }

                    profile.selectedBackground = static_cast<int>(shrine.type) + 1;
                    profile.selectedBGM        = static_cast<int>(shrine.type) + 1;

                    updateSelectedBackgroundSelectorIndex();
                    updateSelectedBGMSelectorIndex();

                    switchToBGM(static_cast<za::SizeT>(profile.selectedBGM), /* force */ false);
                }
                else if (cdStatus == TickResult::Running)
                {
                    spawnParticlesWithHue(wrapHue(shrine.getHue() + 40.f),
                                          static_cast<SizeT>(1 + 12 * shrine.getDeathProgress()),
                                          shrine.getDrawPosition() + rngFast.getVec2f({-1.f, -1.f}, {1.f, 1.f}) * 32.f,
                                          ParticleType::Fire,
                                          za::max(0.25f, 1.f - shrine.getDeathProgress()),
                                          0.75f);

                    spawnParticlesWithHue(shrine.getHue(),
                                          static_cast<SizeT>(4 + 36 * shrine.getDeathProgress()),
                                          shrine.getDrawPosition() + rngFast.getVec2f({-1.f, -1.f}, {1.f, 1.f}) * 32.f,
                                          ParticleType::Shrine,
                                          za::max(0.35f, 1.2f - shrine.getDeathProgress()),
                                          0.5f);
                }
            }
        }
    }

    za::vectorEraseIf(pt->shrines, [](const Shrine& shrine) { return shrine.getDeathProgress() >= 1.f; });
}


////////////////////////////////////////////////////////////
void Main::collectDollImpl(Doll& d, HexSession& session, const bool copy)
{
    ZA_ASSERT(!d.tcDeath.hasValue());

    statDollCollected();

    for (SizeT i = 0u; i < 8u; ++i)
        spawnParticlesWithHue(wrapHue(rngFast.getF(-50.f, 50.f) + (copy ? 180.f : 0.f)),
                              8,
                              d.getDrawPosition(),
                              ParticleType::Hex,
                              0.5f,
                              0.35f);

    screenShakeAmount = 1.5f;
    screenShakeTimer  = 500.f;

    d.tcDeath.emplace(TimedCountdown{.duration = 750.f});
    d.tcDeath->restart();

    const bool allDollsCollected = za::allOf(session.dolls.begin(), session.dolls.end(), [&](const Doll& otherDoll) {
        return otherDoll.tcDeath.hasValue();
    });

    if (allDollsCollected)
    {
        playSound(sounds.buffon);

        constexpr float buffDurationMult[] = {
            1.f, // Normal
            1.f, // Uni
            1.f, // Devil
            1.f, // Astro
            1.f, // Warden

            1.f, // Witch
            1.f, // Wizard
            1.f, // Mouse
            1.f, // Engi
            1.f, // Repulso
            1.f, // Attracto
            1.f, // Copy
            1.f, // Duck
        };

        static_assert(za::getArraySize(buffDurationMult) == nCatTypes);

        // Apply some diminishing returns if the same buff type is chosen over and over
        constexpr float buffDurationSoftCap = 60'000.f;
        const float     buffDuration        = d.buffPower * 1000.f;

        const float currentBuff = pt->buffCountdownsPerType[asIdx(d.catType)].time;

        const float factor = (currentBuff < buffDurationSoftCap)
                                 ? za::pow((buffDurationSoftCap - currentBuff) / buffDurationSoftCap, 0.15f)
                                 : 0.1f;

        pt->buffCountdownsPerType[asIdx(d.catType)].time += buffDuration * factor;

        Cat* const hexedCat = getSessionTargetCat(session);
        ZA_ASSERT(hexedCat != nullptr);

        spawnParticle({.position = d.getDrawPosition(),
                       .velocity = (hexedCat->getDrawPosition(profile.enableCatBobbing) - d.getDrawPosition()).normalized() * 1.f,
                       .scale         = 0.2f,
                       .scaleDecay    = 0.f,
                       .accelerationY = 0.f,
                       .opacity       = 1.f,
                       .opacityDecay  = 0.0015f,
                       .rotation      = 0.f,
                       .torque        = rngFast.getF(-0.0002f, 0.0002f)},
                      /* hue */ 0.f,
                      ParticleType::CatSoul);

        sounds.soulreturn.settings.position = {d.position.x, d.position.y};
        playSound(sounds.soulreturn);
    }
    else
    {
        sounds.hex.settings.position = {d.position.x, d.position.y};
        playSound(sounds.hex);
    }
}


////////////////////////////////////////////////////////////
void Main::collectDoll(Doll& d, HexSession& session)
{
    collectDollImpl(d, session, /* copy */ false);
}


////////////////////////////////////////////////////////////
void Main::collectCopyDoll(Doll& d, HexSession& session)
{
    collectDollImpl(d, session, /* copy */ true);
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateDollsImpl(const float             deltaTimeMs,
                                   const za::Vec2f         mousePos,
                                   za::Vector<HexSession>& sessionsToUse,
                                   const bool              copy)
{
    for (HexSession& session : sessionsToUse)
    {
        Cat* const hexedCat = getSessionTargetCat(session);

        if (session.dolls.empty())
        {
            if (hexedCat != nullptr)
                (copy ? hexedCat->hexedCopyTimer : hexedCat->hexedTimer)->reversed = true;

            continue;
        }

        // Can happen during prestige transition
        if (hexedCat == nullptr)
            continue;

        for (Doll& d : session.dolls)
        {
            d.update(deltaTimeMs);

            if (!d.tcActivation.isDone())
            {
                (void)d.tcActivation.tick(deltaTimeMs);
                continue;
            }

            if (!d.tcDeath.hasValue())
            {
                if (rngFast.getF(0.f, 1.f) > 0.8f)
                    spawnParticle({.position      = d.getDrawPosition() + za::Vec2f{rngFast.getF(-32.f, +32.f), 32.f},
                                   .velocity      = rngFast.getVec2f({-0.05f, -0.05f}, {0.05f, 0.05f}),
                                   .scale         = rngFast.getF(0.08f, 0.27f) * 0.5f,
                                   .scaleDecay    = 0.f,
                                   .accelerationY = -0.002f,
                                   .opacity       = 1.f,
                                   .opacityDecay  = rngFast.getF(0.00025f, 0.0015f),
                                   .rotation      = rngFast.getF(0.f, za::tau),
                                   .torque        = rngFast.getF(-0.002f, 0.002f)},
                                  /* hue */ wrapHue(rngFast.getF(-50.f, 50.f) + (copy ? 180.f : 0.f)),
                                  ParticleType::Hex);

                const bool click = (mBtnDown(getLMB(), /* penetrateUI */ false) ||
                                    playerInputState.fingerPositions[0].hasValue());

                if (click && (mousePos - d.position).lengthSquared() <= d.getRadiusSquared())
                {
                    if (copy)
                        collectCopyDoll(d, session);
                    else
                        collectDoll(d, session);
                }
            }
            else
            {
                (void)d.tcDeath->tick(deltaTimeMs);

                spawnParticlesWithHue(wrapHue(d.hue + (copy ? 180.f : 0.f)),
                                      static_cast<SizeT>(1 + 12 * d.getDeathProgress()),
                                      d.getDrawPosition() + rngFast.getVec2f({-1.f, -1.f}, {1.f, 1.f}) * 32.f,
                                      ParticleType::Hex,
                                      za::max(0.25f, 1.f - d.getDeathProgress()),
                                      0.75f);
            }
        }

        za::vectorEraseIf(session.dolls, [](const Doll& d) { return d.getDeathProgress() >= 1.f; });
    }

    // Drop sessions whose cat has since gone away (e.g. prestige transition).
    za::vectorEraseIf(sessionsToUse, [&](const HexSession& s) { return getSessionTargetCat(s) == nullptr; });
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateDolls(const float deltaTimeMs, const za::Vec2f mousePos)
{
    if (getWitchCat() == nullptr)
        return;

    gameLoopUpdateDollsImpl(deltaTimeMs, mousePos, pt->hexSessions, /* copy */ false);
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateCopyDolls(const float deltaTimeMs, const za::Vec2f mousePos)
{
    if (getCopyCat() == nullptr || pt->copycatCopiedCatType != CatType::Witch)
        return;

    gameLoopUpdateDollsImpl(deltaTimeMs, mousePos, pt->copyHexSessions, /* copy */ true);
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateHellPortals(const float deltaTimeMs)
{
    const float hellPortalRadius = pt->getComputedRangeByCatType(CatType::Devil);

    for (HellPortal& hp : pt->hellPortals)
    {
        if (hp.life.tick(deltaTimeMs) == TickResult::JustFinished)
        {
            sounds.makeBomb.settings.position = {hp.position.x, hp.position.y};
            playSound(sounds.portaloff);
        }

        for (za::SizeT iP = 0u; iP < 2u; ++iP)
            spawnParticle({.position = hp.getDrawPosition() +
                                       rngFast.getDirVec2f() * rngFast.getF(hellPortalRadius * 0.95f, hellPortalRadius * 1.15f),
                           .velocity      = rngFast.getVec2f({-0.025f, -0.025f}, {0.025f, 0.025f}),
                           .scale         = rngFast.getF(0.08f, 0.27f) * 0.85f,
                           .scaleDecay    = -0.00025f,
                           .accelerationY = 0.f,
                           .opacity       = 1.f,
                           .opacityDecay  = rngFast.getF(0.00155f, 0.0145f),
                           .rotation      = rngFast.getF(0.f, za::tau),
                           .torque        = rngFast.getF(-0.002f, 0.002f)},
                          /* hue */ 0.f,
                          ParticleType::Fire2);
    }

    za::vectorEraseIf(pt->hellPortals, [](const HellPortal& hp) { return hp.life.isDone(); });
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateWitchBuffs(const float deltaTimeMs)
{
    for (Countdown& buffCountdown : pt->buffCountdownsPerType)
        if (buffCountdown.tick(deltaTimeMs) == TickResult::JustFinished)
            playSound(sounds.buffoff);
}


////////////////////////////////////////////////////////////
void Main::addEventBubblefall(const float regionCenterX)
{
    const auto& eventsCfg = gameConstants.events;
    const auto& bfCfg     = eventsCfg.bubblefall;

    pt->activeEvents.emplaceBack(EBubblefall{
        .regionCenterX = regionCenterX,
        .regionWidth   = bfCfg.regionWidth,
        .remainingMs   = bfCfg.durationMs,
        .subTickMs     = 0.f,
    });

    pushNotification("Something is happening!", "Bubblefall TODO");
}


////////////////////////////////////////////////////////////
void Main::addEventInvincibleBubble()
{
    const auto& cfg = gameConstants.events.invincibleBubble;

    // Spawn one Combo bubble at a random x at the top of the map. The bubble
    // is the event manifestation; the variant entry expires immediately.
    auto& bubble = pt->bubbles.emplaceBack(makeRandomBubble(*pt, rng, pt->getMapLimit(), boundaries.y));

    bubble.type      = BubbleType::Combo;
    bubble.ephemeral = true;
    bubble.radius    = rng.getF(cfg.minRadius, cfg.maxRadius);

    // Keep the bubble well inside the play area: at least `spawnEdgeMarginPx`
    // away from either horizontal boundary (capped so we never invert the
    // range on a tiny / unbought map).
    const float mapLimit = pt->getMapLimit();
    const float margin   = za::min(cfg.spawnEdgeMarginPx, za::max(0.f, mapLimit * 0.5f - bubble.radius));
    bubble.position.x    = rng.getF(margin, mapLimit - margin);
    bubble.position.y    = -bubble.radius * rng.getF(cfg.spawnYOffsetTopMin, cfg.spawnYOffsetTopMax);
    bubble.velocity.x    = rng.getF(-cfg.velocityJitterX, cfg.velocityJitterX);
    bubble.velocity.y    = cfg.initialVelocityY + rng.getF(-cfg.velocityJitterY, cfg.velocityJitterY);

    pt->activeEvents.emplaceBack(EInvincibleBubble{.remainingMs = 0.f});

    spawnParticles(32, bubble.position, ParticleType::Bubble, 0.6f, 0.5f);
    sounds.shimmer.settings.position = {bubble.position.x, bubble.position.y};
    playSound(sounds.shimmer);

    pushNotification("Something is happening!", "Invincible bubble TODO");
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateEvents(const float deltaTimeMs)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    if (inPrestigeTransition || splashCountdown.time > 0.f)
        return;

    const auto& eventsCfg = gameConstants.events;
    const auto& bfCfg     = eventsCfg.bubblefall;

    // Schedule the next random event.
    if (!pt->nextEventSpawnMs.hasValue())
    {
        pt->nextEventSpawnMs.emplace(rng.getF(eventsCfg.minSpawnIntervalMs, eventsCfg.maxSpawnIntervalMs));
    }
    else if (false) // TODO: event eligibility rules
    {
        auto& nextEventSpawnMs = pt->nextEventSpawnMs.value();

        nextEventSpawnMs -= deltaTimeMs;

        if (nextEventSpawnMs <= 0.f)
        {
            nextEventSpawnMs = rng.getF(eventsCfg.minSpawnIntervalMs, eventsCfg.maxSpawnIntervalMs);


            // Uniform pick across the available event kinds. Adding a new kind is
            // a matter of extending this branch.
            if (rng.getI<SizeT>(0u, 1u) == 0u)
            {
                const float halfWidth = bfCfg.regionWidth * 0.5f;
                addEventBubblefall(rng.getF(halfWidth, pt->getMapLimit() - halfWidth));
            }
            else
            {
                addEventInvincibleBubble();
            }
        }
    }

    // Per-kind update: advance the event's own timers and (if positional)
    // drive its spawner. Returns the remaining duration -- <= 0 means expire.
    const auto tickEvent = za::OverloadSet{
        // Invincible bubble events are fire-and-forget: the bubble itself is
        // the manifestation, so the entry expires on the very next frame.
        [&](EInvincibleBubble& e) -> float { return e.remainingMs; },

        [&](EBubblefall& e) -> float
    {
        e.remainingMs -= deltaTimeMs;

        // Attack/release envelope on the spawn rate: ramps up over
        // `attackRatio` of the duration, holds at full rate, then ramps
        // down over `releaseRatio`. Advancing `subTickMs` by
        // `deltaTimeMs * intensity` scales the effective spawn frequency
        // without changing the total event length.
        const float duration    = bfCfg.durationMs <= 0.f ? 1.f : bfCfg.durationMs;
        const float elapsedNorm = za::clamp(1.f - (e.remainingMs / duration), 0.f, 1.f);

        const float attack  = za::clamp(bfCfg.attackRatio, 0.f, 0.5f);
        const float release = za::clamp(bfCfg.releaseRatio, 0.f, 0.5f);

        float intensity = 1.f;
        if (attack > 0.f && elapsedNorm < attack)
            intensity = easeInOutCubic(za::clamp(elapsedNorm / attack, 0.f, 1.f));
        else if (release > 0.f && elapsedNorm > 1.f - release)
            intensity = easeInOutCubic(za::clamp((1.f - elapsedNorm) / release, 0.f, 1.f));

        e.subTickMs += deltaTimeMs * intensity;

        while (e.subTickMs >= bfCfg.spawnIntervalMs)
        {
            e.subTickMs -= bfCfg.spawnIntervalMs;

            const float leftX  = e.regionCenterX - e.regionWidth * 0.5f;
            const float rightX = e.regionCenterX + e.regionWidth * 0.5f;

            // Bypass the bubble-count cap: event bubbles are added directly.
            // They're flagged `ephemeral` so they despawn at the bottom
            // instead of being recycled like regular bubbles.
            for (SizeT i = 0u; i < bfCfg.bubblesPerTick; ++i)
            {
                auto& bubble = pt->bubbles.emplaceBack(makeRandomBubble(*pt, rng, pt->getMapLimit(), boundaries.y));

                bubble.position.x = rng.getF(leftX, rightX);
                bubble.position.y = -bubble.radius * rng.getF(1.f, 3.f);
                bubble.velocity.x = rng.getF(-bfCfg.velocityJitterX, bfCfg.velocityJitterX);
                bubble.velocity.y = bfCfg.initialVelocityY + rng.getF(-bfCfg.velocityJitterY, bfCfg.velocityJitterY);
                bubble.ephemeral  = true;

                spawnParticles(4, bubble.position, ParticleType::Bubble, 0.35f, 0.5f);

                sounds.reversePop.settings.position = {bubble.position.x, bubble.position.y};
                playSound(sounds.reversePop, /* maxOverlap */ 1u);
            }
        }

        return e.remainingMs;
    },
    };

    for (GameEvent& ev : pt->activeEvents)
        (void)ev.linearVisit(tickEvent);

    za::vectorEraseIf(pt->activeEvents,
                      [&](GameEvent& ev)
    {
        return ev.linearVisit(za::OverloadSet{
            [](const EBubblefall& e) { return e.remainingMs <= 0.f; },
            [](const EInvincibleBubble& e) { return e.remainingMs <= 0.f; },
        });
    });
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateMana(const float deltaTimeMs)
{
    if (getWizardCat() == nullptr)
        return;

    //
    // Mana mult buff
    const float manaMult = pt->buffCountdownsPerType[asIdx(CatType::Wizard)].time > 0.f ? 3.5f : 1.f;

    //
    // Mana
    if (pt->mana < pt->getComputedMaxMana())
        pt->manaTimer += deltaTimeMs * manaMult;
    else
        pt->manaTimer = 0.f;

    if (pt->manaTimer >= pt->getComputedManaCooldown())
    {
        pt->manaTimer = 0.f;

        if (pt->mana < pt->getComputedMaxMana())
        {
            pt->mana += 1u;

            if (profile.showFullManaNotification && pt->mana == pt->getComputedMaxMana())
                pushNotification("Mana fully charged!", "The Wizardcat is eager to cast a spell...");
        }
    }

    //
    // Mewltiplier Aura spell
    if (pt->mewltiplierAuraTimer > 0.f)
    {
        pt->mewltiplierAuraTimer -= deltaTimeMs;
        pt->mewltiplierAuraTimer = za::max(pt->mewltiplierAuraTimer, 0.f);

        const float wizardRange = pt->getComputedRangeByCatType(CatType::Wizard);

        if (const Cat* wizardCat = getWizardCat(); wizardCat != nullptr)
            for (SizeT i = 0u; i < 8u; ++i)
                spawnParticlesWithHueNoGravity(230.f,
                                               1,
                                               rngFast.getPointInCircle(wizardCat->position, wizardRange),
                                               ParticleType::Star,
                                               0.15f,
                                               0.05f);

        if (const Cat* copyCat = getCopyCat(); copyCat != nullptr && pt->copycatCopiedCatType == CatType::Wizard)
            for (SizeT i = 0u; i < 8u; ++i)
                spawnParticlesWithHueNoGravity(230.f,
                                               1,
                                               rngFast.getPointInCircle(copyCat->position, wizardRange),
                                               ParticleType::Star,
                                               0.15f,
                                               0.05f);
    }


    //
    // Stasis Field spell
    if (pt->stasisFieldTimer > 0.f)
    {
        pt->stasisFieldTimer -= deltaTimeMs;
        pt->stasisFieldTimer = za::max(pt->stasisFieldTimer, 0.f);

        const float wizardRange = pt->getComputedRangeByCatType(CatType::Wizard);

        if (const Cat* wizardCat = getWizardCat(); wizardCat != nullptr)
            for (SizeT i = 0u; i < 8u; ++i)
                spawnParticlesWithHueNoGravity(50.f,
                                               1,
                                               rngFast.getPointInCircle(wizardCat->position, wizardRange),
                                               ParticleType::Star,
                                               0.15f,
                                               0.05f);

        if (const Cat* copyCat = getCopyCat(); copyCat != nullptr && pt->copycatCopiedCatType == CatType::Wizard)
            for (SizeT i = 0u; i < 8u; ++i)
                spawnParticlesWithHueNoGravity(50.f,
                                               1,
                                               rngFast.getPointInCircle(copyCat->position, wizardRange),
                                               ParticleType::Star,
                                               0.15f,
                                               0.05f);
    }
}


////////////////////////////////////////////////////////////
void Main::gameLoopUpdateAutocast()
{
    Cat* wizardCat = getWizardCat();
    Cat* copyCat   = getCopyCat();

    if (wizardCat == nullptr || !pt->perm.autocastPurchased || pt->perm.autocastIndex == 0u || isWizardBusy())
        return;

    const auto spellIndex = pt->perm.autocastIndex - 1u;

    if (static_cast<za::SizeT>(spellIndex) > pt->psvSpellCount.nPurchases)
        return;

    if (pt->mana >= spellManaCostByIndex[spellIndex])
    {
        pt->mana -= spellManaCostByIndex[spellIndex];
        castSpellByIndex(spellIndex, wizardCat, copyCat);

        constexpr const char* spellNames[4] = {
            "Starpaw Conversion",
            "Mewltiplier Aura",
            "Dark Union",
            "Stasis Field",
        };

        pushNotification("Autocast", "Spell \"%s\" casted!", spellNames[spellIndex]);
    }
}
