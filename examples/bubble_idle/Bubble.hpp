#pragma once

#include "BubbleType.hpp"

#include "ExampleUtils/Progress.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Base/IntTypes.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] Bubble
{
    za::Vec2f position;
    za::Vec2f velocity;

    float radius;
    float rotation; // Purely visual
    float torque;
    float hueMod;

    // Star/Nova absorption progress, advanced by cats and Magic shrines: the
    // bubble is absorbed when it reaches `tau`. Reset on every type change.
    float absorptionProgress = 0.f;

    Countdown repelledCountdown;
    Countdown attractedCountdown;

    BubbleType type;

    // Event-spawned bubbles (e.g. Bubblefall) despawn when they fall off the
    // bottom instead of being recycled at the top.
    bool ephemeral = false;

    // Stable per-bubble counter used to derive the Normal-bubble hue. Assigned
    // once at spawn so the color stays put even when earlier bubbles in the
    // vector get erased (which would otherwise shift every other bubble's
    // index-based hue).
    za::U32 hueSeed = 0u;

    // Combo (invincible) bubble state. After the first click, `comboTimerMs`
    // becomes the bespoke combo window: each subsequent click resets it; if it
    // reaches 0 the bubble pops with payout. `comboClickCount` is the running
    // tally of clicks on this bubble -- drives the payout magnitude.
    float   comboTimerMs    = 0.f;
    za::U32 comboClickCount = 0u;

    // Pending bubble transformation (Unicat star, Devilcat bomb, ...).
    // While `pendingTransformMs > 0`, the bubble is frozen mid-air and inert
    // (cannot be popped). When the timer reaches 0, the bubble turns into
    // `pendingTransformTargetType`; if `pendingTransformCatIdx` is not the
    // sentinel (0xFFFF) it is credited in `bombIdxToCatIdx` at resolve time.
    float   pendingTransformMs         = 0.f;
    za::U8  pendingTransformTargetType = 0u;
    za::U16 pendingTransformCatIdx     = 0xFF'FFu;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline float getRadiusSquared() const
    {
        return radius * radius;
    }
};
