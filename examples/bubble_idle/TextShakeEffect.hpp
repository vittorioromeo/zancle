#pragma once


namespace za
{
class FastNonCryptoRng;
} // namespace za


////////////////////////////////////////////////////////////
struct [[nodiscard]] TextShakeEffect
{
    float grow  = 0.f;
    float angle = 0.f;

    void bump(za::FastNonCryptoRng& rng, float strength);
    void update(float deltaTimeMs);
    void applyToText(auto& text) const;
};
