#include "TextShakeEffect.hpp"

#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/TextData.hpp"

#include "Zancle/Geometry/Angle.hpp"

#include "Zancle/Random/FastNonCryptoRng.hpp"

#include "Zancle/Math/ClampMacro.hpp"


////////////////////////////////////////////////////////////
void TextShakeEffect::bump(za::FastNonCryptoRng& rng, const float strength)
{
    grow  = strength;
    angle = rng.getF(-grow * 0.2f, grow * 0.2f);
}

////////////////////////////////////////////////////////////
void TextShakeEffect::update(const float deltaTimeMs)
{
    if (grow > 0.f)
        grow -= deltaTimeMs * 0.0165f;

    if (angle != 0.f)
    {
        const float sign = angle > 0.f ? 1.f : -1.f;
        angle -= sign * deltaTimeMs * 0.00565f;

        if (sign * angle < 0.f)
            angle = 0.f;
    }

    grow  = ZA_CLAMP(grow, 0.f, 5.f);
    angle = ZA_CLAMP(angle, -0.5f, 0.5f);
}

////////////////////////////////////////////////////////////
void TextShakeEffect::applyToText(auto& text) const
{
    text.scale    = {1.f + grow * 0.2f, 1.f + grow * 0.2f};
    text.rotation = za::radians(angle);
}


template void TextShakeEffect::applyToText(za::Text& text) const;
template void TextShakeEffect::applyToText(za::TextData& text) const;
