#pragma once

#include "Aliases.hpp"
#include "ShrineConstants.hpp"
#include "ShrineType.hpp"
#include "TextShakeEffect.hpp"

#include "ExampleUtils/HueColor.hpp"
#include "ExampleUtils/Progress.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Cos.hpp"
#include "Zancle/Math/Remainder.hpp"
#include "Zancle/Math/Sin.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] Shrine
{
    za::Vec2f position;

    float wobbleRadians = 0.f;

    za::Optional<TimedCountdown> tcActivation;
    za::Optional<TimedCountdown> tcDeath;

    TextShakeEffect textStatusShakeEffect;

    MoneyType collectedReward = 0u;

    ShrineType type;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline bool isActive() const
    {
        return isDoneOr(tcActivation);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getActivationProgress() const
    {
        return getElapsedOr(tcActivation, 0.f);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline float getDeathProgress() const
    {
        return getElapsedOr(tcDeath, 0.f);
    }

    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] inline void update(const float deltaTime)
    {
        textStatusShakeEffect.update(deltaTime);
        wobbleRadians = za::truncatedRemainder(wobbleRadians + deltaTime * 0.002f + getDeathProgress() * 0.2f, za::tau);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline za::Vec2f getDrawPosition() const
    {
        const za::Vec2f wobbleOffset{za::cos(wobbleRadians) * (7.5f + getDeathProgress() * 128.f),
                                     za::sin(wobbleRadians) * (14.f + getDeathProgress() * 128.f)};

        return position + getActivationProgress() * wobbleOffset;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline float getRange() const
    {
        if (!isActive())
            return 0.f;

        return 256.f * (1.f - getDeathProgress());
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline float getRangeSquared() const
    {
        const float range = getRange();
        return range * range;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline bool isInRange(const za::Vec2f point) const
    {
        return isActive() && (point - position).lengthSquared() < getRangeSquared();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline float getHue() const
    {
        return wrapHue(shrineHues[static_cast<U8>(type)]);
    }

    ////////////////////////////////////////////////////////////
    static inline constexpr float radius = 64.f;

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] inline constexpr float getRadius() const noexcept
    {
        return radius;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr float getRadiusSquared() const noexcept
    {
        return radius * radius;
    }
};
