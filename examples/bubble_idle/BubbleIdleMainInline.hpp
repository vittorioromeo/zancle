#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Inline definitions for `Main` member functions and free
// helpers that touch `Playthrough` and `Profile` member data.
//
// Kept out of `BubbleIdleMain.hpp` so most TUs can include the
// main header without paying for `Playthrough.hpp` (~581 lines
// + 27 transitive includes) and `Profile.hpp`. TUs that call any
// of these helpers must include this header too.
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Aliases.hpp"
#include "Bubble.hpp"
#include "BubbleIdleMain.hpp"
#include "BubbleType.hpp"
#include "CatType.hpp"
#include "Playthrough.hpp"
#include "Profile.hpp"

#include "ExampleUtils/ControlFlow.hpp"
#include "ExampleUtils/HueColor.hpp"
#include "ExampleUtils/MathUtils.hpp"

#include "Zancle/Random/FastNonCryptoRng.hpp"

#include "Zancle/Geometry/Vec2.hpp"


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline Bubble makeRandomBubble(
    Playthrough&          pt,
    za::FastNonCryptoRng& rng,
    const float           mapLimit,
    const float           maxY)
{
    return {
        .position = rng.getVec2f({mapLimit, maxY}),
        .velocity = rng.getVec2f({-0.1f, -0.1f}, {0.1f, 0.1f}),

        .radius   = rng.getF(0.07f, 0.16f) * 256.f *
                    remap(static_cast<float>(pt.psvBubbleCount.nPurchases), 0.f, 30.f, 1.1f, 0.8f),
        .rotation = 0.f,
        .torque   = 0.f,
        .hueMod   = 0.f,

        .repelledCountdown  = {},
        .attractedCountdown = {},

        .type      = BubbleType::Normal,
        .ephemeral = false,
        .hueSeed   = pt.nextBubbleHueSeed++,
    };
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline void Main::spawnParticles(const SizeT n, const za::Vec2f position, auto... args)
{
    if (!profile.showParticles || !particleCullingBoundaries.isInside(position))
        return;

    for (SizeT i = 0; i < n; ++i)
        implEmplaceParticle(position, args...);
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline void Main::spawnParticlesWithHue(const float hue, const SizeT n, const za::Vec2f position, auto... args)
{
    if (!profile.showParticles || !particleCullingBoundaries.isInside(position))
        return;

    for (SizeT i = 0; i < n; ++i)
        implEmplaceParticle(position, args...).hueByte = hueToByte(hue);
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline void Main::spawnParticlesNoGravity(const SizeT n, const za::Vec2f position, auto... args)
{
    if (!profile.showParticles || !particleCullingBoundaries.isInside(position))
        return;

    for (SizeT i = 0; i < n; ++i)
        implEmplaceParticle(position, args...).accelerationY = 0.f;
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline void Main::spawnParticlesWithHueNoGravity(
    const float     hue,
    const SizeT     n,
    const za::Vec2f position,
    auto... args)
{
    if (!profile.showParticles || !particleCullingBoundaries.isInside(position))
        return;

    for (SizeT i = 0; i < n; ++i)
    {
        auto& p         = implEmplaceParticle(position, args...);
        p.accelerationY = 0.f;
        p.hueByte       = hueToByte(hue);
    }
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline void Main::withAllStats(auto&& func)
{
    func(profile.statsLifetime);
    func(pt->statsTotal);
    func(pt->statsSession);
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline void Main::forEachBubbleInRadiusSquared(const za::Vec2f center, const float radiusSq, auto&& func)
{
    for (Bubble& bubble : pt->bubbles)
        if ((bubble.position - center).lengthSquared() <= radiusSq)
            if (func(bubble) == ControlFlow::Break)
                break;
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline void Main::forEachBubbleInRadius(const za::Vec2f center, const float radius, auto&& func)
{
    forEachBubbleInRadiusSquared(center, radius * radius, func);
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline Bubble* Main::pickRandomBubbleInRadiusMatching(const za::Vec2f center, const float radius, auto&& predicate)
{
    const float radiusSq = radius * radius;

    SizeT   count    = 0u;
    Bubble* selected = nullptr;

    for (Bubble& bubble : pt->bubbles)
        if (predicate(bubble) && (bubble.position - center).lengthSquared() <= radiusSq)
        {
            ++count;

            // Select the current bubble with probability `1/count` (reservoir sampling)
            if (rng.getI<SizeT>(0, count - 1) == 0)
                selected = &bubble;
        }

    return (count == 0u) ? nullptr : selected;
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline float Main::getComputedCooldownByCatTypeOrCopyCat(const CatType catType) const
{
    return pt->getComputedCooldownByCatType(catType == CatType::Copy ? pt->copycatCopiedCatType : catType);
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline float Main::getComputedRangeByCatTypeOrCopyCat(const CatType catType) const
{
    return pt->getComputedRangeByCatType(catType == CatType::Copy ? pt->copycatCopiedCatType : catType);
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline bool Main::isUnicatTranscendenceActive() const
{
    return pt->perm.unicatTranscendencePurchased && pt->perm.unicatTranscendenceEnabled;
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline bool Main::isDevilcatHellsingedActive() const
{
    return pt->perm.devilcatHellsingedPurchased && pt->perm.devilcatHellsingedEnabled;
}
